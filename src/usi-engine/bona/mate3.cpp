// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "shogi.h"

enum { mate_king_cap_checker = 0,
       mate_cap_checker_gen,
       mate_cap_checker,
       mate_king_cap_gen,
       mate_king_cap,
       mate_king_move_gen,
       mate_king_move,
       mate_intercept_move_gen,
       mate_intercept_move,
       mate_intercept_weak_move,
       mate_intercept_drop_sup };

static int CONV mate3_and( tree_t * restrict ptree, int turn, int ply,
			   int flag );
static void CONV checker( const tree_t * restrict ptree, char *psq, int turn );
static unsigned int CONV gen_king_cap_checker( const tree_t * restrict ptree,
					       int to, int turn );
static int CONV mate_weak_or( tree_t * restrict ptree, int turn, int ply,
			      int from, int to );
static unsigned int * CONV gen_move_to( const tree_t * restrict ptree, int sq,
					int turn,
					unsigned int * restrict pmove );
static unsigned int * CONV gen_king_move( const tree_t * restrict ptree,
					  const char *psq, int turn,
					  int is_capture,
					  unsigned int * restrict pmove );
static unsigned int * CONV gen_intercept( tree_t * restrict __ptree__,
					  int sq_checker, int ply, int turn,
					  int * restrict premaining,
					  unsigned int * restrict pmove,
					  int flag );
static int CONV gen_next_evasion_mate( tree_t * restrict ptree,
				       const char *psq, int ply, int turn,
				       int flag );
/*
static uint64_t mate3_hash_tbl[ MATE3_MASK + 1 ] = {0};

static int C