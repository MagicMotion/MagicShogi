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
static float x8(float x) noexcept { return 8.0f * x; }
static float x9(float x) noexcept { return 9.0f * x; }
static float x16(float x) noexcept { return 16.0f * x; }

constexpr uint len_kernel    = 3U;
constexpr uint size_kernel   = len_kernel * len_kernel;
constexpr uint len_tile_out  = 3U;
constexpr uint len_tile_in   = 5U;
constexpr uint size_tile_in  = len_tile_in * len_tile_in;
constexpr uint ntile_h       = 3U;
constexpr uint ntile_w       = 3U;
constexpr uint ntile         = ntile_h * ntile_w;
constexpr uint size_plane_in = size_tile_in * ntile;
constexpr uint pad           = 1U;
constexpr float bn_factor    = 1.0f / 999.982f;
constexpr float bn_eps       = 1e-5f;

void NNetCPU::reset(uint maxsize_batch, const vector<pair<uint, row_t>> &wght,
		    int thread_num) noexcept {
  assert(0 < maxsize_batch);
  if (thread_num == -1) thread_num = omp_get_max_threads();
  _thread_num = thread_num;

#if defined(USE_MKL)
#  if ! defined(__INTEL_COMPILER) && defined(__linux__) && ! defined(__MIC__)
  if (mkl_set_threading_layer(MKL_THREADING_GNU) < 0)
    die(ERR_INT("mkl_set_interface_layer() failed."));
#  endif
  mkl_set_num_threads(thread_num);
#endif

  _maxsize_batch = maxsize_batch;
  load(wght);
  
  _matM.reset(new float [_maxsize_batch * _conv3x3_nout_max * size_plane_in]);
  _matV.reset(new float [_maxsize_batch * _conv3x3_nin_max  * size_plane_in]);
  for (auto &f : _fslot) f.reset(new float [_maxsize_batch * _maxsize_out]); }

// in:  weight[nout][nin][size_kernel]
// ret: matrix_U[size_tile_in][nout][nin]
static row_t gen_matU(uint nout, uint nin, const float *weight) {
  assert(0 < nout && 0 < nin && weight);
  constexpr float matG[5][3] = { +1.0f / 2.0f, +0.0f,        +0.0f,
				 -1.0f / 2.0f, -1.0f / 2.0f, -1.0f / 2.0f,
				 -1.0f / 6.0f, +1.0f / 6.0f, -1.0f / 6.0f,
				 +1.0f / 6.0f, +1.0f / 3.0f, +2.0f / 3.0f,
				 +0.0f,        +0.0f,        +1.0f };

  row_t matU(new float [size_tile_in * nout * nin]);
  const uint nout_in = nout * nin;
  for (uint ch_io = 0; ch_io < nout_in; ++ch_io) {
    const float *mg = weight + ch_io * size_kernel;
    for (uint uh = 0; uh < len_tile_in; ++uh)
      for (uint uw = 0; uw < len_tile_in; ++uw) {
	float fsum = 0.0;
	for (uint u1 = 0; u1 < len_kernel; ++u1)
	  for (uint u2 = 0; u2 < len_kernel; ++u2)
	    fsum += matG[uh][u1] * matG[uw][u2] * mg[u1 * len_kernel + u2];
	matU[(uh * len_tile_in + uw) * nout_in + ch_io] = fsum; } }
  
  return matU; }

static row_t gen_mean(uint nch, const float *bias, const float *mean)
  noexcept {
  row_t row(new float [nch]);
  for (uint ch = 0; ch < nch; ++ch) row[ch] = mean[ch] * bn_factor - bias[ch];
  return row; }

static row_t gen_sd_inv(uint nch, const float *sd) noexcept {
  row_t row(new float [nch]);
  for (uint ch = 0; ch < nch; ++ch)
    row[ch] = 1.0f / std::sqrt(sd[ch] * bn_factor + bn_eps);
  return row; }

static row_t gen_head1