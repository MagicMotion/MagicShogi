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
       flag_resigned        = b0010,
       flag_drawn           = b0100,
       flag_suspend         = b1000,
       mask_game_end        = b1111,
       flag_quit            = b0001 << 4,
       flag_puzzling        = b0010 << 4,
       flag_pondering       = b0100 << 4,
       flag_thinking        = b1000 << 4,
       flag_problem         = b0001 << 8,
       flag_move_now        = b0010 << 8,
       flag_quit_ponder     = b0100 << 8,
       flag_nostdout        = b1000 << 8,
       flag_search_error    = b0001 << 12,
       flag_nonewlog        = b0010 << 12,
       flag_reverse         = b0100 << 12,
       flag_narrow_book     = b1000 << 12,
       flag_time_extendable = b0001 << 16,
       flag_learning        = b0010 << 16,
       flag_nobeep          = b0100 << 16,
       flag_nostress        = b1000 << 16,
       flag_nopeek          = b0001 << 20,
       flag_noponder        = b0010 << 20,
       flag_noprompt        = b0100 << 20,
       flag_sendpv          = b1000 << 20,
       flag_skip_root_move  = b0001 << 24 };


enum { flag_hand_pawn       = 1 <<  0,
       flag_hand_lance      = 1 <<  5,
       flag_hand_knight     = 1 <<  8,
       flag_hand_silver     = 1 << 11,
       flag_hand_gold       = 1 << 14,
       flag_hand_bishop     = 1 << 17,
       flag_hand_rook       = 1 << 19 };

enum { f_hand_pawn   =    0,
       e_hand_pawn   =   19,
       f_hand_lance  =   38,
       e_hand_lance  =   43,
       f_hand_knight =   48,
       e_hand_knight =   53,
       f_hand_silver =   58,
       e_hand_silver =   63,
       f_hand_gold   =   68,
       e_hand_gold   =   73,
       f_hand_bishop =   78,
       e_hand_bishop =   81,
       f_hand_rook   =   84,
       e_hand_rook   =   87,
       fe_hand_end   =   90,
       f_pawn        =   81,
       e_pawn        =  162,
       f_lance       =  225,
       e_lance       =  306,
       f_knight      =  360,
       e_knight      =  441,
       f_silver      =  504,
       e_silver      =  585,
       f_gold        =  666,
       e_gold        =  747,
       f_bishop      =  828,
       e_bishop      =  909,
       f_horse       =  990,
       e_horse       = 1071,
       f_rook        = 1152,
       e_rook        = 1233,
       f_dragon      = 1314,
       e_dragon      = 1395,
       fe_end        = 1476,

       kkp_hand_pawn   =   0,
       kkp_hand_lance  =  19,
       kkp_hand_knight =  24,
       kkp_hand_silver =  29,
       kkp_hand_gold   =  34,
       kkp_hand_bishop =  39,
       kkp_hand_rook   =  42,
       kkp_hand_end    =  45,
       kkp_pawn        =  36,
       kkp_lance       = 108,
       kkp_knight      = 171,
       kkp_silver      = 252,
       kkp_gold        = 333,
       kkp_bishop      = 414,
       kkp_horse       = 495,
       kkp_rook        = 576,
       kkp_dragon      = 657,
       kkp_end         = 738 };

enum { pos_n = fe_end * ( fe_end + 1 ) / 2 };

typedef struct { bitboard_t gold, silver, knight, lance; } check_table_t;

#if ! defined(MINIMUM)
typedef struct { fpos_t fpos;  unsigned int games, moves, lines; } rpos_t;
typedef struct {
  double pawn, lance, knight, silver, gold, bishop, rook;
  double pro_pawn, pro_lance, pro_knight, pro_silver, horse, dragon;
  float pc_on_sq[nsquare][fe_end*(fe_end+1)/2];
  float kkp[nsquare][nsquare][kkp_end];
} param_t;
#endif

typedef enum { mode_write, mode_read_write, mode_read } record_mode_t;

typedef struct { uint64_t word1, word2; }                        trans_entry_t;
typedef struct { trans_entry_t prefer, always[2]; }              trans_table_t;
typedef struct { int count;  unsigned int cnst[2], vec[RAND_N]; }rand_work_t;

typedef struct {
  int no1_value, no2_value;
  unsigned int no1, no2;
} move_killer_t;

typedef struct { unsigned int no1, no2; } killer_t;

typedef struct {
  union { char str_move[ MAX_ANSWER ][ 8 ]; } info;
  char str_name1[ SIZE_PLAYERNAME ];
  char str_name2[ SIZE_PLAYERNAME ];
  FILE *pf;
  unsigned int games, moves, lines;
} record_t;

