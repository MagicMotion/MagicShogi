
// Copyright (C) 2019 Team AobaZero
// This source code is in the public domain.
#pragma once
#include <exception>
#include <cerrno>

namespace Err {
  using uint = unsigned int;
  static constexpr uint size_buf(1024);
  void die(const std::exception &e) noexcept;
}

class ErrCLL : public std::exception {
  char _buf[Err::size_buf];
  
public:
  explicit ErrCLL(int line, const char *fname, const char *func, int no)
    noexcept;
  virtual const char* what() const noexcept { return _buf; }
};

class ErrInt : public std::exception {
  char _buf[Err::size_buf];

public:  
  explicit ErrInt(int line, const char *fname, const char *fmt, ...) noexcept;
  virtual const char* what() const noexcept { return _buf; }
};

#define ERR_CLL(f) ErrCLL(__LINE__, __FILE__, f, errno)
#define ERR_INT(...) ErrInt(__LINE__, __FILE__, __VA_ARGS__)