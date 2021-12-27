
// 2019 Team AobaZero
// This source code is in the public domain.
#include "err.hpp"
#include "nnet-ocl.hpp"
#include <string>
#include <utility>
#include <vector>
using std::string;
using std::vector;
using std::pair;
using namespace ErrAux;
#if !defined(USE_OPENCL_AOBA)
std::string NNetOCL::reset(uint, const std::vector<std::pair<uint, row_t>> &,
			   int, bool, bool, bool, bool, const char *)
  noexcept {
  die(ERR_INT("No OpenCL support"));
  return string(""); }
#else
#include "iobase.hpp"
#include "option.hpp"
#include <algorithm>
#include <deque>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>
#include <tuple>
#include <cassert>
#include <chrono>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#define WMMA_ACCUMU16 0

using std::chrono::steady_clock;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::this_thread::sleep_for;
using std::copy_n;
using std::cout;
using std::deque;
using std::endl;
using std::exception;
using std::fill_n;
using std::forward;
using std::ifstream;
using std::lock_guard;
using std::make_tuple;
using std::map;
using std::max;
using std::max_element;
using std::min;
using std::move;
using std::mutex;
using std::ofstream;
using std::set;
using std::sort;
using std::stringstream;
using std::swap;
using std::terminate;
using std::thread;
using std::tie;
using std::to_string;
using std::tuple;
using std::unique_lock;
using std::unique_ptr;
using row_t  = unique_ptr<float []>;
using uint   = unsigned int;
using ushort = unsigned short;
using uchar  = unsigned char;
using namespace ErrAux;
struct CLResWght { OCL::Memory matU, mean, sd_inv; };
static_assert(sizeof(float) == sizeof(int32_t),
	      "sizeof(float) == sizeof(int32_t) * 4U");
static_assert(sizeof(float) == sizeof(uint), "sizeof(float) == sizeof(uint)");
static_assert(sizeof(float) == sizeof(ushort) * 2U,
	      "sizeof(float) == sizeof(ushort) * 2U");

constexpr char msg_bad_wght_dim[]    = "bad weight dimension";
constexpr char msg_opencl_error[]    = "opencl";
constexpr uint tune_sample_size_base = 256U;
constexpr uint size_wrap_wmma        = 32U;
constexpr uint len_kernel            = 3U;
constexpr uint size_kernel           = len_kernel * len_kernel;
constexpr uint len_tile_out          = 3U;
constexpr uint len_tile_in           = 5U;
constexpr uint size_tile_in          = len_tile_in * len_tile_in;
constexpr uint ntile_h               = 3U;
constexpr uint ntile_w               = 3U;
constexpr uint ntile                 = ntile_h * ntile_w;
constexpr uint size_plane_in         = size_tile_in * ntile;
constexpr uint pad                   = 1U;
constexpr uint send_size_ave         = 3000U;
constexpr uint read_size_ave         = 400U;
constexpr uint size_align_local      = 32U;
constexpr float bn_factor            = 1.0f / 999.982f;
constexpr float bn_eps               = 1e-5f;

/*
  static int64_t elapsed_sum = 0;
  static int64_t nelapsed    = 0;
  _queue.finish();
  steady_clock::time_point start = steady_clock::now();
  _queue.finish();
  steady_clock::time_point end = steady_clock::now();
  int64_t elapsed = duration_cast<microseconds>(end - start).count();
  elapsed_sum += elapsed;
  nelapsed    += 1U;
  std::cout << std::endl
	    << elapsed << " " << elapsed_sum / nelapsed << std::endl;
*/

const string code_decode = R"(
__kernel void zero_clear(__global float *p) { p[get_global_id(0)] = 0.0f; }

__constant ushort tbl_ch_fill[] = {
   28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
   73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
  118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,
  163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,
  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
  253,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,
  298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,
  343,344,345,346,347,348,349,350,351,352,353,354,355,356,357,358,359,
  360,361 };

__kernel __attribute__((reqd_work_group_size(81, 1, 1)))
void plane_fill(__global const float *pvalue, __global float *p) {
  uint uplane = get_global_id(0);
  uint ich    = get_global_id(1);
  uint ub     = get_global_id(2);
  uint index  = (tbl_ch_fill[ich]*NB + ub) * SIZE_PLANE;
  p[index + uplane] = pvalue[ub*NCH_FILL + ich]; }

__kernel void set_one(__global const uint *pindex, __global float *p) {
  uint u = pindex[INDEX_BLOCK + get_global_id(0)];
  uint sq = u % SIZE_PLANE;
  uint ch = (u / SIZE_PLANE) % NCH_INPUT;
  uint ub = (u / SIZE_PLANE) / NCH_INPUT;
  p[(ch*NB + ub) * SIZE_PLANE + sq] = 1.0f; }
)";

const string code_compute_policy =
  "#define NCH_OUT    " + to_string(NNAux::nch_out_policy) + "U\n"
  "#define SIZE_PLANE " + to_string(NNAux::size_plane)     + R"(U
__kernel void compute_policy(__global const uint *nnmoves, uint offin,
                             __global const float *weight,
                             __global const float *bias,
                             __global const float *fin,
                             __global float *result) {
  uint gid    = get_global_id(0);
  uint uv     = nnmoves[offin + gid];
  uint ub     = uv / (NCH_OUT * SIZE_PLANE);
  uint nnmove = uv % (NCH_OUT * SIZE_PLANE);
  uint chout  = nnmove / SIZE_PLANE;
  uint sq     = nnmove % SIZE_PLANE;
  float fsum  = bias[chout];
  for (uint chin = 0; chin < NCH_IN; ++chin)
    fsum += weight[chout*NCH_IN + chin] * fin[chin*NB*128U + ub*128U + sq];
  result[NB + gid] = fsum; }
)";

