// 2019 Team AobaZero
// This source code is in the public domain.
#include <assert.h>
#include "shogi.h"


static void CONV add_behind_attacks( bitboard_t * restrict pbb, int idirec,
				     int ik );

unsigned int * CONV
b_gen_checks( tree_t * restrict __ptree__, unsigned int * restrict pmove )
{
  bitboard_t bb_piece, bb_rook_chk, bb_bishop_chk, bb_chk, bb_move_to;
  bitboard_t bb_diag1_chk, bb_diag2_chk, bb_file_chk, bb_drop_to, bb_desti;
  bitboard_t bb_rank_chk;
  const tree_t * restrict ptree = __ptree__;
  unsigned int u0, u1, u2;
  int from, to, sq_wk, idirec;

  sq_wk = SQ_WKING;
  bb_file_chk = AttackFile( sq_wk );
  bb_rank_chk = AttackRank( sq_wk );
  BBOr( bb_rook_chk, bb_file_chk, bb_rank_chk );

  bb_diag1_chk = AttackDiag1( sq_wk );
  bb_diag2_chk = AttackDiag2( sq_wk );
  BBOr( bb_bishop_chk, bb_diag1_chk, bb_diag2_chk );
  BBNot( bb_move_to, BB_BOCCUPY );
  BBOr( bb_drop_to, BB_BOCCUPY, BB_WOCCUPY );
  BBNot( bb_drop_to, bb_drop_to );

  from  = SQ_BKING;
  idirec = (int)adirec[sq_wk][from];
  if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
    {
      BBIni( bb_chk );
      add_behind_attacks( &bb_chk, idirec, sq_wk );
      BBAnd( bb_chk, bb_chk, abb_king_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );
      
      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(king)
	    | Cap2Move(-BOARD[to]);
	}
    }
  
  
  bb_piece = BB_BDRAGON;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBOr( bb_chk, bb_rook_chk, abb_king_attacks[sq_wk] );
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      AttackDragon( bb_desti, from );
      BBAnd( bb_chk, bb_chk, bb_desti );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(dragon)
	    | Cap2Move(-BOARD[to]);
	}
    }

  bb_piece = BB_BHORSE;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBOr( bb_chk, bb_bishop_chk, abb_king_attacks[sq_wk] );
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      AttackHorse( bb_desti, from );
      BBAnd( bb_chk, bb_chk, bb_desti );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(horse)
	    | Cap2Move(-BOARD[to]);
	}
    }

  u1 = BB_BROOK.p[1];
  u2 = BB_BROOK.p[2];
  while( u1 | u2 )
    {
      from = last_one12( u1, u2 );
      u1   ^= abb_mask[from].p[1];
      u2   ^= abb_mask[from].p[2];

      AttackRook( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	bb_chk       = bb_rook_chk;
	bb_chk.p[0] |= abb_king_attacks[sq_wk].p[0];
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while ( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}

      while( bb_chk.p[1] | bb_chk.p[2] )
	{
	  to          = last_one12( bb_chk.p[1], bb_chk.p[2] );
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(-BOARD[to]);
	}
    }

  u0 = BB_BROOK.p[0];
  while( u0 )
    {
      from = last_one0( u0 );
      u0   ^= abb_mask[from].p[0];
      
      AttackRook( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	BBOr( bb_chk, bb_rook_chk, abb_king_attacks[sq_wk] );
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }

  u1 = BB_BBISHOP.p[1];
  u2 = BB_BBISHOP.p[2];
  while( u1 | u2 )
    {
      from = last_one12( u1, u2 );
      u1   ^= abb_mask[from].p[1];
      u2   ^= abb_mask[from].p[2];

      AttackBishop( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	bb_ch