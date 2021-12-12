// 2019 Team AobaZero
// This source code is in the public domain.
#include "err.hpp"
#include "iobase.hpp"
#include "nnet-cpu.hpp"
#if ! defined(USE_OPENBLAS) && ! defined(USE_MKL)
using std::pair;
using std::vector;
using namespace ErrAux;
void NNetCPU::reset(uint, const vector<pair<uint, row_t>> &, int) noexcept {
  die(ERR_INT("No CPU BLAS support")); }
#else
#  include <algorithm>
#  include <utility>
#  include <vector>
#  include <cassert>
#  include <cmath>
#  include <omp.h>
#  if defined(USE_MKL)
#    include <mkl.h>
#  elif defined(USE_OPENBLAS)
#    include <cblas.h>
#  endif
using std::copy_n;
using std::fill_n;
using std::forward;
using std::max;
using std::max_element;
using std::move;
using std::pair;
using std::swap;
using std::vector;
using row_t  = std::unique_ptr<float []>;
using ushort = unsigned short;
using namespace ErrAux;

constexpr char msg_bad_wght_dim[] = "bad weight dimension";

static float x1(float x) noexcept { return x; }
static float x2(float x) noexcept { return 2.0f * x; }
static float x3(float x) noexcept { return 3.0f * x; }
static float x4(float x) noexcept { return 4.0f * x; }
static float x6(float x) noexcept { return 6.0f * x; }
static float x8(float x) noexcept { retur