const string code_transform_value2 =
  "#define SIZE_PLANE " + to_string(NNAux::size_plane) + R"(U
__kernel __attribute__((reqd_work_group_size(SIZE_PLANE, 1, 1)))
void transform_value2(__global const float *fin, uint offin,
                      __global float *fout) {
  uint sq = get_global_id(0);
  uint ub = get_global_id(1);
  uint ch = get_global_id(2);
  fout[ch*SIZE_PLANE*NN_OUT + sq*NN_OUT + ub]
    = fin[offin + ch*NBATCH*128U + ub*128U + sq]; }
)";

const string code_BNReLU =
  "#define SIZE_PLANE " + to_string(NNAux::size_plane) + R"(U
__kernel __attribute__((reqd_work_group_size(SIZE_PLANE, 1, 1)))
void compute_BNReLU(__global const float *mean, __global const float *sd_inv,
                    __global const float *fin, __global float *fout) {
  uint sq = get_global_id(0);
  uint ub = get_global_id(1);
  uint ch = get_global_id(2);
  float a = sd_inv[ch];
  float b = mean[ch];
  fout[ch*NB*128U + ub*128U + sq]
    = max(0.0f, a*(fin[ch*NN_IN + ub*SIZE_PLANE + sq] - b)); }
)";

const string code_common =
  "#define SIZE_PLANE "   + to_string(NNAux::size_plane) + "U\n"
  "#define HEIGHT "       + to_string(NNAux::height)     + "U\n"
  "#define WIDTH "        + to_string(NNAux::width)      + "U\n"
  "#define SIZE_ALIGN "   + to_string(size_align_local)  + "U\n"
  "#define LEN_TILE_IN "  + to_string(len_tile_in)       + "U\n"
  "#define LEN_TILE_OUT " + to_string(len_tile_out)      + "U\n"
  "#define NTILE_H "      + to_string(ntile_h)           + "U\n"
  "#define NTILE_W "      + to_string(ntile_w)           + "U\n"
  "#define NTILE "        + to_string(ntile)             + "U\n"
  "#define PAD "          + to_string(pad)               + R"(U
float x1(float x) { return x; }
float x2(float x) { return x + x; }
float x3(float x) { return 3.0f * x; }
float x4(float x) { return 4.0f * x; }
float x6(float x) { return 6.0f * x; }
float x8(float x) { return 8.0f * x; }
float x9(float x) { return 9.0f * x; }
float x16(float x) { return 16.0f * x; }
//float x3(float x) { return x + x + x; }
//float x4(float x) { x += x; return x + x; }
//float x6(float x) { x += x; return x + x + x; }
//float x8(float x) { x += x; x += x; return x + x; }
//nfloat x9(float x) { float y = x + x; y += y; return y + y + x; }
//float x16(float x) { x += x; x += x; x += x; return x + x; }
)";

const string code_compute_matV_child = R"(
#ifdef STORE_HALF
void store(float f, uint off, __global half *p) { vstore_half(f, off, p); }
#else
void store(float f, uint off, __global float *p) { p[off] = f; }
#endif

