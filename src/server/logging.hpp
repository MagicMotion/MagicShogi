// 2019 Team AobaZero
// This source code is in the public domain.
#pragma once
#include "xzi.hpp"
#include "iobase.hpp"
#include <fstream>
#include <mutex>
#include <cinttypes>
using std::ofstream;
using std::mutex;
namespace OSI { class IAddr; }

namespace Log {
  constexpr char bad_XZ_format[]      = "bad XZ format";
  constexpr char bad_CSA_format[]     = "bad CSA format";
  constexpr char conn_denied[]        = "connection denied";
  constexpr char record_ignored[]     = "record ignored";
  constexpr char too_many_conn_sec[]  = "too many connections in a second";
  constexpr char too_many_conn_min[]  = "too many connections in a minute";
  constexp