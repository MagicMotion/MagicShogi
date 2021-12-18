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

static row_t gen_head1_weight(uint nout1, uint nout2, uint nin,
			      const float *w1, const float *w2) noexcept {
  row_t row(new float [(nout1 + nout2) * nin * NNAux::size_plane]);
  copy_n(w1, nout1 * nin, row.get());
  copy_n(w2, nout2 * nin, row.get() + nout1 * nin);
  return row; }

static row_t gen_head1_mean(uint nch1, uint nch2,
			    const float *bias1, const float *mean1,
			    const float *bias2, const float *mean2) noexcept {
  row_t row(new float [nch1 + nch2]);
  for (uint ch = 0; ch < nch1; ++ch)
    row[ch] = mean1[ch] * bn_factor - bias1[ch];
  for (uint ch = 0; ch < nch2; ++ch)
    row[nch1 + ch] = mean2[ch] * bn_factor - bias2[ch];
  return row; }

static row_t gen_head1_sd_inv(uint nch1, uint nch2,
			      const float *sd1, const float *sd2) noexcept {
  row_t row(new float [nch1 + nch2]);
  for (uint ch = 0; ch < nch1; ++ch)
    row[ch] = 1.0f / std::sqrt(sd1[ch] * bn_factor + bn_eps);
  for (uint ch = 0; ch < nch2; ++ch)
    row[nch1 + ch] = 1.0f / std::sqrt(sd2[ch] * bn_factor + bn_eps);
  return row; }

void NNetCPU::load(const vector<pair<uint, row_t>> &wght) noexcept {
  constexpr uint nrow_input = 4U;
  constexpr uint nrow_head  = 14U;
  uint nrow = static_cast<uint>(wght.size());
  if (nrow < nrow_input + nrow_head) die(ERR_INT(msg_bad_wght_dim));
  
  uint nrow_res = nrow - nrow_input - nrow_head;
  if (nrow_res % 8U) die(ERR_INT(msg_bad_wght_dim));
  
  // load body part
  // for index in 0 ... nrow_res / 4
  // (4*index + 0) weight [nout][nin][size_plane]
  // (4*index + 1) bias [nout]
  // (4*index + 2) mean value [nout]
  // (4*index + 3) standard deviation [nout]
  _resnet_nout      = wght[1].first;
  _conv3x3_nin_max  = max(NNAux::nch_input, _resnet_nout);
  _conv3x3_nout_max = _resnet_nout;
  _maxsize_out      = _resnet_nout * NNAux::size_plane;
  row_t matU, mean, sd_inv;
  uint nin = NNAux::nch_input;
  uint index = 0;
  for (uint u = 0; u < 1U + nrow_res / 4U; ++u) {
    if (wght[index].first != _resnet_nout * nin * size_kernel
	|| wght[index + 1U].first != _resnet_nout
	|| wght[index + 2U].first != _resnet_nout
	|| wght[index + 3U].first != _resnet_nout)
      die(ERR_INT(msg_bad_wght_dim));
    matU = gen_matU(_resnet_nout, nin, wght[index].second.get());
    mean = gen_mean(_resnet_nout, wght[index + 1U].second.get(),
		    wght[index + 2U].second.get());
    sd_inv = gen_sd_inv(_resnet_nout, wght[index + 3U].second.get());
    _reswghts.push_back({move(matU), move(mean), move(sd_inv)});
    nin = _resnet_nout;
    index += 4U; }

  // load head1 part (conv 1x1)
  // index + 0: weight (policy part)
  // index + 1: bias (policy part)
  // index + 2: mean value (policy part)
  // index + 3: standard deviation (policy part)
  // index + 6: weight (value part)
  // index + 7: bias (value part)
  // index + 8: mean value (value part)
  // index + 9: standard deviation (value part)
  _policy1_nout = wght[index + 1U].first;
  _value1_nout  = wght[index + 7U].first;
  nin           = _resnet_nout;
  _head1_nout   = _policy1_nout + _value1_nout;
  _maxsize_out  = max(_maxsize_out, _head1_nout * NNAux::size_plane);
  if (wght[index].first != _policy1_nout * nin
      || wght[index + 2U].first != _policy1_nout
      || wght[index + 3U].first != _policy1_nout
      || wght[index + 6U].first != _value1_nout * nin
      || wght[index + 8U].first != _value1_nout
      || wght[index + 9U].first != _value1_nout)
    die(ERR_INT(msg_bad_wght_dim));
  _head1_weight = gen_head1_weight(_policy1_nout, _value1_nout, nin,
				   wght[index + 0U].second.get(),
				   wght[index + 6U].second.get());
  _head1_mean = gen_head1_mean(_policy1_nout, _value1_nout,
			       wght[index + 1U].second.get(),
			       wght[index + 2U].second.get(),
			       wght[index + 7U].second.get(),
			       wght[index + 8U].second.get());
  _head1_sd_inv = gen_head1_sd_inv(_policy1_nout, _value1_nout,
				   wght[index + 3U].second.get(),
				   wght[index + 9U].second.get());

  // load policy2 part (conv 1x1)
  // index + 4: weight
  // index + 5: bias
  uint nout    = NNAux::nch_out_policy;
  _policy2_nin = _policy1_nout;
  _maxsize_out = max(_maxsize_out, nout * NNAux::size_plane);
  if (wght[index + 4U].first != nout * _policy2_nin
      || wght[index + 5U].first != nout) die(ERR_INT(msg_bad_wght_dim));
  _policy2_weight.reset(new float [nout * _policy2_nin]);
  _policy2_bias.reset(new float [nout]);
  copy_n(wght[index + 4U].second.get(), nout * _policy2_nin,
	 _policy2_weight.get());
  copy_n(wght[index + 5U].second.get(), nout, _policy2_bias.get());

  // load value2 part (fc)
  // index + 10: weight
  // index + 11: bias
  _value2_nin  = _value1_nout * NNAux::size_plane;
  _value2_nout = wght[index + 11U].first;
  if (wght[index + 10U].first != _value2_nout * _value2_nin)
    die(ERR_INT(msg_bad_wght_dim));
  _value2_weight.reset(new float [_value2_nout * _value2_nin]);
  _value2_bias.reset(new float [_value2_nout]);
  copy_n(wght[index + 10U].second.get(), _value2_nout * _value2_nin,
	 _value2_weight.get());
  copy_n(wght[index + 11U].second.get(), _value2_nout, _value2_bias.get());

  // load value3 part (fc)
  // index + 12: weight
  // index + 13: bias
  _value3_nin  = _value2_nout;
  _value3_nout = wght[index + 13U].first;
  if (wght[index + 12U].first != _value3_nout * _value3_nin
      || _value3_nout != 1) die(ERR_INT(msg_bad_wght_dim));
  _value3_weight.reset(new float [_value3_nout * _value3_nin]);
  _value3_bias.reset(new float [_value3_nout]);
  copy_n(wght[index + 12U].second.get(), _value3_nout * _value3_nin,
	 _value3_weight.ge