void compute_matV_child(uint utile, uint dim_n_offset,
                        float md[LEN_TILE_IN][LEN_TILE_IN],
                        __local const float *flin, __global void *matV) {
  uint uh = utile / NTILE_W;
  uint uw = utile % NTILE_W;
  int y0  = uh*LEN_TILE_OUT - PAD;
  int x0  = uw*LEN_TILE_OUT - PAD;
  for (int y = 0; y < LEN_TILE_IN; ++y)
    for (int x = 0; x < LEN_TILE_IN; ++x) {
      if (0 <= y0 + y && y0 + y < HEIGHT && 0 <= x0 + x && x0 + x < WIDTH)
        md[y][x] = flin[(y0 + y)*WIDTH + x0 + x];
      else md[y][x] = 0.0f; }

  store(+ x4(md[0][0]) - x2(md[0][1]) - x4(md[0][2]) + x2(md[0][3])
        - x2(md[1][0]) + x1(md[1][1]) + x2(md[1][2]) - x1(md[1][3])
        - x4(md[2][0]) + x2(md[2][1]) + x4(md[2][2]) - x2(md[2][3])
        + x2(md[3][0]) - x1(md[3][1]) - x2(md[3][2]) + x1(md[3][3]),
        (0U*LEN_TILE_IN + 0U)*NK*NN + dim_n_offset, matV);
  store(- x4(md[1][0]) + x2(md[1][1]) + x4(md[1][2]) - x2(md[1][3])
        - x2(md[2][0]) + x1(md[2][1]) + x2(md[2][2]) - x1(md[2][3])
        + x2(md[3][0]) - x1(md[3][1]) - x2(md[3][2]) + x1(md[3][3]),
        (1U*LEN_TILE_IN + 0U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][0]) - x2(md[1][1]) - x4(md[1][2]) + x2(md[1][3])
        - x6(md[2][0]) + x3(md[2][1]) + x6(md[2][2]) - x3(md[2][3])
        + x2(md[3][0]) - x1(md[3][1]) - x2(md[3][2]) + x1(md[3][3]),
        (2U*LEN_TILE_IN + 0U)*NK*NN + dim_n_offset, matV);
  store(- x2(md[1][0]) + x1(md[1][1]) + x2(md[1][2]) - x1(md[1][3])
        + x2(md[3][0]) - x1(md[3][1]) - x2(md[3][2]) + x1(md[3][3]),
        (3U*LEN_TILE_IN + 0U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][0]) - x2(md[1][1]) - x4(md[1][2]) + x2(md[1][3])
        - x2(md[2][0]) + x1(md[2][1]) + x2(md[2][2]) - x1(md[2][3])
        - x4(md[3][0]) + x2(md[3][1]) + x4(md[3][2]) - x2(md[3][3])
        + x2(md[4][0]) - x1(md[4][1]) - x2(md[4][2]) + x1(md[4][3]),
        (4U*LEN_TILE_IN + 0U)*NK*NN + dim_n_offset, matV);

  store(- x4(md[0][1]) - x2(md[0][2]) + x2(md[0][3])
        + x2(md[1][1]) + x1(md[1][2]) - x1(md[1][3])
        + x4(md[2][1]) + x2(md[2][2]) - x2(md[2][3])
        - x2(md[3][1]) - x1(md[3][2]) + x1(md[3][3]),
        (0U*LEN_TILE_IN + 1U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][1]) + x2(md[1][2]) - x2(md[1][3])
        + x2(md[2][1]) + x1(md[2][2]) - x1(md[2][3])
        - x2(md[3][1]) - x1(md[3][2]) + x1(md[3][3]),
        (1U*LEN_TILE_IN + 1U)*NK*NN + dim_n_offset, matV);
  store(- x4(md[1][1]) - x2(md[1][2]) + x2(md[1][3])
        + x6(md[2][1]) + x3(md[2][2]) - x3(md[2][3])
        - x2(md[3][1]) - x1(md[3][2]) + x1(md[3][3]),
        (2U*LEN_TILE_IN + 1U)*NK*NN + dim_n_offset, matV);
  store(+ x2(md[1][1]) + x1(md[1][2]) - x1(md[1][3])
        - x2(md[3][1]) - x1(md[3][2]) + x1(md[3][3]),
        (3U*LEN_TILE_IN + 1U)*NK*NN + dim_n_offset, matV);
  store(- x4(md[1][1]) - x2(md[1][2]) + x2(md[1][3])
        + x2(md[2][1]) + x1(md[2][2]) - x1(md[2][3])
        + x4(md[3][1]) + x2(md[3][2]) - x2(md[3][3])
        - x2(md[4][1]) - x1(md[4][2]) + x1(md[4][3]),
        (4U*LEN_TILE_IN + 1U)*NK*NN + dim_n_offset, matV);

  store(+ x4(md[0][1]) - x6(md[0][2]) + x2(md[0][3])
        - x2(md[1][1]) + x3(md[1][2]) - x1(md[1][3])
        - x4(md[2][1]) + x6(md[2][2]) - x2(md[2][3])
        + x2(md[3][1]) - x3(md[3][2]) + x1(md[3][3]),
        (0U*LEN_TILE_IN + 2U)*NK*NN + dim_n_offset, matV);
  store(- x4(md[1][1]) + x6(md[1][2]) - x2(md[1][3])
        - x2(md[2][1]) + x3(md[2][2]) - x1(md[2][3])
        + x2(md[3][1]) - x3(md[3][2]) + x1(md[3][3]),
        (1U*LEN_TILE_IN + 2U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][1]) - x6(md[1][2]) + x2(md[1][3])
        - x6(md[2][1]) + x9(md[2][2]) - x3(md[2][3])
        + x2(md[3][1]) - x3(md[3][2]) + x1(md[3][3]),
        (2U*LEN_TILE_IN + 2U)*NK*NN + dim_n_offset, matV);
  store(- x2(md[1][1]) + x3(md[1][2]) - x1(md[1][3])
        + x2(md[3][1]) - x3(md[3][2]) + x1(md[3][3]),
        (3U*LEN_TILE_IN + 2U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][1]) - x6(md[1][2]) + x2(md[1][3])
        - x2(md[2][1]) + x3(md[2][2]) - x1(md[2][3])
        - x4(md[3][1]) + x6(md[3][2]) - x2(md[3][3])
        + x2(md[4][1]) - x3(md[4][2]) + x1(md[4][3]),
        (4U*LEN_TILE_IN + 2U)*NK*NN + dim_n_offset, matV);

  store(- x2(md[0][1]) + x2(md[0][3]) + x1(md[1][1]) - x1(md[1][3])
        + x2(md[2][1]) - x2(md[2][3]) - x1(md[3][1]) + x1(md[3][3]),
        (0U*LEN_TILE_IN + 3U)*NK*NN + dim_n_offset, matV);
  store(+ x2(md[1][1]) - x2(md[1][3]) + x1(md[2][1]) - x1(md[2][3])
        - x1(md[3][1]) + x1(md[3][3]),
        (1U*LEN_TILE_IN + 3U)*NK*NN + dim_n_offset, matV);
  store(- x2(md[1][1]) + x2(md[1][3]) + x3(md[2][1]) - x3(md[2][3])
        - x1(md[3][1]) + x1(md[3][3]),
        (2U*LEN_TILE_IN + 3U)*NK*NN + dim_n_offset, matV);
  store(+ x1(md[1][1]) - x1(md[1][3]) - x1(md[3][1]) + x1(md[3][3]),
        (3U*LEN_TILE_IN + 3U)*NK*NN + dim_n_offset, matV);
  store(- x2(md[1][1]) + x2(md[1][3]) + x1(md[2][1]) - x1(md[2][3])
        + x2(md[3][1]) - x2(md[3][3]) - x1(md[4][1]) + x1(md[4][3]),
        (4U*LEN_TILE_IN + 3U)*NK*NN + dim_n_offset, matV);

  store(+ x4(md[0][1]) - x2(md[0][2]) - x4(md[0][3]) + x2(md[0][4])
        - x2(md[1][1]) + x1(md[1][2]) + x2(md[1][3]) - x1(md[1][4])
        - x4(md[2][1]) + x2(md[2][2]) + x4(md[2][3]) - x2(md[2][4])
        + x2(md[3][1]) - x1(md[3][2]) - x2(md[3][3]) + x1(md[3][4]),
        (0U*LEN_TILE_IN + 4U)*NK*NN + dim_n_offset, matV);
  store(- x4(md[1][1]) + x2(md[1][2]) + x4(md[1][3]) - x2(md[1][4])
        - x2(md[2][1]) + x1(md[2][2]) + x2(md[2][3]) - x1(md[2][4])
        + x2(md[3][1]) - x1(md[3][2]) - x2(md[3][3]) + x1(md[3][4]),
        (1U*LEN_TILE_IN + 4U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][1]) - x6(md[2][1]) + x2(md[3][1])
        - x2(md[1][2]) + x3(md[2][2]) - x1(md[3][2])
        - x4(md[1][3]) + x6(md[2][3]) - x2(md[3][3])
        + x2(md[1][4]) - x3(md[2][4]) + x1(md[3][4]),
        (2U*LEN_TILE_IN + 4U)*NK*NN + dim_n_offset, matV);
  store(- x2(md[1][1]) + x2(md[3][1]) + x1(md[1][2]) - x1(md[3][2])
        + x2(md[1][3]) - x2(md[3][3]) - x1(md[1][4]) + x1(md[3][4]),
        (3U*LEN_TILE_IN + 4U)*NK*NN + dim_n_offset, matV);
  store(+ x4(md[1][1]) - x2(md[1][2]) - x4(md[1][3]) + x2(md[1][4])
        - x2(md[2][1]) + x1(md[2][2]) + x2(md[2][3]) - x1(md[2][4])
        - x4(md[3][1]) + x2(md[3][2]) + x4(md[3][3]) - x2(md[3][4])
        + x2(md[4][1]) - x1(md[4][2]) - x2(md[4][3]) + x1(md[4][4]),
        (4U*LEN_TILE_IN + 4U)*NK*NN + dim_n_offset, matV); }
)";

