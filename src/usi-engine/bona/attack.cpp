// 2019 Team AobaZero
// This source code is in the public domain.
#include <assert.h>
#include <stdlib.h>
#include "shogi.h"

unsigned int CONV
is_pinned_on_white_king( const tree_t * restrict ptree, int isquare,
			int idirec )
{
  bitboard_t bb_attacks, bb_attacker;

  switch ( idirec )
    {
    case direc_rank:
      bb_attacks = AttackRank( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  return BBContract( bb_attacks, BB_B_RD );
	}
      break;

    case direc_file:
      bb_attacks = AttackFile( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  BBAnd( bb_attacker, BB_BLANCE, abb_plus_rays[isquare] );
	  BBOr( bb_attacker, bb_attacker, BB_B_RD );
	  return BBContract( bb_attacks, bb_attacker );  /* return! */
	}
      break;

    case direc_diag1:
      bb_attacks = AttackDiag1( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  return BBContract( bb_attacks, BB_B_BH );      /* return! */
	}
      break;

    default:
      assert( idirec == direc_diag2 );
      bb_attacks = AttackDiag2( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  return BBContract( bb_attacks, BB_B_BH );      /* return! */
	}
      break;
    }
  
  return 0;
}


unsigned int CONV
is_pinned_on_black_king( const tree_t * restrict ptree, int isquare,
			int idirec )
{
  bitboard_t bb_attacks, bb_attacker;

  switch ( idirec )
    {
    case direc_rank:
      bb_attacks = AttackRank( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  return BBContract( bb_attacks, BB_W_RD );
	}
      break;

    case direc_file:
      bb_attacks = AttackFile( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  BBAnd( bb_attacker, BB_WLANCE, abb_minus_rays[isquare] );
	  BBOr( bb_attacker, bb_attacker, BB_W_RD );
	  return BBContract( bb_attacks, bb_attacker );      /* return! */
	}
      break;

    case direc_diag1:
      bb_attacks = AttackDiag1( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  return BBContract( bb_attacks, BB_W_BH );          /* return! */
	}
      break;

    default:
      assert( idirec == direc_diag2 );
      bb_attacks = AttackDiag2( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  return BBContract( bb_attacks, BB_W_BH );          /* return! */
	}
      break;
    }
  return 0;
}


/* perpetual check detections are omitted. */
int CONV
is_mate_b_pawn_drop( tree_t * restrict ptree, int sq_drop )
{
  bitboard_t bb, bb_sum, bb_move;
  int iwk, ito, iret, ifrom, idirec;

  BBAnd( bb_sum, BB_WKNIGHT, abb_b_knight_attacks[sq_drop] );

  BBAndOr( bb_sum, BB_WSILVER, abb_b_silver_attacks[sq_d