// 2019 Team AobaZero
// This source code is in the public domain.
#include "datakeep.hpp"
#include "err.hpp"
#include "hashtbl.hpp"
#include "listen.hpp"
#include "logging.hpp"
#include "osi.hpp"
#include "param.hpp"
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
using std::cout;
using std::endl;
using std::exception;
using std::fill_n;
using std::ifstream;
using std::lock_guard;
using std::max;
using std::min;
using std::mutex;
using std::ofstream;
using std::set;
using std::thread;
using std::unique_ptr;
using std::shared_ptr;
using std::this_thread::sleep_for;
using std::chrono::duration_cast;
using std::chrono::time_point;
using std::chrono::time_point_cast;
using std::chrono::system_clock;
using std::chrono::seconds;
using ErrAux::die;
using namespace Log;
using namespace IOAux;

constexpr uint deny_log_interval   = 3U; // in sec
constexpr char fname_deny_list[]   = "deny-list.cfg";
constexpr char fname_ignore_list[] = "ignore-list.cfg";

enum Cmd { RecvRec = 0, SendInfo = 1, SendWght = 2 };
enum class StatSend { DoNothing, SendInfo, SendWght, SendHeader };

class IAddrValue {
  uint _len;
  unique_ptr<time_point<system_clock> []> _tbl_recv_time;
  seconds _sec_last;
  size_t _size_com;
  
  void update(const time_point<system_clock> & now) noexcept {
    seconds sec_now = duration_cast<seconds>(now.time_since_epoch());
    if (_sec_last < sec_now) { _size_com = 0; _sec_last = sec_now; } }
  
public:
  explicit IAddrValue() noexcept {}
  IAddrValue & operator=(IAddrValue &&value) noexcept {
    _len      = value._len;
    _sec_last = value._sec_last;
    _tbl_recv_time.swap(value._tbl_recv_time);
    return *this; }
  void reset(uint size, const time_point<system_clock> &now) noexcept {
    _len      = 0;
    _size_com = 0;
    _sec_last = duration_cast<seconds>(now.time_since_epoch());
    _tbl_recv_time.reset(new time_point<system_clock> [size]);
    fill_n(_tbl_recv_time.get(), size, now - seconds(3600)); }
  void set_len(uint len) noexcept { _len = len; }
  void update_size_com(size_t size_com,
		       const time_point<system_clock> &now) noexcept {
    update(now); _size_com += size_com; }
  size_t get_size_com(const time_point<system_clock> &now) noexcept {
    update(now); return _size_com;}
  uint get_last() const noexcept { return _sec_last.count(); }
  uint get_len() const noexcept { return _len; }
  bool initialized() const noexcept { return _tbl_recv_time.get(); }
  time_point<system_clock> *get_tbl_recv_time() const noexcept {
    return _tbl_recv_time.get(); }
};

class Peer : public OSI::IAddr {
  using uint   = unsigned int;
  using ushort = unsigned short;
  int _sckt;
  time_point<system_clock> _time;
  size_t _len, _len_sent;
  unique_ptr<char []> _buf;
  shared_ptr<const Wght> _wght;
  uint _iblock;
  StatSend _stat_send;
  
public:
  // special member functions
  explicit Peer() noexcept {}
  ~Peer() noexcept { if (sckt_ok()) close(_sckt); }
  Peer & operator=(const Peer &) = delete;
  Peer(const Peer &) = delete;

  // const member functions
  StatSend get_stat_send() const noexcept { return _stat_send; }
  bool have_wght() const noexcept { return static_cast<bool>(_wght); }
  uint get_iblock() const noexcept { return _iblock; }
  int get_sckt() const noexcept { return _sckt; }
  int sckt_ok() const noexcept { return 0 <= _sckt; }
  size_t get_len() const noexcept { return _len; }
  size_t get_len_sent() const noexcept { return _len_sent; }
  const Wght *get_wght() const noexcept { return _wght.get(); }
  const time_point<system_clock> &get_time() const noexcept { return _time; }
  
  // non-const member functions
  explicit Peer(size_t len) noexcept : _sckt(-1), _buf(new char [len]) {}
  void set_stat_send(StatSend stat_send) noexcept { _stat_send = stat_send; }
  void set_sckt(int sckt) noexcept { _sckt = sckt; }
  void set_len(size_t len) noexcept { _len = len; }
  void set_len_sent(size_t len) noexcept { _len_sent = len; }
  void set_iblock(uint u) noexcept { _iblock = u; }
  void set_wght(shared_ptr<const Wght> wght) noexcept { 