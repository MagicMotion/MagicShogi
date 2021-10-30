// Copyright (C) 2019 Team AobaZero
// This source code is in the public domain.
#pragma once
#include <fstream>
#include <memory>
#include <set>
#include <cassert>
#include <cstdarg>
#include <netinet/in.h>
#include <arpa/inet.h>

template <typename T> class PtrLen;
class FNameID;
namespace IOAux {
  static constexpr unsigned int maxlen_path = 256U;

  size_t make_time_stamp(char *p, size_t n, const char *fmt) noexcept;
  bool is_weight_ok(const char *fname) noexcept;
  bool is_weight_ok(PtrLen<const char> plxz) noexcept;
  void grab_files(std::set<FNameID> &dir_list, const char *dname,
                  const char *fmt, int64_t min_no) noexcept;
  FNameID grab_max_file(const char *dname, const char *fmt) noexcept;
  template <typename T> T bytes_to_int(const char *p) noexcept;
  template <typename T> void int_to_bytes(const T &v, char *p) noexcept;
}

class CINET {
  using uint   = unsigned int;
  using ushort = unsigned short;
  ushort _port;
  char _cipv4[INET_ADDRSTRLEN];

public:
  explicit CINET() noexcept { _cipv4[0] = '\0'; }
  ~CINET() noexcept {}
