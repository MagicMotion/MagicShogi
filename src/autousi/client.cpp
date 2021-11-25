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
using st