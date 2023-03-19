// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "shogi.h"

/* #define DBG_QSEARCH */
#if defined(DBG_QSEARCH)
#  define DOut( ... )  if ( dbg_flag ) { out( __VA_ARGS__ ); }
#else
#  define DOut( ... )
#endif

static int CONV gen_next_quies( tree_t * restrict ptree, int alpha, int turn,
				int ply, int qui_ply );

int CONV
search_quies( tree_t * restrict ptree, int alpha, int beta, int turn, int ply,
	      int qui_ply )
{
  int value, alpha_old, stand_pat;

#if defined(DBG_QSEARCH)
  int dbg_flag = 0;

  if ( iteration_depth == 2 && ply == 4
       && ! strcmp( str_CSA_move(ptree->current_move[1]), "7776FU" )
       && ! strcmp( str_CSA_move(ptree->current_move[2]), "3334FU" )
       && ! strcmp( str_CSA_move(ptree->current_move[3]), "8822UM" ) )
    {
      dbg_flag = 1;
      Out( "qsearch start (alpha=%d beta=%d sp=%d %" PRIu64 ")",
	   alpha, beta, value, ptree->node_searched );
    }
#endif
  

#if defined(TLP)
  if ( ! ptree->tlp_id )
#endif
    {
      node_last_check += 1;
    }
  ptree->node_searched += 1;
  ptree->nquies_called += 1;
  alpha_old             = alpha;
  
  stand_pat = evaluate( ptree, ply, turn );


  if ( alpha < stand_pat )
    {
      if ( beta <= stand_pat )
	{
	  DOut( ", cut by stand-pat\n" );
	  MOVE_CURR = MO