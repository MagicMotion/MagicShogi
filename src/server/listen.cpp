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
  void set_wght(shared_ptr<const Wght> wght) noexcept { _wght = wght; }
  void reset_wght() noexcept { _wght.reset(); }
  void reset_time() noexcept { _time = system_clock::now(); }
  char *get_buf() noexcept { return _buf.get(); }
  void clear() noexcept {
    assert(sckt_ok());
    close(_sckt);
    _wght.reset();
    _sckt = -1; }
};

class AddrList {
  mutex _m;
  set<uint32_t> _set;
  FName _fname;
  off_t _size;
  timespec _mtim;
  
public:
  explicit AddrList(const char *p) noexcept : _fname(p), _size(0) {}

  void reload(Logger *logger) noexcept {
    lock_guard<mutex> lock(_m);
    struct stat sb;
    if (stat(_fname.get_fname(), &sb) < 0) {
      if (errno == ENOENT) {
	if (_size) {
	  _size = 0;
	  logger->out(nullptr, load_list_s, _fname.get_fname());
	  _set.clear(); }
	return; }
      die(ERR_CLL("stat() failed")); }
    
    if (_size == sb.st_size
	&& _mtim.tv_sec == sb.st_mtim.tv_sec
	&& _mtim.tv_nsec == sb.st_mtim.tv_nsec) return;
    _size = sb.st_size;
    _mtim = sb.st_mtim;
    
    _set.clear();
    ifstream ifs(_fname.get_fname());
    if (!ifs) return;
    
    char buf[1024];
    logger->out(nullptr, load_list_s, _fname.get_fname());
    while (ifs.getline(buf, sizeof(buf))) {
      char *token, *saveptr;
      token = strtok_r(buf, " \t\r\n", &saveptr);
      if (!token || token[0] == '#') continue;

      try {
	OSI::IAddr iaddr(token, 0);
	_set.insert(iaddr.get_addr());
	logger->out(nullptr, "%s", iaddr.get_cipv4()); }
      catch (const exception &e) { cout << e.what() << endl; } } }

  bool find(uint32_t addr) noexcept {
    lock_guard<mutex> lock(_m);
    return _set.find(addr) != _set.end(); }

  void insert(const OSI::IAddr & iaddr) noexcept {
    lock_guard<mutex> lock(_m);
    _set.insert(iaddr.get_addr());

    ofstream ofs(_fname.get_fname(), ifstream::app);
    ofs << iaddr.get_cipv4() << endl;
    if (!ofs) die(ERR_INT("cannot write to %s", _fname.get_fname()));
    
    struct stat sb;
    if (stat(_fname.get_fname(), &sb) < 0) die(ERR_CLL("stat() failed"));
    _size = sb.st_size;
    _mtim = sb.st_mtim; }
};

ssize_t Listen::send_wrap(const Peer &peer, const void *buf, size_t len,
			  int flags) noexcept {
  assert(peer.sckt_ok() && buf);
  ssize_t ret = send(peer.get_sckt(), buf, len, flags);
  if (ret <= 0) return ret;
  IAddrValue & value = (*_maxconn_table)[IAddrKey(peer)];
  assert(_maxconn_table->ok());
  if (!value.initialized()) value.reset(_maxconn_len, _now);
  value.update_size_com(ret, _now);
  return ret; }

ssize_t Listen::recv_wrap(const Peer &peer, void *buf, size_t len,
			  int flags) noexcept {
  assert(peer.sckt_ok() && buf);
  return recv(peer.get_sckt(), buf, len, flags);
  /*
  ssize_t ret = recv(peer.get_sckt(), buf, len, flags);
  if (ret <= 0) ret;

  IAddrValue & value = (*_maxconn_table)[IAddrKey(peer)];
  assert(_maxconn_table->ok());
  if (!value.initialized()) value.reset(_maxconn_len, _now);
  value.update_size_com(ret, _now);
  return ret;
  */
 }

Listen::Listen() noexcept : _bEndWorker(false), _s_addr(new sockaddr_in),
  _deny_list(new AddrList(fname_deny_list)),
  _ignore_list(new AddrList(fname_ignore_list)),
  _last_deny(system_clock::now()), _now(system_clock::now()), _sckt_lstn(-1) {}

Listen::~Listen() noexcept { if (0 <= _sckt_lstn) close(_sckt_lstn); }

void Listen::worker() noexcept {
  while (!_bEndWorker) {
    sleep_for(seconds(1));
    _deny_list->reload(_logger);
    _ignore_list->reload(_logger); } }

void Listen::end() noexcept {
  _bEndWorker = true;
  _thread.join(); }

Listen & Listen::get() noexcept {
  static Listen instance;
  return instance; }

void Listen::start(Logger *logger, uint port_player, uint backlog,
		   uint selectTO, uint playerTO, uint max_accept,
		   uint max_recv, uint max_send, uint len_block,
		   uint maxconn_sec, uint maxconn_min, uint cutconn_min,
		   uint maxlen_com) noexcept {
  assert(logger);
  int reuse = 1;

  _deny_list->reload(logger);
  _ignore_list->reload(logger);
  _maxconn_table.reset(new HashTable<IAddrKey, IAddrValue>(15U, 2U << 15U));
  assert(_maxconn_table->ok());
  _now           = system_clock::now();
  _maxlen_com    = maxlen_com;
  _maxconn_sec   = maxconn_sec;
  _maxconn_min   = maxconn_min;
  _cutconn_min   = cutconn_min;
  _maxconn_len   = max(max(maxconn_sec, maxconn_min), cutconn_min) + 1U;
  _logger        = logger;
  _max_accept    = max_accept;
  _max_recv      = max_recv;
  _max_send      = max_send;
  _len_block     = len_block;
  _selectTO_sec  = selectTO / 1000U;
  _selectTO_usec = (selectTO % 1000U) * 1000U;
  _playerTO      = playerTO;
  _thread        = thread(&Listen::worker, this);
  
  _sckt_lstn = socket(AF_INET, SOCK_STREAM, 0);
  if (_sckt_lstn < 0) die(ERR_CLL("socket"));
  if (setsockopt(_sckt_lstn, SOL_SOCKET, SO_REUSEADDR,
		 &reuse, sizeof(reuse)) < 0) die(ERR_CLL("setsockopt"));

  memset(_s_addr.get(), 0, sizeof(*_s_addr));
  _s_addr->sin_family      = AF_INET;
  _s_addr->sin_addr.s_addr = INADDR_ANY;
  _s_addr->sin_port        = htons(port_player);
  if (bind(_sckt_lstn, reinterpret_cast<sockaddr *>(_s_addr.get()),
	   sizeof(*_s_addr)) < 0) die(ERR_CLL("bind"));
  if (listen(_sckt_lstn, backlog) < 0) die(ERR_CLL("listen"));

  _pPeer.reset(new Peer [_max_accept]);
  for (uint u = 0; u < _max_accept; ++u) new (&(_pPeer[u])) Peer(_max_recv); }

void Listen::handle_connect() noexcept {
  sockaddr_in c_addr;
    
  memset(&c_addr, 0, sizeof(c_addr));
  socklen_t c_len = sizeof(c_addr);
  int sckt 