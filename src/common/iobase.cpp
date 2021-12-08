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
    c