const string code_compute_matA_child = R"(
#ifdef LOAD_HALF
float load(uint off, __global const half *p) { return vload_half(off, p); }
#else
float load(uint off, __global const float *p) { return p[off]; }
#endif

#ifdef DO_JOIN
void func_BNReLU(__local float *f, uint off, float sd_inv, float mean,
                 float x) {
  f[off] = max(0.0f, sd_inv * (x - mean) + f[off]); }
#else
void func_BNReLU(__local float *f, uint off, float sd_inv, float mean,
                 float x) {
  f[off] = max(0.0f, sd_inv * (x - mean)); }
#endif

void compute_matA_child(uint utile, uint dim_n_offset, float mean,
                        float sd_inv, float mm[LEN_TILE_IN][LEN_TILE_IN],
                        __global const void *matM,
                        __local float *flout) {
  for (uint uh = 0; uh < LEN_TILE_IN; ++uh)
    for (uint uw = 0; uw < LEN_TILE_IN; ++uw)
      mm[uh][uw] = load((uh*LEN_TILE_IN + uw)*NM*NN + dim_n_offset, matM);

  uint uh  = utile / NTILE_W;
  uint uw  = utile % NTILE_W;
  flout   += (uh*WIDTH + uw)*LEN_TILE_OUT;
#ifdef DO_JOIN
  barrier(CLK_LOCAL_MEM_FENCE);
#endif
  func_BNReLU(flout, 0U*WIDTH + 0U, sd_inv, mean,
              + mm[0][0] + mm[0][1] + mm[0][2] + mm[0][3]
              + mm[1][0] + mm[1][1] + mm[1][2] + mm[1][3]
              + mm[2][0] + mm[2][1] + mm[2][2] + mm[2][3]
              + mm[3][0] + mm[3][1] + mm[3][2] + mm[3][3]);
  func_BNReLU(flout, 0U*WIDTH + 1U, sd_inv, mean,
              + mm[0][1] - mm[0][2] + x2(mm[0][3])
              + mm[1][1] - mm[1][2] + x2(mm[1][3])
              + mm[2][1] - mm[2][2] + x2(mm[2][3])
              + mm[3][1] - mm[3][2] + x2(mm[3][3]));
  func_BNReLU(flout, 0U*WIDTH + 2U, sd_inv, mean,
              + mm[0][1] + mm[0][2] + x4(mm[0][3]) + mm[0][4]
              + mm[1][1] + mm[1][2] + x4(mm[1][3]) + mm[1][4]
              + mm[2][1] + mm[2][2] + x4(mm[2][3]) + mm[2][4]
              + mm[3][1] + mm[3][2] + x4(mm[3][3]) + mm[3][4]);
  func_BNReLU(flout, 1U*WIDTH + 0U, sd_inv, mean,
              + mm[1][0] + mm[1][1] + mm[1][2] + mm[1][3]
              - mm[2][0] - mm[2][1] - mm[2][2] - mm[2][3]
              + x2(mm[3][0]) + x2(mm[3][1]) + x2(mm[3][2]) + x2(mm[3][3]));
  func_BNReLU(flout, 1U*WIDTH + 1U, sd_inv, mean,
              + mm[1][1] - mm[1][2] + x2(mm[1][3])
              - mm[2][1] + mm[2][2]
              - x2(mm[2][3]) + x2(mm[3][1]) - x2(mm[3][2]) + x4(mm[3][3]));
  func_BNReLU(flout, 1U*WIDTH + 2U, sd_inv, mean,
              + mm[1][1] + mm[1][2] + x4(mm[1][3]) + mm[1][4]
              - mm[2][1] - mm[2][2] - x4(mm[2][3]) - mm[2][4]
              + x2(mm[3][1]) + x2(mm[3][2]) + x8(mm[3][3]) + x2(mm[3][4]));
  func_BNReLU(flout, 2U*WIDTH + 0U, sd_inv, mean,
              + mm[1][0] + mm[1][1] + mm[1][2] + mm[1][3]
              + mm[2][0] + mm[2][1] + mm[2][2] + mm[2][3]
              + x4(mm[3][0]) + x4(mm[3][1]) + x4(mm[3][2]) + x4(mm[3][3])
              + mm[4][0] + mm[4][1] + mm[4][2] + mm[4][3]);
  func_BNReLU(flout, 2U*WIDTH + 1U, sd_inv, mean,
              + mm[1][1] - mm[1][2] + x2(mm[1][3])
              + mm[2][1] - mm[2][2]
              + x2(mm[2][3]) + x4(mm[3][1]) - x4(mm[3][2]) + x8(mm[3][3])
              + mm[4][1] - mm[4][2] + x2(mm[4][3]));
  func_BNReLU(flout, 2U*WIDTH + 2U, sd_inv, mean,
              + mm[1][1] + mm[1][2] + x4(mm[1][3]) + mm[1][4]
              + mm[2][1] + mm[2][2] + x4(mm[2][3]) + mm[2][4]
              + x4(mm[3][1]) + x4(mm[3][2]) + x16(mm[3][3]) + x4(mm[3][4])
              + mm[4][1] + mm[4][2] + x4(mm[4][3]) + mm[4][4]); }
)";

