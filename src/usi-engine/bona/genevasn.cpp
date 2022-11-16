// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdio.h>
#include <stdlib.h>
#include "shogi.h"


unsigned int * CONV
b_gen_evasion( tree_t * restrict ptree, unsigned int * restrict pmove )
{
  bitboard_t bb_desti, bb_checker, bb_inter, bb_target, bb_piece;
  unsigned int hand, ubb_target0a, ubb_target0b, ubb_pawn_cmp, utemp;
  unsigned ais_pawn[nfile];
  int nchecker, sq_bk, to, sq_check, idirec;
  int nhand, i, nolance, noknight, from;
  int ahand[6];
  
  /* move the king */
  sq_bk = SQ_BKING;
  
  Xor( sq_bk, BB_BOCCUPY );
  XorFile( sq_bk, OCCUPIED_FILE );
  XorDiag2( sq_bk, OCCUPIED_DIAG2 );
  XorDiag1( sq_bk, OCCUPIED_DIAG1 );

  BBNotAnd( bb_desti, abb_king_attacks[sq_bk], BB_BOCCUPY );
  utemp = From2Move(sq_bk) | Piece2Move(king);
  while ( BBTest( bb_desti ) )
    {
      to = LastOne( bb_desti );
      if ( ! is_black_attacked( ptree, to ) )
	{
	  *pmove++ = To2Move(to) | Cap2Move(-BOARD[to]) | utemp;
	}
      Xor( to, bb_desti );
    }
  
  Xor( sq_bk, BB_BOCCUPY );
  XorFile( sq_bk, OCCUPIED_FILE );
  XorDiag2( sq_bk, OCCUPIED_DIAG2 );
  XorDiag1( sq_bk, OCCUPIED_DIAG1 );
  
  bb_checker = w_attacks_to_piece( ptree, sq_bk );
  nchecker = PopuCount( bb_checker );
  if ( nchecker == 2 ) { return pmove; }
  
  sq_check = LastOne( bb_checker );
  bb_inter = abb_obstacle[sq_bk][sq_check];

  /* move other pieces */
  BBOr( bb_target, bb_inter, bb_checker );
  
  BBAnd( bb_desti, bb_target, BB_BPAWN_ATK );
  while ( BBTest( bb_desti ) )
    {
      to = LastOne( bb_desti );
      Xor( to, bb_desti );

      from = to + 9;
      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  utemp = ( To2Move(to) | From2Move(from) | Piece2Move(pawn)
		    | Cap2Move(-BOARD[to]) );
	  if ( to < A6 ) { utemp |= FLAG_PROMO; }
	  *pmove++ = utemp;
	}
    }

  bb_piece = BB_BLANCE;
  while ( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_desti = AttackFile( from );
      BBAnd( bb_desti, bb_desti, abb_minus_rays[from] );
      BBAnd( bb_desti, bb_desti, bb_target );
      if ( ! BBTest( bb_desti ) ) { continue; }

      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  to = LastOne( bb_desti );

	  utemp = ( To2Move(to) | From2Move(from) | Piece2Move(lance)
		    | Cap2Move(-BOARD[to]) );
	  if ( to <  A6 ) { *pmove++ = utemp | FLAG_PROMO; }
	  if ( to >= A7 ) { *pmove++ = utemp; }
	}
    }

  bb_piece = BB_BKNIGHT;
  while ( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBAnd( bb_desti, bb_target, abb_b_knight_attacks[from] );
      if ( ! BBTest( bb_desti ) ) { continue; }

      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	do {
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );

	  utemp = ( To2Move(to) | From2Move(from) | Piece2Move(knight)
		    | Cap2Move(-BOARD[to]) );
	  if ( to <  A6 ) { *pmove++ = utemp | FLAG_PROMO; }
	  if ( to >= A7 ) { *pmove++ = utemp; }
	  
	} while ( BBTest( bb_desti ) );
    }

  bb_piece = BB_BSILVER;
  while ( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );
      
      BBAnd( bb_desti, bb_target, abb_b_silver_attacks[from] );
      if ( ! BBTest( bb_desti ) ) { continue; }

      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	do {
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );
	  utemp = ( To2Move(to) | From2Move(from) | Piece2Move(silver)
		    | Cap2Move(-BOARD[to]) );
	  if ( from < A6 || to < A6 ) { *pmove++ = utemp | FLAG_PROMO; }
	  *pmove++ = utemp;
	} while ( BBTest( bb_desti ) );
    }

  bb_piece = BB_BTGOLD;
  while( BBTest( bb_piece ) )
    {
      from  = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBAnd( bb_desti, bb_target, abb_b_gold_attacks[from] );
      if ( ! BBTest(bb_desti) ) { continue; }

      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	do {
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );
	  *pmove++ = ( To2Move(to) | From2Move(from)
		       | Piece2Move(BOARD[from])
		       | Cap2Move(-BOARD[to]) );
	} while( BBTest( bb_desti ) );
    }

  bb_piece = BB_BBISHOP;
  while ( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      AttackBishop( bb_desti, from );
      BBAnd( bb_desti, bb_desti, bb_target );
      if ( ! BBTest( bb_desti ) ) { continue; }
      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	do {
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );

	  utemp = ( To2Move(to) | From2Move(from) | Piece2Move(bishop)
		    | Cap2Move(-BOARD[to]) );
	  if ( from < A6 || to < A6 ) { utemp |= FLAG_PROMO; }
	  *pmove++ = utemp;
	} while ( BBTest( bb_desti ) );
    }

  bb_piece = BB_BROOK;
  while ( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      AttackRook( bb_desti, from );
      BBAnd( bb_desti, bb_desti, bb_target );
      if ( ! BBTest( bb_desti ) ) { continue; }
      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	do {
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );

	  utemp = ( To2Move(to) | From2Move(from) | Piece2Move(rook)
		    | Cap2Move(-BOARD[to]) );
	  if ( from < A6 || to < A6 ) { utemp |= FLAG_PROMO; }
	  *pmove++ = utemp;
	} while ( BBTest( bb_desti ) );
    }

  bb_piece = BB_BHORSE;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      AttackHorse( bb_desti, from );
      BBAnd( bb_desti, bb_desti, bb_target );
      if ( ! BBTest(bb_desti) ) { continue; }

      idirec = (int)adirec[sq_bk][from];
      if ( ! idirec || ! is_pinned_on_black_king( ptree, from, idirec ) )
	do {
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti);
	  *pmove++ = ( To2Move(to) | From2Move(from) | Piece2Move(horse)
		       | Cap2Move(-BOARD[to]) );