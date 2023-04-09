﻿// 2019 Team AobaZero
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
#define REP_HIST_LEN            256
#endif

#define EHASH_MASK              0x3fffffU      /* occupies 32MB */
#define MATE3_MASK              0x07ffffU      /* occupies 4MB */

#define HIST_SIZE               0x4000U
#define HIST_INVALID            0xffffU
#define HIST_MAX                0x8000U

#define REJEC_MASK              0x0ffffU
#define REJEC_MIN_DEPTH        ( ( PLY_INC  * 5 ) )

#define EXT_RECAP1             ( ( PLY_INC  * 1 ) /  4 )
#define EXT_RECAP2             ( ( PLY_INC  * 2 ) /  4 )
#define EXT_ONEREP             ( ( PLY_INC  * 2 ) /  4 )
#define EXT_CHECK              ( ( PLY_INC  * 4 ) /  4 )

#define EFUTIL_MG1             ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define EFUTIL_MG2             ( ( MT_CAP_DRAGON * 2 ) /  8 )

#define FMG_MG                 ( ( MT_CAP_DRAGON * 2 ) / 16 )
#define FMG_MG_KING            ( ( MT_CAP_DRAGON * 3 ) / 16 )
#define FMG_MG_MT              ( ( MT_CAP_DRAGON * 8 ) / 16 )
#define FMG_MISC               ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_CAP                ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_DROP               ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_MT                 ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_MISC_KING          ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_CAP_KING           ( ( MT_CAP_DRAGON * 2 ) /  8 )

#define FV_WINDOW               256
#define FV_SCALE                32
#define FV_PENALTY             ( 0.2 / (double)FV_SCALE )

#define MPV_MAX_PV              16

#define TLP_MAX_THREADS         12
#define TLP_NUM_WORK           ( TLP_MAX_THREADS * 8 )

#define TIME_CHECK_MIN_NODE     10000U
#define TIME_CHECK_MAX_NODE     100000U

#define SIZE_FILENAME           256
#define SIZE_PLAYERNAME         256
#define SIZE_MESSAGE            512
#define SIZE_CSALINE            512

#if defined(USI)
#  define SIZE_CMDLINE          ( 1024 * 16 )
#  define SIZE_CMDBUFFER        ( 1024 * 16 )
#else
#  define SIZE_CMDLINE          512
#  define SIZE_CMDBUFFER        512
#endif

#define IsMove(move)           ( (move) & 0xffffffU )
#define MOVE_NA                 0x00000000U
#define MOVE_PASS               0x01000000U
#define MOVE_PONDER_FAILED      0xfe000000U
#define MOVE_RESIGN             0xff000000U
#define MOVE_CHK_SET            0x80000000U
#define MOVE_CHK_CLEAR          0x40000000U

#define MAX_LEGAL_MOVES         700
#define MAX_LEGAL_EVASION       256
#define MOVE_LIST_LEN           16384

#define MAX_SIZE_SECTION        0xffff
#define NUM_SECTION             0x4000

#define MATERIAL            (ptree->posi.material)
#define HAND_B              (ptree->posi.hand_black)
#define HAND_W              (ptree->posi.hand_white)

#define BB_BOCCUPY          (ptree->posi.b_occupied)
#define BB_BTGOLD           (ptree->posi.b_tgold)
#define BB_B_HDK            (ptree->posi.b_hdk)
#define BB_B_BH             (ptree->posi.b_bh)
#define BB_B_RD             (ptree->posi.b_rd)
#define BB_BPAWN_ATK        (ptree->posi.b_pawn_attacks)
#define BB_BPAWN            (ptree->posi.b_pawn)
#define BB_BLANCE           (ptree->posi.b_lance)
#define BB_BKNIGHT          (ptree->posi.b_knight)
#define BB_BSILVER          (ptree->posi.b_silver)
#define BB_BGOLD            (ptree->posi.b_gold)
#define BB_BBISHOP          (ptree->posi.b_bishop)
#define BB_BROOK            (ptree->posi.b_rook)
#define BB_BKING            (abb_mask[SQ_BKING])
#define BB_BPRO_PAWN        (ptree->posi.b_pro_pawn)
#define BB_BPRO_LANCE       (ptree->posi.b_pro_lance)
#define BB_BPRO_KNIGHT      (ptree->posi.b_pro_knight)
#define BB_BPRO_SILVER      (ptree->posi.b_pro_silver)
#define BB_BHORSE           (ptree->posi.b_horse)
#define BB_BDRAGON          (ptree->posi.b_dragon)

