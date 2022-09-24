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
	bb_chk       = bb_bishop_chk;
	bb_chk.p[0] |= abb_king_attacks[sq_wk].p[0];
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while ( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}

      while( bb_chk.p[1] | bb_chk.p[2] )
	{
	  to          = last_one12( bb_chk.p[1], bb_chk.p[2] );
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(-BOARD[to]);
	}
    }

  u0 = BB_BBISHOP.p[0];
  while( u0 )
    {
      from = last_one0( u0 );
      u0   ^= abb_mask[from].p[0];
      
      AttackBishop( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	BBOr( bb_chk, bb_bishop_chk, abb_king_attacks[sq_wk] );
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }


  bb_piece = BB_BTGOLD;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk = abb_w_gold_attacks[sq_wk];

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      BBAnd( bb_chk, bb_chk, abb_b_gold_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = ( To2Move(to) | From2Move(from)
		       | Piece2Move(BOARD[from])
		       | Cap2Move(-BOARD[to]) );
	}
    }
  

  u0 = BB_BSILVER.p[0];
  while( u0 )
    {
      from = last_one0( u0 );
      u0   ^= abb_mask[from].p[0];

      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = abb_w_gold_attacks[sq_wk].p[1];
      bb_chk.p[2] = 0;

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      bb_chk.p[0] &= bb_move_to.p[0] & abb_b_silver_attacks[from].p[0];
      bb_chk.p[1] &= bb_move_to.p[1] & abb_b_silver_attacks[from].p[1];

      while( bb_chk.p[0] | bb_chk.p[1] )
	{
	  to          = last_one01( bb_chk.p[0], bb_chk.p[1] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u1 = BB_BSILVER.p[1] & 0x7fc0000U;
  while( u1 )
    {
      from = last_one1( u1 );
      u1   ^= abb_mask[from].p[1];
      
      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = bb_chk.p[2] = 0;
      
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      bb_chk.p[0] &= bb_move_to.p[0] & abb_b_silver_attacks[from].p[0];
      while ( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  bb_piece = BB_BSILVER;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk = abb_w_silver_attacks[sq_wk];

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      BBAnd( bb_chk, bb_chk, abb_b_silver_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(-BOARD[to]);
	}
    }
  

  u0 = BB_BKNIGHT.p[0];
  u1 = BB_BKNIGHT.p[1] & 0x7fffe00U;
  while( u0 | u1 )
    {
      from = last_one01( u0, u1 );
      u0   ^= abb_mask[from].p[0];
      u1   ^= abb_mask[from].p[1];

      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = bb_chk.p[2] = 0;

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      bb_chk.p[0] &= abb_b_knight_attacks[from].p[0] & bb_move_to.p[0];

      while( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(knight)
		       | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u2 = BB_BKNIGHT.p[2];
  u1 = BB_BKNIGHT.p[1] & 0x3ffffU;
  while( u2 | u1 )
    {
      from = last_one12( u1, u2 );
      u2   ^= abb_mask[from].p[2];
      u1   ^= abb_mask[from].p[1];

      bb_chk = abb_w_knight_attacks[sq_wk];

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      BBAnd( bb_chk, bb_chk, abb_b_knight_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(knight)
	    | Cap2Move(-BOARD[to]);
	}
    }


  bb_piece = BB_BLANCE;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = bb_chk.p[2] = 0;

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	}

      BBAnd( bb_chk, bb_chk, AttackFile( from ) );
      BBAnd( bb_chk, bb_chk, abb_minus_rays[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(lance)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u1 = BB_BLANCE.p[1];
  u2 = BB_BLANCE.p[2];
  while( u1| u2 )
    {
      from = last_one12( u1, u2 );
      u1   ^= abb_mask[from].p[1];
      u2   ^= abb_mask[from].p[2];

      bb_chk = bb_file_chk;
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  add_behind_attacks( &bb_chk, idirec, sq_wk );
	  BBAnd( bb_chk, bb_chk, abb_minus_rays[from] );
	}
      else { BBAnd( bb_chk, bb_file_chk, abb_plus_rays[sq_wk] );}

      BBAnd( bb_chk, bb_chk, AttackFile( from ) );
      BBAnd( bb_chk, bb_chk, bb_move_to );
      bb_chk.p[0] = bb_chk.p[0] & 0x1ffU;

      while( BBTest( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(lance)
	    | Cap2Move(-BOARD[to]);
	}
    }

  BBIni( bb_chk );
  bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
  if ( sq_wk < A2 ) { BBOr( bb_chk, bb_chk, abb_mask[sq_wk+nfile] ); }
  BBAnd( bb_chk, bb_chk, bb_move_to );
  BBAnd( bb_chk, bb_chk, BB_BPAWN_ATK );

  BBAnd( bb_piece, bb_diag1_chk, BB_BPAWN );
  while ( BBTest(bb_piece) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );
      
      to = from - nfile;
      if ( BOARD[to] > 0 ) { continue; }

      bb_desti = AttackDiag1( from );
      if ( BBContract( bb_desti, BB_B_BH ) )
	{
	  BBNotAnd( bb_chk, bb_chk, abb_mask[to] );

	  *pmove = To2Move(to) | From2Move(from)
	    | Piece2Move(pawn) | Cap2Move(-BOARD[to]);
	  if ( from < A5 ) { *pmove |= FLAG_PROMO; }
	  pmove += 1;
	}
    }

  BBAnd( bb_piece, bb_diag2_chk, BB_BPAWN );
  while ( BBTest(bb_piece) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );
      
      to = from - nfile;
      if ( BOARD[to] > 0 ) { continue; }

      bb_desti = AttackDiag2( from );
      if ( BBContract( bb_desti, BB_B_BH ) )
	{
	  BBNotAnd( bb_chk, bb_chk, abb_mask[to] );

	  *pmove = To2Move(to) | From2Move(from)
	    | Piece2Move(pawn) | Cap2Move(-BOARD[to]);
	  if ( from < A5 ) { *pm