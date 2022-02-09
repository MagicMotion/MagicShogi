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
  
  void update(cons