const string code_compute_matA = R"(
__kernel __attribute__((reqd_work_group_size(NTILE, NBWS, 1)))
void compute_matA_BNReLU(__global const void *matM,
                         __global const float *mean_array,
                         __global const float *sd_inv_array,
                         __global const float *fbypass,
                         __global float *fout) {
  uint utile = get_global_id(0);
  uint ch    = get_global_id(2);
  uint ng    = NB / NBWS;
  uint ug    = get_group_id(1);
  uint ub1   = get_local_id(1);
  __local float flout[NBWS*SIZE_PLANE] __attribute__((aligned(SIZE_ALIGN)));
#ifdef DO_JOIN
  for (uint u = 0; u < NTILE; ++u)
    flout[u*NBWS*NTILE + ub1*NTILE + utile]
      = fbypass[ch*ng*NTILE*BLOCK_SIZE + ug*NTILE*BLOCK_SIZE
                + u*BLOCK_SIZE + ub1*NTILE + utile];
#endif

  uint dim_n_offset = ch*NN + ug*NB_PARTITION + ub1*NTILE + utile;
  float mean   = mean_array[ch];
  float sd_inv = sd_inv_array[ch];
  float M[LEN_TILE_IN][LEN_TILE_IN];
  compute_matA_child(utile, dim_n_offset, mean, sd_inv, M, matM,
                     flout + ub1*SIZE_PLANE);

  barrier(CLK_LOCAL_MEM_FENCE);
  for (uint u = 0; u < NTILE; ++u)
    fout[ch*NN_OUT + (ug*NBWS + ub1)*SIZE_PLANE + u*NTILE + utile]
      = flout[ub1*SIZE_PLANE + u*NTILE + utile]; }
)";

const string code_compute_matV = R"(
__kernel __attribute__((reqd_work_group_size(NTILE, NBWS, 1)))
void compute_matV(__global const float *fin, __global void *matV) {
  uint utile = get_global_id(0);
  uint ch    = get_global_id(2);
  uint ug    = get_group_id(1);
  uint ub1   = get_local_id(1);
  __local float flin[NBWS*SIZE_PLANE] __attribute__((aligned(SIZE_ALIGN)));
  for (uint u = 0; u < 9U; ++u)
    flin[u*NBWS*NTILE + ub1*NTILE + utile]
      = fin[ch*NB*SIZE_PLANE + ug*NBWS*SIZE_PLANE + u*NBWS*NTILE
            + ub1*NTILE + utile];

  uint dim_n_offset = ch*NN + ug*NB_PARTITION + ub1*NTILE + utile;
  float M[LEN_TILE_IN][LEN_TILE_IN];
  barrier(CLK_LOCAL_MEM_FENCE);
  compute_matV_child(utile, dim_n_offset, M, flin + ub1*SIZE_PLANE, matV); }
)";

const string code_compute_matAV = R"(
__kernel __attribute__((reqd_work_group_size(NTILE, NBWS, 1)))
void compute_matAV(__global const void *matM,
                   __global const float *mean_array,
                   __global const float *sd_inv_array,
#if defined(DO_JOIN) || defined(DO_FORK)
                   __global float *matV, __global float *fbypass) {
#else
                   __global float *matV) {
#endif
  uint utile = get_global_id(0);
  uint ch    = get_global_id(2);
  uint ng    = NB / NBWS;
  uint ug    = get_group_id(1);
  uint ub1   = get_local_id(1);
  __local float flout[NBWS*SIZE_PLANE] __attribute__((aligned(SIZE_ALIGN)));
#ifdef DO_JOIN
  for (uint u = 0; u < NTILE; ++u)
    flout[u*NBWS*NTILE + ub1*NTILE + utile]
      = fbypass[ch*ng*NTILE*BLOCK_SIZE + ug*NTILE*BLOCK_SIZE
                + u*BLOCK_SIZE + ub1*NTILE + utile];
#endif

  uint dim_n_offset = ch*NN + ug*NB_PARTITION + ub1*NTILE + utile;
  float mean        = mean_array[ch];
  float sd_inv      = sd_inv_array[ch];
  float M[LEN_TILE_IN][LEN_TILE_IN];
  compute_matA_child(utile, dim_n_offset, mean, sd_inv, M, matM,
                     flout + ub1*SIZE_PLANE);

  barrier(CLK_LOCAL_MEM_FENCE);
#ifdef DO_FORK
  for (uint u = 0; u < NTILE; ++u)
    fbypass[ch*ng*NTILE*BLOCK_SIZE + ug*NTILE*BLOCK_SIZE
            + u*BLOCK_SIZE + ub1*NTILE + utile]
      = flout[u*NBWS*NTILE + ub1*NTILE + utile];
#endif
  compute_matV_child(utile, dim_n_offset, M, flout + ub1*SIZE_PLANE, matV); }
)";


