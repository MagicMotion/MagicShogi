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

static int CONV
mhash_probe( tree_t * restrict ptree, int turn, int ply )
{
  uint64_t key_current, key, word;
  unsigned int move;

  word  = mate3_hash_tbl[ (unsigned int)HASH_KEY & MATE3_MASK ];
#if ! defined(__x86_64__)
  word ^= word << 32;
#endif

  key          = word     & ~(uint64_t)0x7ffffU;
  key_current  = HASH_KEY & ~(uint64_t)0x7ffffU;
  key_current ^= (uint64_t)HAND_B << 42;
  key_current ^= (uint64_t)turn << 63;

  if ( key != key_current ) { return 0; }

  move = (unsigned int)word & 0x7ffffU;
  if ( move != MOVE_NA )
    {
      move |= turn ? Cap2Move( BOARD[I2To(move)])
                   : Cap2Move(-BOARD[I2To(move)]);
    }

  MOVE_CURR = move;

  return 1;
}


static void CONV
mhash_store( const tree_t * restrict ptree, int turn, unsigned int move )
{
  uint64_t word;

  word  = HASH_KEY & ~(uint64_t)0x7ffffU;
  word |= (uint64_t)( move & 0x7ffffU );
  word ^= (uint64_t)HAND_B << 42;
  word ^= (uint64_t)turn << 63;
  
#if ! defined(__x86_64__)
  word ^= word << 32;
#endif
  mate3_hash_tbl[ (unsigned int)HASH_KEY & MATE3_MASK ] = word;
}
*/

unsigned int CONV
is_mate_in3ply( tree_t * restrict ptree, int turn, int ply )
{
  int value, flag_skip;

/*
  if ( mhash_probe( ptree, turn, ply ) )
    {
      if ( MOVE_CURR == MOVE_NA ) { return 0; }
      else                        { return 1; }
    }
*/

  if ( ply >= PLY_MAX-2 ) { return 0; }

  flag_skip = 0;

#if defined(YSS_ZERO)
  ptree->anext_move[ply].move_last = ptree->move_last[0];
  ptree->move_last[ply] = GenCheck( turn, ptree->move_last[0] );
#else
  ptree->anext_move[ply].move_last = ptree->move_last[ply-1];	// only move_last[0] is set in AobaZero.
  ptree->move_last[ply] = GenCheck( turn, ptree->move_last[ply-1] );
#endif

  while ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
    {
      MOVE_CURR = *ptree->anext_move[ply].move_last++;

      if ( MOVE_CURR & MOVE_CHK_CLEAR ) { flag_skip = 0; }
      if ( flag_skip ) { continue; }

      assert( is_move_valid( ptree, MOVE_CURR, turn ) );
      MakeMove( turn, MOVE_CURR, ply );
      if ( InCheck(turn) )
	{
	  UnMakeMove( turn, MOVE_CURR, ply );
	  continue;
	}

      value = mate3_and( ptree, Flip(turn), ply+1, 0 );
      
      UnMakeMove( turn, MOVE_CURR, ply );

      if ( value )
	{
//	  mhash_store( ptree, turn, MOVE_CURR );
	  return 1;
	}

      if ( ( MOVE_CURR & MOVE_CHK_SET )
	   && I2To(MOVE_CURR) != I2To(ptree->current_move[ply+1]) )
	{
	  flag_skip = 1;
	}
    }

//mhash_store( ptree, turn, MOVE_NA );
  return 0;
}


static int CONV
mate3_and( tree_t * restrict ptree, int turn, int ply, int flag )
{
  unsigned int move;
  char asq[2];

  assert( InCheck(turn) );

  ptree->anext_move[ply].next_phase = mate_king_cap_checker;
  checker( ptree, asq, turn );

  while ( gen_next_evasion_mate( ptree, asq, ply, turn, flag ) ) {

    if ( ptree-