// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "shogi.h"

int CONV
gen_next_move( tree_t * restrict ptree, int ply, int turn )
{
  switch ( ptree->anext_move[ply].next_phase )
    {
    case next_move_hash:
      {
	unsigned int * restrict pmove;
	int * restrict psortv = ptree->sort_value;
	unsigned int move, killer1, killer2, move_hash, move_best, move_second;
	int i, j, sortv, n, value_best, value_second, value, remaining;

	ptree->anext_move[ply].phase_done = 0;
	ptree->anext_move[ply].next_phase = next_move_capture;
	ptree->anext_move[ply].move_last  = pmove = ptree->move_last[ply];
	ptree->move_last[ply] = GenCaptures( turn, pmove );

	move_hash  = ptree->amove_hash[ply];
	killer1    = ptree->amove_killer[ply].no1;
	killer2    = ptree->amove_killer[ply].no2;
	remaining  = 0;
	move_best  = move_second  = 0;
	value_best = value_second = 0;
	n = (int)( ptree->move_last[ply] - pmove );
	for ( i = 0; i < n; i++ )
	  {
	    move = pmove[i];
	    sortv = swap( ptree, move, -1, INT_MAX, turn );
	    if ( sortv > value_best )
	      {
		move_second  = move_best;
		value_second = value_best;
		value_best = sortv;
		move_best  = move;
	      }
	    else if ( sortv > value_second )
	      {
		move_second  = move;
		value_second = sortv;
	      }
	    if ( move == move_hash ) { sortv = INT_MIN; }
	    else if ( UToFromToPromo(move) == killer1 )
	      {
		killer1 = 0;
		value = ptree->amove_killer[ply].no1_value
		  + p_value_ex[15U+UToCap(move)];
		if ( sortv < value ) { sortv = value; }
		if ( sortv > -1 ) { remaining++; }
	      }
	    else if ( UToFromToPromo(move) == killer2 )
	      {
		killer2 = 0;
		value = ptree->amove_killer[ply].no2_value
		  + p_value_ex[15U+UToCap(move)];
		if ( sortv < value ) { sortv = value; }
		if ( sortv > -1 ) { remaining++; }
	      }
	    else if ( sortv > -1 ) { remaining++; }
	    psortv[i] = sortv;
	  }

	if ( killer1
	     && killer1 != move_hash
	     && ptree->amove_killer[ply].no1_value > -1
	     && is_move_valid( ptree, killer1, turn ) )
	  {
	    *( ptree->move_last[ply]++ ) = killer1;
	    psortv[n++] = ptree->amove_killer[ply].no1_value;
	    remaining++;
	  }

	if ( killer2
	     && killer2 != move_hash
	     && ptree->amove_killer[ply].no2_value > -1
	     && is_move_valid( ptree, killer2, turn ) )
	  {
	    *( ptree->