const string code_compute_matM_wmma = R"(
#if WMMA_ACCUMU16
void wmma_mma(uint *mc, __local const uint *ma, __local const uint *mb) {
  asm("{ wmma.mma.sync.aligned.col.row" SGEMM_TDM ".f16.f16\n"
      "    {%0, %1, %2, %3},\n"
      "    {%4, %5, %6, %7, %8, %9, %10, %11},\n"
      "    {%12, %13, %14, %15, %16, %17, %18, %19},\n"
      "    {%0, %1, %2, %3}; }"
      : "+r"(mc[0]), "+r"(mc[1]), "+r"(mc[2]), "+r"(mc[3])
      : "r"(ma[SGEMM_NLM*WRAP_SIZE*0]), "r"(ma[SGEMM_NLM*WRAP_SIZE*1]),
        "r"(ma[SGEMM_NLM*WRAP_SIZE*2]), "r"(ma[SGEMM_NLM*WRAP_SIZE*3]),
        "r"(ma[SGEMM_NLM*WRAP_SIZE*4]), "r"(ma[SGEMM_NLM*WRAP_SIZE*5]),
        "r"(ma[SGEMM_NLM*WRAP_SIZE*6]), "r"(ma[SGEMM_NLM*WRAP_SIZE*7]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*0]), "r"(mb[SGEMM_NLN*WRAP_SIZE*1]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*2]), "r"(mb[SGEMM_NLN*WRAP_SIZE*3]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*4]), "r"(mb[SGEMM_NLN*WRAP_SIZE*5]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*6]), "r"(mb[SGEMM_NLN*WRAP_SIZE*7])); }

void wmma_store(__global void *dest, const uint *src) {
  asm("{  wmma.store.d.sync.aligned.row" SGEMM_TDM ".global.f16\n"
      "     [%4], {%0, %1, %2, %3}, %5; }"
      :: "r"(src[0]), "r"(src[1]), "r"(src[2]), "r"(src[3]),
         "l"(dest), "r"(NN) : "memory"); }

#  define TYPE_OUT  ushort
#  define ACCU_SIZE 4U
#else
void wmma_mma(uint *mc, __local const uint *ma, __local const uint *mb) {
  asm("{ wmma.mma.sync.aligned.col.row" SGEMM_TDM ".f32.f32\n"
      "    {%0, %1, %2, %3, %4, %5, %6, %7},\n"
      "    {%8, %9, %10, %11, %12, %13, %14, %15},\n"
      "    {%16, %17, %18, %19, %20, %21, %22, %23},\n"
      "    {%0, %1, %2, %3, %4, %5, %6, %7}; }"
      : "+r"(mc[0]), "+r"(mc[1]), "+r"(mc[2]), "+r"(mc[3]),
        "+r"(mc[4]), "+r"(mc[5]), "+r"(mc[6]), "+r"(mc[7])
      : "r"(ma[SGEMM_NLM*WRAP_SIZE*0]), "r"(ma[SGEMM_NLM*WRAP_SIZE*1]),
        "r"(ma[SGEMM_NLM*WRAP_SIZE*2]), "r"(ma[SGEMM_NLM*WRAP_SIZE*3]),
        "r"(ma[SGEMM_NLM*WRAP_SIZE*4]), "r"(ma[SGEMM_NLM*WRAP_SIZE*5]),
        "r"(ma[SGEMM_NLM*WRAP_SIZE*6]), "r"(ma[SGEMM_NLM*WRAP_SIZE*7]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*0]), "r"(mb[SGEMM_NLN*WRAP_SIZE*1]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*2]), "r"(mb[SGEMM_NLN*WRAP_SIZE*3]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*4]), "r"(mb[SGEMM_NLN*WRAP_SIZE*5]),
        "r"(mb[SGEMM_NLN*WRAP_SIZE*6]), "r"(mb[SGEMM_NLN*WRAP_SIZE*7])); }

void wmma_store(__global void *dest, const uint *src) {
  asm("{  wmma.store.d.sync.aligned.row" SGEMM_TDM ".global.f32\n"
      "     [%8], {%0, %1, %2, %3, %4, %5, %6, %7}, %9; }"
      :: "r"(src[0]), "r"(src[1]), "r"(src[2]), "r"(src[3]),
         "r"(src[4]), "r"(src[5]), "r"(src[6]), "r"(src[7]),
         "l"(dest), "r"(NN) : "memory"); }

#  define TYPE_OUT  uint
#  define ACCU_SIZE 8U
#endif