#define BB_WOCCUPY          (ptree->posi.w_occupied)
#define BB_WTGOLD           (ptree->posi.w_tgold)
#define BB_W_HDK            (ptree->posi.w_hdk)
#define BB_W_BH             (ptree->posi.w_bh)
#define BB_W_RD             (ptree->posi.w_rd)
#define BB_WPAWN_ATK        (ptree->posi.w_pawn_attacks)
#define BB_WPAWN            (ptree->posi.w_pawn)
#define BB_WLANCE           (ptree->posi.w_lance)
#define BB_WKNIGHT          (ptree->posi.w_knight)
#define BB_WSILVER          (ptree->posi.w_silver)
#define BB_WGOLD            (ptree->posi.w_gold)
#define BB_WBISHOP          (ptree->posi.w_bishop)
#define BB_WROOK            (ptree->posi.w_rook)
#define BB_WKING            (abb_mask[SQ_WKING])
#define BB_WPRO_PAWN        (ptree->posi.w_pro_pawn)
#define BB_WPRO_LANCE       (ptree->posi.w_pro_lance)
#define BB_WPRO_KNIGHT      (ptree->posi.w_pro_knight)
#define BB_WPRO_SILVER      (ptree->posi.w_pro_silver)
#define BB_WHORSE           (ptree->posi.w_horse)
#define BB_WDRAGON          (ptree->posi.w_dragon)

#define OCCUPIED_FILE       (ptree->posi.occupied_rl90)
#define OCCUPIED_DIAG1      (ptree->posi.occupied_rr45)
#define OCCUPIED_DIAG2      (ptree->posi.occupied_rl45)
#define BOARD               (ptree->posi.asquare)

#define SQ_BKING            (ptree->posi.isquare_b_king)
#define SQ_WKING            (ptree->posi.isquare_w_king)

#define HASH_KEY            (ptree->posi.hash_key)
#define HASH_VALUE          (ptree->sort_value[0])
#define MOVE_CURR           (ptree->current_move[ply])
#define MOVE_LAST           (ptree->current_move[ply-1])

#define NullDepth(d) ( (d) <  PLY_INC*26/4 ? (d)-PLY_INC*12/4 :              \
                     ( (d) <= PLY_INC*30/4 ? PLY_INC*14/4                    \
                                           : (d)-PLY_INC*16/4) )

#define RecursionThreshold  ( PLY_INC * 3 )

#define RecursionDepth(d) ( (d) < PLY_INC*18/4 ? PLY_INC*6/4                 \
                                               : (d)-PLY_INC*12/4 )

#define LimitExtension(e,ply) if ( (e) && (ply) > 2 * iteration_depth ) {     \
                                if ( (ply) < 4 * iteration_depth ) {          \
                                  e *= 4 * iteration_depth - (ply);           \
                                  e /= 2 * iteration_depth;                   \
                                } else { e = 0; } }

#define Flip(turn)          ((turn)^1)
#define Inv(sq)             (nsquare-1-sq)
#define PcOnSq(k,i)         pc_on_sq[k][(i)*((i)+3)/2]
#define PcPcOnSq(k,i,j)     pc_on_sq[k][(i)*((i)+1)/2+(j)]

/*
  xxxxxxxx xxxxxxxx xxx11111  pawn
  xxxxxxxx xxxxxxxx 111xxxxx  lance
  xxxxxxxx xxxxx111 xxxxxxxx  knight
  xxxxxxxx xx111xxx xxxxxxxx  silver
  xxxxxxx1 11xxxxxx xxxxxxxx  gold
  xxxxx11x xxxxxxxx xxxxxxxx  bishop
  xxx11xxx xxxxxxxx xxxxxxxx  rook
 */
