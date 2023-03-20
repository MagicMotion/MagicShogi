// 2019 Team AobaZero
// This source code is in the public domain.
#ifndef SHOGI_H
#define SHOGI_H

#include <stdio.h>
#include "bitop.h"
#include "param.h"
#include <string>

#if defined(_WIN32)

#  include <Winsock2.h>
#  define CONV              __fastcall
#  define SCKT_NULL         INVALID_SOCKET
typedef SOCKET sckt_t;

#else

#  include <pthread.h>
#  include <sys/times.h>
#  define CONV
#  define SCKT_NULL         -1
#  define SOCKET_ERROR      -1
typedef int sckt_t;

#endif

/* Microsoft C/C++ on x86 and x86-64 */
#if defined(_MSC_VER)

#  define _CRT_DISABLE_PERFCRIT_LOCKS
#ifndef UINT64_MAX
#  define UINT64_MAX    ULLONG_MAX
#endif
#  define PRIu64        "I64u"
#  define PRIx64        "I64x"
#ifndef UINT64_C
#  define UINT64_C(u)  ( u )
#endif

#  define restrict      __restrict
#  define strtok_r      strtok_s
#  define read          _read
#  define strncpy( dst, src, len ) strncpy_s( dst, len, src, _TRUNCATE )
#  define snprintf( buf, size, fmt, ... )   \
          _snprintf_s( buf, size, _TRUNCATE, fmt, __VA_ARGS__ )
#  define vsnprintf( buf, size, fmt, list ) \
          _vsnprintf_s( buf, size, _TRUNCATE, fmt, list )
typedef unsigned __int64 uint64_t;
typedef volatile long lock_t;

/* GNU C and Intel C/C++ on x86 and x86-64 */
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

#  include <inttypes.h>
#  define restrict __restrict
typedef volatile int lock_t;

/* other targets. */
#else

#  include <inttypes.h>
typedef struct { unsigned int p[3]; } bitboard_t;
typedef pthread_mutex_t lock_t;
extern unsigned char aifirst_one[512];
extern unsigned char ailast_one[512];

#endif

#define BK_TINY
/*
  #define BK_SMALL
  #define BK_ULTRA_NARROW
  #define BK_COM
  #define NO_STDOUT
  #define DBG_EASY
*/

//#if defined(CSASHOGI)
#if defined(CSASHOGI) || (defined(_WIN32) && defined(YSS_ZERO))
#  define NO_STDOUT
#  if ! defined(WIN32_PIPE)
#    define WIN32_PIPE
#  endif
#endif

#if defined(TLP)
#  define SHARE volatile
#  define SEARCH_ABORT ( root_abort || ptree->tlp_abort )
#else
#  define SHARE
#  define SEARCH_ABORT root_abort
#endif

#define NUM_UNMAKE              2
#define QUIES_PLY_LIMIT         7
#define SHELL_H_LEN             7
#define MAX_ANSWER              8
#define PLY_INC                 8
#define PLY_MAX                 128
#define RAND_N                  624
#define TC_NMOVE                35U
#define SEC_MARGIN              15U
#define SEC_KEEP_ALIVE          180U
#define TIME_RESPONSE           200U
#define RESIGN_THRESHOLD       ( ( MT_CAP_DRAGON * 5 ) /  8 )
//#define BNZ_VER                 "0.1"
//#define BNZ_VER                 "1"	// 20190311
//#define BNZ_VER                 "2"	// 20190322
//#define BNZ_VER                 "3"	// 20190323
//#define BNZ_VER                 "4"	// 20190323
//#define BNZ_VER                 "5"	// 20190324
//#define BNZ_VER                 "6"	// 20190419
//#define BNZ_VER                 "7"	// 20190420
//#define BNZ_VER                 "8"	// 20190430
//#define BNZ_VER                 "9"	// 20190527
//#define BNZ_VER                 "10"	// 20190708
//#define BNZ_VER                 "11"	// 20190709
//#define BNZ_VER                 "12"	// 20201013
//#define BNZ_VER                 "13"	// 20201023 resign 10%
//#define BNZ_VER                 "14"	// 20201108 declare win bug fix. fAutoResign
//#define BNZ_VER                 "15"	// 20201207 sente 1 mate bug fix
//#define BNZ_VER                 "16"	// 20210528 komaochi, mate3
//#define BNZ_VER                 "17"	// 20210607 bug fix. does not tend to move 1 ply mate.
//#define BNZ_VER                 "18"	// 20210608 OpenCL selfcheck off temporarily
//#define BNZ_VER                 "19"	// 20210622 Windows binary
//#define BNZ_VER                 "20"	// 20210628 softmax temperature > 1.0 is adjusted, even if moves <= 30.
//#define BNZ_VER                 "21"	// 20210919 LCB, kld_gain, reset_root_visit
//#define BNZ_VER                 "22"	// 20210920 fDiffRootVisit
//#define BNZ_VER                 "23"	// 20210920 no change. autousi uses kldgain.
//#define BNZ_VER                 "24"	// 20210922 sum_games bug fix.
//#define BNZ_VER                 "25"	// 20211102 mtemp 1.3 (autousi)
//#define BNZ_VER                 "26"	// 20211204 final with weight w1250. -name
#define BNZ_VER                 "27"	// 20240301 OpenCL bug fixed for latest NVIDIA driver update
#define BNZ_NAME                "AobaKomaochi"

#define REP_MAX_PLY             32
#if defined(YSS_ZERO)
#define REP_HIST_LEN            (1024+PLY_MAX)
#else
#define REP_HIST_LEN    