void wmma_load_a(__local uint *dest, __global const uint *src) {
  asm("{ wmma.load.a.sync.aligned.col" SGEMM_TDM ".global.f16\n"
      "    {%0, %1, %2, %3, %4, %5, %6, %7}, [%8], %9; }"
      : "=r"(dest[SGEMM_NLM*WRAP_SIZE*0]), "=r"(dest[SGEMM_NLM*WRAP_SIZE*1]),
        "=r"(dest[SGEMM_NLM*WRAP_SIZE*2]), "=r"(dest[SGEMM_NLM*WRAP_SIZE*3]),
        "=r"(dest[SGEMM_NLM*WRAP_SIZE*4]), "=r"(dest[SGEMM_NLM*WRAP_SIZE*5]),
        "=r"(dest[SGEMM_NLM*WRAP_SIZE*6]), "=r"(dest[SGEMM_NLM*WRAP_SIZE*7])
      : "l"(src), "r"(NM) : "memory"); }

void wmma_load_b(__local uint *dest, __global const uint *src) {
  asm("{ wmma.load.b.sync.aligned.row" SGEMM_TDM ".global.f16\n"
      "    {%0, %1, %2, %3, %4, %5, %6, %7}, [%8], %9; }"
      : "=r"(dest[SGEMM_NLN*WRAP_SIZE*0]), "=r"(dest[SGEMM_NLN*WRAP_SIZE*1]),
        "=r"(dest[SGEMM_NLN*WRAP_SIZE*2]), "=r"(dest[SGEMM_NLN*WRAP_SIZE*3]),
        "=r"(dest[SGEMM_NLN*WRAP_SIZE*4]), "=r"(dest[SGEMM_NLN*WRAP_SIZE*5]),
        "=r"(dest[SGEMM_NLN*WRAP_SIZE*6]), "=r"(dest[SGEMM_NLN*WRAP_SIZE*7])
      : "l"(src), "r"(NN) : "memory"); }

#if (SGEMM_NLM == 1U && SGEMM_NLN == 1U)
__kernel __attribute__((reqd_work_group_size(WRAP_SIZE, 1, 1)))
void compute_matM(__global const uint *gA, __global const uint *gB,
                  __global TYPE_OUT *gC) {
  uint ub  = get_global_id(2);
  uint ugm = get_group_id(1);
  uint ugn = get_group_id(0);
  gA += ub*(OFFA/2U) + ugm*(SGEMM_NPTM/2U);
  gB += ub*(OFFB/2U) + ugn*(SGEMM_NPTN/2U);
  gC += ub*OFFC + ugm*SGEMM_NPTM*NN + ugn*SGEMM_NPTN;

  uint pD[SGEMM_NPM][SGEMM_NPN][ACCU_SIZE];
  for (uint upm = 0; upm < SGEMM_NPM; ++upm)
    for (uint upn = 0; upn < SGEMM_NPN; ++upn)
      for (uint u = 0; u < ACCU_SIZE; ++u) pD[upm][upn][u] = 0;

  __local uint la[SGEMM_NPK][SGEMM_NPM][8][WRAP_SIZE]
               __attribute__((aligned(32)));
  __local uint lb[SGEMM_NPK][SGEMM_NPN][8][WRAP_SIZE]
               __attribute__((aligned(32)));

  uint ulane = get_local_id(0);
  uint ngk   = NK / SGEMM_NPTK;
  for (uint ugk = 0; ugk < ngk; ++ugk) {

    for (uint upk = 0; upk < SGEMM_NPK; ++upk)
      for (uint upm = 0; upm < SGEMM_NPM; ++upm)
        wmma_load_a(&la[upk][upm][0][ulane],
                    gA + upk*SGEMM_NTK*(NM/2U) + upm*(SGEMM_NTM/2U));

    for (uint upk = 0; upk < SGEMM_NPK; ++upk)
      for (uint upn = 0; upn < SGEMM_NPN; ++upn)
        wmma_load_b(&lb[upk][upn][0][ulane],
                    gB + upk*SGEMM_NTK*(NN/2U) + upn*(SGEMM_NTN/2U));

    for (uint upk = 0; upk < SGEMM_NPK; ++upk)
      for (uint upm = 0; upm < SGEMM_NPM; ++upm)
        for (uint upn = 0; upn < SGEMM_NPN; ++upn)
           wmma_mma(&pD[upm][upn][0],
                   &la[upk][upm][0][ulane], &lb[upk][upn][0][ulane]);

    gA += SGEMM_NPTK*(NM/2U);
    gB += SGEMM_NPTK*(NN/2U); }

  for (uint upm = 0; upm < SGEMM_NPM; ++upm)
    for (uint upn = 0; upn < SGEMM_NPN; ++upn)
      wmma_store(gC + upm*SGEMM_NTM*NN + upn*SGEMM_NTN, &pD[upm][upn][0]); }

#else

__kernel __attribute__((reqd_work_group_size(SGEMM_NLN*WRAP_SIZE,
                                             SGEMM_NLM,1)))