#define I2HandPawn(hand)       (((hand) >>  0) & 0x1f)
#define I2HandLance(hand)      (((hand) >>  5) & 0x07)
#define I2HandKnight(hand)     (((hand) >>  8) & 0x07)
#define I2HandSilver(hand)     (((hand) >> 11) & 0x07)
#define I2HandGold(hand)       (((hand) >> 14) & 0x07)
#define I2HandBishop(hand)     (((hand) >> 17) & 0x03)
#define I2HandRook(hand)        ((hand) >> 19)
#define IsHandPawn(hand)       ((hand) & 0x000001f)
#define IsHandLance(hand)      ((hand) & 0x00000e0)
#define IsHandKnight(hand)     ((hand) & 0x0000700)
#define IsHandSilver(hand)     ((hand) & 0x0003800)
#define IsHandGold(hand)       ((hand) & 0x001c000)
#define IsHandBishop(hand)     ((hand) & 0x0060000)
#define IsHandRook(hand)       ((hand) & 0x0180000)
#define IsHandSGBR(hand)       ((hand) & 0x01ff800)
/*
  xxxxxxxx xxxxxxxx x1111111  destination
  xxxxxxxx xx111111 1xxxxxxx  starting square or drop piece+nsquare-1
  xxxxxxxx x1xxxxxx xxxxxxxx  flag for promotion
  xxxxx111 1xxxxxxx xxxxxxxx  piece to move
  x1111xxx xxxxxxxx xxxxxxxx  captured piece
 */
#define To2Move(to)             ((unsigned int)(to)   <<  0)
#define From2Move(from)         ((unsigned int)(from) <<  7)
#define Drop2Move(piece)        ((nsquare-1+(piece))  <<  7)
#define Drop2From(piece)         (nsquare-1+(piece))
#define FLAG_PROMO               (1U                  << 14)
#define Piece2Move(piece)       ((piece)              << 15)
#define Cap2Move(piece)         ((piece)              << 19)
#define I2To(move)              (((move) >>  0) & 0x007fU)
#define I2From(move)            (((move) >>  7) & 0x007fU)
#define I2FromTo(move)          (((move) >>  0) & 0x3fffU)
#define I2IsPromote(move)       ((move) & FLAG_PROMO)
#define I2PieceMove(move)       (((move) >> 15) & 0x000fU)
#define UToFromToPromo(u)       ( (u) & 0x7ffffU )
#define UToCap(u)               (((u)    >> 19) & 0x000fU)
#define From2Drop(from)         ((from)-nsquare+1)


#define AttackFile(i)  (abb_file_attacks[i]                               \
                         [((ptree->posi.occupied_rl90.p[aslide[i].irl90]) \
                            >> aslide[i].srl90) & 0x7f])

#define AttackRank(i)  (abb_rank_attacks[i]                               \
                         [((ptree->posi.b_occupied.p[aslide[i].ir0]       \
                            |ptree->posi.w_occupied.p[aslide[i].ir0])     \
                             >> aslide[i].sr0) & 0x7f ])

#define AttackDiag1(i)                                         \
          (abb_bishop_attacks_rr45[i]                        \
            [((ptree->posi.occupied_rr45.p[aslide[i].irr45]) \
               >> aslide[i].srr45) & 0x7f])

#define AttackDiag2(i)                                         \
          (abb_bishop_attacks_rl45[i]                        \
            [((ptree->posi.occupied_rl45.p[aslide[i].irl45]) \
               >> aslide[i].srl45) & 0x7f])

