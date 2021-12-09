
// 2019 Team AobaZero
// This source code is in the public domain.
#pragma once
#include <set>
#include <cstddef>
#include <cstdint>

template <typename T> class PtrLen;
class FName;
class FNameID;
namespace OSI { class IAddr; }
namespace IOAux {
  static constexpr unsigned int maxlen_path = 256U;
  int64_t match_fname(const char *p, const char *fmt_scn) noexcept;
  size_t make_time_stamp(char *p, size_t n, const char *fmt) noexcept;
  bool is_weight_ok(const char *fname, uint64_t &digest) noexcept;
  bool is_weight_ok(PtrLen<const char> plxz, uint64_t &digest) noexcept;
  void grab_files(std::set<FNameID> &dir_list, const char *dname,
                  const char *fmt, int64_t min_no) noexcept;
  FNameID grab_max_file(const char *dname, const char *fmt) noexcept;
  template <typename T> T bytes_to_int(const char *p) noexcept;
  template <typename T> void int_to_bytes(const T &v, char *p) noexcept;
}

class IAddrKey {
  using uint = unsigned int;
  uint32_t _addr;