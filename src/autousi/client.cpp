// 2019 Team AobaZero
// This source code is in the public domain.
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif
#include "client.hpp"
#include "err.hpp"
#include "jqueue.hpp"
#include "option.hpp"
#include "osi.hpp"
#include "param.hpp"
#include "xzi.hpp"
#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <cassert>
#include <chrono>
#include <cinttypes>
#include <cstring>
using std::cerr;
using std::cout;
using std::current_exception;
using std::endl;
using std::exception;
using std::exception_ptr;
using std::ifstream;
using std::ios;
using std::lock_guard;
using std::make_shared;
using std::map;
using std::min;
using std::mutex;
using std::ofstream;
using std::rethrow_exception;
using std::set;
using std::set_terminate;
using std::string;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using ErrAux::die;
using namespace IOAux;
using uchar = unsigned char;
using uint  = unsigned int;

constexpr uint snd_retry_interval  = 5U;    // in sec
constexpr uint snd_max_retry       = 3U;    // in sec
constexpr uint snd_sleep           = 200U;  // in msec
constexpr uint wght_polling        = 30U;   // in sec
constexpr uint wght_retry_interval = 7U;    // in sec

constexpr uint maxlen_rec_xz       = 1024U * 1024U;
constexpr uint len_head            = 5U;
constexpr char info_name[]         = "info.txt";
constexpr char tmp_name[]          = "tmp.bi_";
constexpr char corrupt_fmt[]       = ( "Corruption of temporary files. "
				       "Cleeanup files in %s/" );
constexpr char tmp_fmt[]           = "tmp%012" PRIi64 ".bi_";
constexpr char wght_xz_fmt[]       = "w%012" PRIi64 ".txt.xz";
constexpr char fmt_tmp_scn[]       = "tmp%16[^.].bi_";
constexpr char fmt_wght_xz_scn[]   = "w%16[^.].txt.xz";

set<FNameID> WghtFile::_all;
mutex WghtFile::_m;
void WghtFile::cleanup() {
  lock_guard<mutex> lock(_m);
  for (auto it = _all.begin(); it != _all.end(); it = _all.erase(it))
    remove(it->get_fname()); }

WghtFile::WghtFile(const FNameID &fxz, uint keep_wght) noexcept
  : _fname(fxz), _keep_wght(keep_wght) {
  uint len = _fname.get_len_fname();
  const char *p = _fname.get_fname();
  if (len < 4U || p[len-3U] != '.' || p[len-2U] != 'x' || p[len-1U] != 'z')
    die(ERR_INT("bad file name %s", p));
  _fname.cut_fname(3U);
  
  ifstream ifs(fxz.get_fname(), ios::binary);
  if (!ifs) die(ERR_INT("cannot open %s", fxz.get_fname()));
  
  ofstream ofs(_fname.get_fname(), ios::binary | ios::trunc);
  XZDecode<ifstream, ofstream> xzd;
  if (!xzd.decode(&ifs, &ofs, SIZE_MAX))
    die(ERR_INT("cannot decode %s", fxz.get_fname()));
  
  ifs.close();
  ofs.close();
  if (!ofs) die(ERR_INT("cannot write to %s", _fname.get_fname()));

  _crc64 = xzd.get_crc64();
  if (_keep_wght) return;
  lock_guard<mutex> lock(_m);
  _all.insert(_fname); }

WghtFile::~WghtFile() noexcept {
  if (_keep_wght) return;
  lock_guard<mutex> lock(_m);
  remove(_fname.get_fname());
  _all.erase(_fname); }

float Client::receive_header(const OSI::Conn &conn, uint TO, uint bufsiz) {
  char buf[HEADER_SIZE];
  conn.recv(buf, HEADER_SIZE, TO, bufsiz);
  
  uint Major = static_cast<uchar>(buf[0]);
  uint Minor = static_cast<uchar>(buf[1]);
  if (Major != Ver::major || Ver::minor < Minor)
    die(ERR_INT("Please update autousi!"));

  assert(HEADER_SIZE >= 1+1+2+HANDICAP_TYPE*2);
  for (int i=0; i<HANDICAP_TYPE; i++) {
    uint16_t x = bytes_to_int<uint16_t>(buf + 4 + i*2);
    _handicap_rate[i] = x;
//  cout << "handi rate " << i << " = " << x << e