#define BishopAttack0(i) ( AttackDiag1(i).p[0] | AttackDiag2(i).p[0] )
#define BishopAttack1(i) ( AttackDiag1(i).p[1] | AttackDiag2(i).p[1] )
#define BishopAttack2(i) ( AttackDiag1(i).p[2] | AttackDiag2(i).p[2] )
#define AttackBLance(bb,i) BBAnd( bb, abb_minus_rays[i], AttackFile(i) )
#define AttackWLance(bb,i) BBAnd( bb, abb_plus_rays[i],  AttackFile(i) )
#define AttackBishop(bb,i) BBOr( bb, AttackDiag1(i), AttackDiag2(i) )
#define AttackRook(bb,i)   BBOr( bb, AttackFile(i), AttackRank(i) )
#define AttackHorse(bb,i)  AttackBishop(bb,i); BBOr(bb,bb,abb_king_attacks[i])
#define AttackDragon(bb,i) AttackRook(bb,i);   BBOr(bb,bb,abb_king_attacks[i])

#define InCheck(turn)                                        \
         ( (turn) ? is_white_attacked( ptree, SQ_WKING )     \
                  : is_black_attacked( ptree, SQ_BKING ) )

#define MakeMove(turn,move,ply)                                \
                ( (turn) ? make_move_w( ptree, move, ply ) \
                         : make_move_b( ptree, move, ply ) )

#define UnMakeMove(turn,move,ply)                                \
                ( (turn) ? unmake_move_w( ptree, move, ply ) \
                         : unmake_move_b( ptree, move, ply ) )

#define IsMoveCheck( ptree, turn, move )                        \
                ( (turn) ? is_move_check_w( ptree, move )   \
                         : is_move_check_b( ptree, move ) )

#define GenCaptures(turn,pmove) ( (turn) ? w_gen_captures( ptree, pmove )   \
                                         : b_gen_captures( ptree, pmove ) )

#define GenNoCaptures(turn,pmove)                                             \
                               ( (turn) ? w_gen_nocaptures( ptree, pmove )  \
                                        : b_gen_nocaptures( ptree, pmove ) )

#define GenDrop(turn,pmove)     ( (turn) ? w_gen_drop( ptree, pmove )       \
                                         : b_gen_drop( ptree, pmove ) )

#define GenCapNoProEx2(turn,pmove)                                 \
                ( (turn) ? w_gen_cap_nopro_ex2( ptree, pmove )   \
                         : b_gen_cap_nopro_ex2( ptree, pmove ) )

#define GenNoCapNoProEx2(turn,pmove)                                \
                ( (turn) ? w_gen_nocap_nopro_ex2( ptree, pmove )  \
                         : b_gen_nocap_nopro_ex2( ptree, pmove ) )

#define GenEvasion(turn,pmove)                                  \
                ( (turn) ? w_gen_evasion( ptree, pmove )      \
                         : b_gen_evasion( ptree, pmove ) )

#define GenCheck(turn,pmove)                                  \
                ( (turn) ? w_gen_checks( ptree, pmove )      \
                         : b_gen_checks( ptree, pmove ) )

#define IsMateIn1Ply(turn)                                    \
                ( (turn) ? is_w_mate_in_1ply( ptree )         \
                         : is_b_mate_in_1ply( ptree ) )

#define IsDiscoverBK(from,to)                                  \
          idirec = (int)adirec[SQ_BKING][from],               \
          ( idirec && ( idirec!=(int)adirec[SQ_BKING][to] )   \
            && is_pinned_on_black_king( ptree, from, idirec ) )

#define IsDiscoverWK(from,to)                                  \
          idirec = (int)adirec[SQ_WKING][from],               \
          ( idirec && ( idirec!=(int)adirec[SQ_WKING][to] )   \
            && is_pinned_on_white_king( ptree, from, idirec ) )
#define IsMateWPawnDrop(ptree,to) ( BOARD[(to)+9] == king                 \
                                     && is_mate_w_pawn_drop( ptree, to ) )

#define IsMateBPawnDrop(ptree,to) ( BOARD[(to)-9] == -king                \
                                     && is_mate_b_pawn_drop( ptree, to ) )

enum { b0000, b0001, b0010, b0011, b0100, b0101, b0110, b0111,
       b1000, b1001, b1010, b1011, b1100, b1101, b1110, b1111 };