void compute_matM(__global const uint *gA, __global const uint *gB,
                  __global TYPE_OUT *gC) {
  uint ub  = get_global_id(2);
  uint ugm = get_group_id(1);
  uint ugn = get_group_id(0);
  gA += ub*(OFFA/2U) + ugm*(SGEMM_NLPTM/2U);
  gB += ub*(OFFB/2U) + ugn*(SGEMM_NLPTN/2U);
  gC += ub*OFFC + ugm*SGEMM_NLPTM*NN + ugn*SGEMM_NLPTN;

  uint pD[SGEMM_NPM][SGEMM_NPN][ACCU_SIZE];
  for (uint upm = 0; upm < SGEMM_NPM; ++upm)
    for (uint upn = 0; upn < SGEMM_NPN; ++upn)
      for (uint u = 0; u < ACCU_SIZE; ++u) pD[upm][upn][u] = 0;

  __local uint la[SGEMM_NPK][SGEMM_NPM][8][SGEMM_NLM][WRAP_SIZE]
               __attribute__((aligned(32)));
  __local uint lb[SGEMM_NPK][SGEMM_NPN][8][SGEMM_NLN][WRAP_SIZE]
               __attribute__((aligned(32)));

  uint ulm   = get_local_id(1);
  uint uln   = get_local_id(0) / WRAP_SIZE;
  uint ulane = get_local_id(0) % WRAP_SIZE;
  uint ngk   = NK / SGEMM_NPTK;
  uint ulm_r = (ulm*SGEMM_NLN + uln) % SGEMM_NLM;
  uint uln_r = (ulm*SGEMM_NLN + uln) / SGEMM_NLM;
  for (uint ugk = 0; ugk < ngk; ++ugk) {

    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint u = uln_r; u < SGEMM_NPM*SGEMM_NPK; u += SGEMM_NLN) {
      uint upm = u % SGEMM_NPM;
      uint upk = u / SGEMM_NPM;
      wmma_load_a(&la[upk][upm][0][ulm_r][ulane],
                  gA + upk*SGEMM_NTK*(NM/2U)
                     + upm*(SGEMM_NLTM/2U) + ulm_r*(SGEMM_NTM/2U)); }
    for (uint u = ulm; u < SGEMM_NPN*SGEMM_NPK; u += SGEMM_NLM) {
      uint upn = u % SGEMM_NPN;
      uint upk = u / SGEMM_NPN;
      wmma_load_b(&lb[upk][upn][0][uln][ulane],
                  gB + upk*SGEMM_NTK*(NN/2U)
                     + upn*(SGEMM_NLTN/2U) + uln*(SGEMM_NTN/2U)); }
    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint upk = 0; upk < SGEMM_NPK; ++upk)
      for (uint upm = 0; upm < SGEMM_NPM; ++upm)
        for (uint upn = 0; upn < SGEMM_NPN; ++upn)
          wmma_mma(&pD[upm][upn][0],
                   &la[upk][upm][0][ulm][ulane], &lb[upk][upn][0][uln][ulane]);
    gA += SGEMM_NPTK*(NM/2U);
    gB += SGEMM_NPTK*(NN/2U); }

  gC += ulm*SGEMM_NTM*NN + uln*SGEMM_NTN;
  for (uint upm = 0; upm < SGEMM_NPM; ++upm)
    for (uint upn = 0; upn < SGEMM_NPN; ++upn)
      wmma_store(gC + upm*SGEMM_NLTM*NN + upn*SGEMM_NLTN, &pD[upm][upn][0]); }
#endif
)";

const string code_sgemm_child_half = R"(
void sgemm_child(__global const half *gA, __global const half *gB,
                 __global float *gC, uint ulm, uint uln,
                 __local float lA[SGEMM_NPK][SGEMM_NLM*SGEMM_NPM],
                 __local float lB[SGEMM_NPK][SGEMM_NLN*SGEMM_NPN]) {
  float pC[SGEMM_NPM][SGEMM_NPN];
  float pB[SGEMM_NPN];
  float pA;
  for (uint upm = 0; upm < SGEMM_NPM; ++upm)
    for (uint upn = 0; upn < SGEMM_NPN; ++upn) pC[upm][upn] = 0.0f;

  uint ul = ulm*SGEMM_NLN + uln;
  uint ulA1 = (ul % ((SGEMM_NLM*SGEMM_NPM)/2U))*2U;
  uint ulA2 = ul / ((SGEMM_NLM*SGEMM_NPM)/2U);
  uint ulB1 = (ul % ((SGEMM_NLN*SGEMM_NPN)/2U))*2U;
  uint ulB2 = ul / ((SGEMM_NLN*SGEMM_NPN)/2U);
  for (uint ugk = 0; ugk < NK / SGEMM_NPK; ++ugk) {
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint u = 0; u < SGEMM_NPK; u += (2U*SGEMM_NLN) / SGEMM_NPM) {
      lA[u + ulA2][ulA1]      = vload_half((u + ulA2)*NM + ulA1, gA);
      lA[u + ulA2][ulA1 + 1U] = vload_half((u + ulA2)*NM + ulA1 + 1U, gA); }
    for (uint u = 0; u < SGEMM_NPK; u += (2U*SGEMM_NLM) / SGEMM_NPN) {
      lB[u + ulB2][ulB1]      = vload_half((u + ulB2)*NN + ulB1, gB);
      lB[u + ulB2][ulB1 + 1U] = vload_half((u + ulB2)*NN + ulB1 + 1U, gB); }
    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint upk = 0; upk < SGEMM_NPK; ++upk) {
      for (uint upn = 0; upn < SGEMM_NPN; ++upn)
        pB[upn] = lB[upk][upn*SGEMM_NLN + uln];
      for (uint upm = 0; upm < SGEMM_NPM; ++upm) {
        pA = lA[upk][ulm*SGEMM_NPM + upm];
        for (uint upn = 0; upn < SGEMM_NPN; ++upn)
          pC[upm][upn] += pA * pB[upn]; } }
    gA += SGEMM_NPK*NM;
    gB += SGEMM_NPK*NN; }

  for (uint upm = 0; upm < SGEMM_NPM; ++upm)
    for (uint upn = 0; upn < SGEMM_NPN; ++upn)
      gC[ulm*SGEMM_NPM*NN + upm*NN + upn*SGEMM_NLN + uln] = pC[upm][upn]; }