typedef struct {
  unsigned int a[PLY_MAX];
  unsigned char type;
  unsigned char length;
  unsigned char depth;
} pv_t;

typedef struct {
  unsigned char ir0,   sr0;
  unsigned char irl90, srl90;
  unsigned char irl45, srl45;
  unsigned char irr45, srr45;
} slide_tbl_t;


typedef struct {
  uint64_t hash_key;
  bitboard_t b_occupied,     w_occupied;
  bitboard_t occupied_rl90,  occupied_rl45, occupied_rr45;
  bitboard_t b_hdk,          w_hdk;
  bitboard_t b_tgold,        w_tgold;
  bitboard_t b_bh,           w_bh;
  bitboard_t b_rd,           w_rd;
  bitboard_t b_pawn_attacks, w_pawn_attacks;
  bitboard_t b_lance,        w_lance;
  bitboard_t b_knight,       w_knight;
  bitboard_t b_silver,       w_silver;
  bitboard_t b_bishop,       w_bishop;
  bitboard_t b_rook,         w_rook;
  bitboard_t b_horse,        w_horse;
  bitboard_t b_dragon,       w_dragon;
  bitboard_t b_pawn,         w_pawn;
  bitboard_t b_gold,         w_gold;
  bitboard_t b_pro_pawn,     w_pro_pawn;
  bitboard_t b_pro_lance,    w_pro_lance;
  bitboard_t b_pro_knight,   w_pro_knight;
  bitboard_t b_pro_silver,   w_pro_silver;
  unsigned int hand_black, hand_white;
  int material;
  signed char asquare[nsquare];
  unsigned char isquare_b_king, isquare_w_king;
} posi_t;


typedef struct {
  unsigned int hand_black, hand_white;
  char turn_to_move;
  signed char asquare[nsquare];
} min_posi_t;

typedef struct {
  uint64_t nodes;
  unsigned int move, status;
#if defined(DFPN_CLIENT)
  volatile int dfpn_cresult;
#endif
} root_move_t;

typedef struct {
  unsigned int *move_last;
  unsigned int move_cap1;
  unsigned int move_cap2;
  int phase_done, next_phase, remaining, value_cap1, value_cap2;
} next_move_t;

/* data: 31  1bit flag_learned */
/*       30  1bit is_flip      */
/*       15 16bit value        */
typedef struct {
  uint64_t key_book;
  unsigned int key_responsible, key_probed, key_played;
  unsigned int hand_responsible, hand_probed, hand_played;
  unsigned int move_played, move_responsible, move_probed, data;
} history_book_learn_t;

typedef struct tree tree_t;
struct tree {
  posi_t posi;
  uint64_t rep_board_list[ REP_HIST_LEN ];
#if defined(YSS_ZERO)
  // 棋譜と探索木を含めた局面図
  min_posi_t record_plus_ply_min_posi[REP_HIST_LEN];
  int history_in_check[REP_HIST_LEN];	// 王手がかかっているか
  uint64_t sequence_hash;
  uint64_t keep_sequence_hash[REP_HIST_LEN];
  int reached_ply;
  int max_reached_ply;
  int sum_reached_ply;
#endif
  uint64_t node_searched;
  unsigned int *move_last[ PLY_MAX ];
  next_move_t anext_move[ PLY_MAX ];
  pv_t pv[ PLY_MAX ];
  move_killer_t amove_killer[ PLY_MAX ];
  unsigned int null_pruning_done;
  unsigned int null_pruning_tried;
  unsigned int check_extension_done;
  unsigned int recap_extension_done;
  unsigned int onerp_extension_done;
  unsigned int neval_called;
  unsigned int nquies_called;
  unsigned int nfour_fold_rep;
  unsigned int nperpetual_check;
  unsigned int nsuperior_rep;
  unsigned int nrep_tried;
  unsigned int ntrans_always_hit;
  unsigned int ntrans_prefer_hit;
  unsigned int ntrans_probe;
  unsigned int ntrans_exact;
  unsigned int ntrans_lower;
  unsigned int ntrans_upper;
  unsigned int ntrans_superior_hit;
  unsigned int ntrans_inferior_hit;
  unsigned int fail_high;
  unsigned int fail_high_first;
  unsigned int rep_hand_list[ REP_HIST_LEN ];
  unsigned int amove_hash[ PLY_MAX ];
  unsigned int amove[ MOVE_LIST_LEN ];
  unsigned int current_move[ PLY_MAX ];
  killer_t killers[ PLY_MAX ];
  unsigned int hist_nmove[ PLY_MAX ];
  unsigned int hist_move[ PLY_MAX ][ MAX_LEGAL_MOVES ];
  int sort_value[ MAX_LEGAL_MOVES ];
  unsigned short hist_tried[ HIST_SIZE ];
  unsigned short hist_good[ HIST_SIZE ];
  short save_material[ PLY_MAX ];
  int save_eval[ PLY_MAX+1 ];
  unsigned char nsuc_check[ PLY_MAX+1 ];
  int nrep;
#if defined(TLP)
  struct tree *tlp_ptrees_sibling[ TLP_MAX_THREADS ];
  struct tree *tlp_ptree_parent;
  lock_t tlp_lock;
  volatile int tlp_abort;
  volatile int tlp_used;
  unsigned short tlp_slot;
  short tlp_beta;
  short tlp_best;
  volatile unsigned char tlp_nsibling;
  unsigned char tlp_depth;
  unsigned char tlp_state_node;
  unsigned char tlp_id;
  char tlp_turn;
  char tlp_ply;
#endif
};