enum { A9 = 0, B9, C9, D9, E9, F9, G9, H9, I9,
           A8, B8, C8, D8, E8, F8, G8, H8, I8,
           A7, B7, C7, D7, E7, F7, G7, H7, I7,
           A6, B6, C6, D6, E6, F6, G6, H6, I6,
           A5, B5, C5, D5, E5, F5, G5, H5, I5,
           A4, B4, C4, D4, E4, F4, G4, H4, I4,
           A3, B3, C3, D3, E3, F3, G3, H3, I3,
           A2, B2, C2, D2, E2, F2, G2, H2, I2,
           A1, B1, C1, D1, E1, F1, G1, H1, I1 };

enum { promote = 8, empty = 0,
       pawn, lance, knight, silver, gold, bishop, rook, king, pro_pawn,
       pro_lance, pro_knight, pro_silver, piece_null, horse, dragon };

enum { npawn_max = 18,  nlance_max  = 4,  nknight_max = 4,  nsilver_max = 4,
       ngold_max = 4,   nbishop_max = 2,  nrook_max   = 2,  nking_max   = 2 };

enum { rank1 = 0, rank2, rank3, rank4, rank5, rank6, rank7, rank8, rank9 };
enum { file1 = 0, file2, file3, file4, file5, file6, file7, file8, file9 };

enum { nhand = 7, nfile = 9,  nrank = 9,  nsquare = 81 };

enum { mask_file1 = (( 1U << 18 | 1U << 9 | 1U ) << 8) };

enum { flag_diag1 = b0001, flag_plus = b0010 };

enum { score_draw     =     1,
       score_max_eval = 30000,
       score_matelong = 30002,
       score_mate1ply = 32598,
       score_inferior = 32599,
       score_bound    = 32600,
       score_foul     = 32600 };

enum { phase_hash      = b0001,
       phase_killer1   = b0001 << 1,
       phase_killer2   = b0010 << 1,
       phase_killer    = b0011 << 1,
       phase_cap1      = b0001 << 3,
       phase_cap_misc  = b0010 << 3,
       phase_cap       = b0011 << 3,
       phase_history1  = b0001 << 5,
       phase_history2  = b0010 << 5,
       phase_history   = b0011 << 5,
       phase_misc      = b0100 << 5 };

enum { next_move_hash = 0,  next_move_capture,   next_move_history2,
       next_move_misc };

/* next_evasion_hash should be the same as next_move_hash */
enum { next_evasion_hash = 0, next_evasion_genall, next_evasion_misc };


enum { next_quies_gencap, next_quies_captures, next_quies_misc };

enum { no_rep = 0, four_fold_rep, perpetual_check, perpetual_check2,
       black_superi_rep, white_superi_rep, hash_hit, prev_solution, book_hit,
       pv_fail_high, mate_search };

enum { record_misc, record_eof, record_next, record_resign, record_drawn,
       record_error };

enum { black = 0, white = 1 };	// black(sente), white(gote).

enum { direc_misc           = b0000,
       direc_file           = b0010, /* | */
       direc_rank           = b0011, /* - */
       direc_diag1          = b0100, /* / */
       direc_diag2          = b0101, /* \ */
       flag_cross           = b0010,
       flag_diag            = b0100 };

enum { value_null           = b0000,
       value_upper          = b0001,
       value_lower          = b0010,
       value_exact          = b0011,
       flag_value_up_exact  = b0001,
       flag_value_low_exact = b0010,
       node_do_null         = b0100,
       node_do_recap        = b1000,
       node_do_mate         = b0001 << 4,
       node_mate_threat     = b0010 << 4, /* <- don't change it */ 
       node_do_futile       = b0100 << 4,
       node_do_recursion    = b1000 << 4,
       node_do_hashcut      = b0001 << 8,
       state_node_end };
/* note: maximum bits are 8.  tlp_state_node uses type unsigned char. */

enum { flag_from_ponder     = b0001 };

enum { flag_time            = b0001,
       flag_history         = b0010,
       flag_rep             = b0100,
       flag_detect_hang     = b1000,
       flag_nomake_move     = b0010 << 4,
       flag_nofmargin       = b0100 << 4 };

/* flags represent status of root move */
enum { flag_searched        = b0001,
       flag_first           = b0010 };


enum { flag_mated           = b0001,
       fl