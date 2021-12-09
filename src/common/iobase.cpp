// 2019 Team AobaZero
// This source code is in the public domain.
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif
#include "err.hpp"
#include "osi.hpp"
#include "xzi.hpp"
#include "iobase.hpp"
#include <fstream>
#include <limits>
#include <type_traits>
#include <cassert>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <ctime>
using std::ifstream;
using std::ios;
using std::ofstream;
using std::set;
using std::unique_ptr;
using namespace IOAux;
using ErrAux::die;
using uchar  = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;

int64_t IOAux::match_fname(const char *p, const char *fmt_scn) noexcept {
  assert(p && fmt_scn);
  char buf_new[maxlen_path + 1];
  char fmt_new[maxlen_path + 4];
  char buf[1024];
  char dummy1, dummy2;

  strcpy(buf_new, p);
  strcat(buf_new, "x");
  strcpy(fmt_new, fmt_scn);
  strcat(fmt_new, "%c%c");
  int64_t i64 = -1;
  if (sscanf(buf_new, fmt_new, buf, &dummy1, &dummy2) == 2 && dummy1 == 'x') {
    char *endptr;
    long long int v(strtoll(buf, &endptr, 10));
    if (endptr != buf && *endptr == '\0'
        && 0 <= v && v <= INT64_MAX && v < LLONG_MAX) i64 = v; }

  return i64; }

IAddrKey::IAddrKey(const OSI::IAddr &iaddr) noexcept
  : _addr(iaddr.get_addr()), _index(static_cast<uint>(iaddr.get_crc64())) {}

const char *FName::get_bname() const noexcept {
  const char *bname = _fname;
  for (const char *p = _fname; *p != '\0'; ++p) if (*p == '/') bname = p + 1;
  return bname; }

bool FName::ok() const noexcept {
  uint u;
  for (u = 0; u < _len; ++u) if (_fname[u] <= 0) return false;
  if (_fname[u] != 0) return false;
  return true; }

FName & FName::operator=(const FName &fname) noexcept {
  if (this == &fname) return *this;
  _len = fname._len;
  if (sizeof(_fname) <= _len) die(ERR_INT("FName too long"));
  memcpy(_fname, fname._fname, _len + 1U);
  return *this; }

FName::FName(const FName &fname) noexcept : _len(fname._len) {
  if (sizeof(_fname) <= _len) die(ERR_INT("FName too long"));
  memcpy(_fname, fname._fname, _len + 1U); }

FName::FName(const char *p1, const char *p2) noexcept : _len(0) {
  add_fname(p1); add_fname(p2); }

void FName::add_fname(const char *p) noexcept {
  assert(p);
  if (0 < _len && _fname[_len - 1U] != '/') {
    if (sizeof(_fname) <= _len) die(ERR_INT("FName too long"));
    _fname[_len++] = '/'; }
    
  for (;; ++_len, ++p) {
    if (sizeof(_fname) <= _len) die(ERR_INT("FName too long"));
    if (*p == '\0') break;
    _fname[_len] = *p; }
  _fname[_len] = '\0'; }

void FName::add_fmt_fname(const char *fmt, ...) noexcept {
  assert(fmt);
  va_list ap;
  char bname[maxlen_path];

  va_start(ap, fmt);
  int ret = vsnprintf(bname, sizeof(bname), fmt, ap);
  va_end(ap);
  if (static_cast<int>(sizeof(bname)) <= ret) die(ERR_INT("FName too long"));
  add_fname(bname); }

void FName::cut_fname(uint u) noexcept {
  if (_len < u) die(ERR_INT("cannot cut %u of %u characters", u, _len));
  _len -= u;
  _fname[_len] = '\0'; }

FNameID::FNameID(int64_t i64, const FName &fname) noexcept
  : FName(fname), _i64(i64) {}

FNameID::FNameID(int64_t i64, const char *fname)
  noexcept : FName(fname), _i64(i64) {}

FNameID::FNameID(int64_t i64, const char *dname, const char *bname)
  noexcept : FName(dname, bname), _i64(i64) {}

bool operator<(const FNameID &fn1, const FNameID &fn2) {
  return fn1.get_id() < fn2.get_id(); }

size_t IOAux::make_time_stamp(char *p, size_t n, const char *fmt) noexcept {
  time_t t = time(nullptr);
  tm *ptm = localtime(&t);
  if (!ptm) die(ERR_CLL("localtime"));

  size_t len = strftime(p, n, fmt, ptm);
  if (len == 0) die(ERR_INT("strftime() failed"));
  return len; }

bool IOAux::is_weight_ok(const char *fname, uint64_t &digest) noexcept {
  assert(fname);
  ifstream ifs(fname, ios::binary | ios::ate);
  size_t len = ifs.tellg();
  ifs.seekg(0, ifs.beg);
  if (!ifs) die(ERR_INT("cannot open %s", fna