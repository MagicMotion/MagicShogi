// 2019 Team AobaZero
// This source code is in the public domain.
#pragma once
#include <exception>
#include <memory>
#include <cstdint>

class FName;
struct sockaddr_in;
namespace OSI {
  using uint = unsigned int;
  void handle_signal(void (*handler)(int)) noexcept;
  void prevent_multirun(const FName &fname) noexcept;
  char *strtok(char *str, const char *delim, char **saveptr) noexcept;
  void binary2text(char *msg, uint &len, char &ch_last) noexcept;
  bool has_parent() noexcept;
  uint get_pid() noexcept;
  uint get_ppid() noexcept;

  class DirLock {
    std::unique_ptr<class dirlock_impl> _impl;
  public:
    explicit DirLock(const char *dwght) noexcept;
    ~DirLock() noexcept;
  };

  class Semaphore {
    std::unique_ptr<class sem_impl> _impl;
  public:
    static void cleanup() noexcept;
    explicit Semaphore() noexcept;
    ~Semaphore() noexcept;
    void open(const char *name, bool flag_create, uint value) noexcept;
    void close() noexcept;
    void inc() noexcept;
    void dec_wait() noexcept;
    int dec_wait_timeout(ui