#if defined(YSS_ZERO)
void copy_min_posi(tree_t * restrict ptree, int sideToMove, int ply);
#endif

extern SHARE unsigned int game_status;

extern int npawn_box;
extern int nlance_box;
extern int nknight_box;
extern int nsilver_box;
extern int ngold_box;
extern int nbishop_box;
extern int nrook_box;

extern unsigned int ponder_move_list[ MAX_LEGAL_MOVES ];
extern unsigned int ponder_move;
extern int ponder_nmove;

extern root_move_t root_move_list[ MAX_LEGAL_MOVES ];
extern SHARE int root_abort;
extern int root_nmove;
extern int root_index;
extern int root_value;
extern int root_alpha;
extern int root_beta;
extern int root_turn;
extern int root_nfail_high;
extern int root_nfail_low;
extern int resign_threshold;

extern uint64_t node_limit;
extern unsigned int node_per_second;
extern unsigned int node_next_signal;
extern unsigned int node_last_check;

extern unsigned int hash_mask;
extern int trans_table_age;

extern pv_t last_pv;
extern pv_t alast_pv_save[NUM_UNMAKE];
extern int alast_root_value_save[NUM_UNMAKE];
extern int last_root_value;
extern int amaterial_save[NUM_UNMAKE];
extern unsigned int amove_save[NUM_UNMAKE];
extern unsigned char ansuc_check_save[NUM_UNMAKE];

extern SHARE trans_table_t *ptrans_table;
extern trans_table_t *ptrans_table_orig;
extern int log2_ntrans_table;

extern int depth_limit;

extern unsigned int time_last_result;
extern unsigned int time_last_eff_search;
extern unsigned int time_last_search;
extern unsigned int time_last_check;
extern unsigned int time_turn_start;
extern unsigned int time_start;
extern unsigned int time_max_limit;
extern unsigned int time_limit;
extern unsigned int time_response;
extern unsigned int sec_limit;
extern unsigned int sec_limit_up;
extern unsigned int sec_limit_depth;
extern unsigned int sec_elapsed;
extern unsigned int sec_b_total;
extern unsigned int sec_w_total;

extern record_t record_problems;
extern record_t record_game;
extern FILE *pf_book;
extern int record_num;

extern int p_value_ex[31];
extern int p_value_pm[15];
extern int p_value[31];
extern short pc_on_sq[nsquare][fe_end*(fe_end+1)/2];
extern short kkp[nsquare][nsquare][kkp_end];

extern uint64_t ehash_tbl[ EHASH_MASK + 1 ];
extern rand_work_t rand_work;
extern slide_tbl_t aslide[ nsquare ];
extern bitboard_t abb_b_knight_attacks[ nsquare ];
extern bitboard_t abb_b_silver_attacks[ nsquare ];
extern bitboard_t abb_b_gold_attacks[ nsquare ];
extern bitboard_t abb_w_knight_attacks[ nsquare ];
extern bitboard_t abb_w_silver_attacks[ nsquare ];
extern bitboard_t abb_w_gold_attacks[ nsquare ];
extern bitboard_t abb_king_attacks[ nsquare ];
extern bitboard_t abb_obstacle[ nsquare ][ nsquare ];
extern bitboard_t abb_bishop_attacks_rl45[ nsquare ][ 128 ];
extern bitboard_t abb_bishop_attacks_rr45[ nsquare ][ 128 ];
extern bitboard_t abb_rank_attacks[ nsquare ][ 128 ];
extern bitboard_t abb_file_attacks[ nsquare ][ 128 ];
extern bitboard_t abb_mask[ nsquare ];
extern bitboard_t abb_mask_rl90[ nsquare ];
extern bitboard_t abb_mask_rl45[ nsquare ];
extern bit