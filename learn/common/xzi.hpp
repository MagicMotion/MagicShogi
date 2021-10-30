// Copyright (C) 2019 Team AobaZero
// This source code is in the public domain.
#pragma once
#include <iosfwd>
#include <cstdint>
#include <lzma.h>

template <typename T> class PtrLen;
namespace XZAux {
  uint64_t crc64(const PtrLen<char> pl) noexcept;
}

class DevNul {};

template <typename T> class PtrLen {
public:
  T *p;
  size_t len;
  
  explicit PtrLen() noexcept {}
  ~PtrLen() noexcept {}
  
  explicit PtrLen(T *p0, size_t len0) noexcept : p(p0), len(len0) {}
  operator PtrLen<const T>() const noexcept { return PtrLen<const T>(p, len); }
  
  void clear() noexcept { len = 0; }
  bool ok() const noexcept { return p != NULL; }
};

class XZBase {
protected:
  uint8_t _inbuf[1024 * 8];
  uint8_t _outbuf[1024 * 8];

  explicit XZBase() noexcept {}
  ~XZBase(