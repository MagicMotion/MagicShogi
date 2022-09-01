// 2019 Team AobaZero
// This source code is in the public domain.
#include "shogi.h"

unsigned int * CONV
b_gen_captures( const tree_t * restrict ptree, unsigned int * restrict pmove )
{
  bitboard_t bb_movable, bb_capture, bb_piece, bb_desti;
  unsigned int utemp;
  int ito, ifrom;

  bb_capture = BB_WOCCUPY;
  BBNot( bb_movable, BB_BOCCUPY );

  bb_desti.p[0] = BB_BPAWN_ATK.p[0] & bb_movable.p[0];
  bb_desti.p[1] = BB_BPAWN_ATK.p[1] & bb_capture.p[1];
  bb_desti.p[2] = BB_BPAWN_ATK.p[2] & bb_capture.p[2];
  while ( BBToU( bb_desti ) )
    {
      ito = LastOne( bb_desti );
      Xor( ito, bb_desti );

      ifrom = ito + 9;
      utemp = ( To2Move(ito) | From2Move(ifrom) | Cap2Move(-BOARD[ito])
		| Piece2Move(pawn) );
      if ( ito < A6 ) { utemp |= FLAG_PROMO; }
      *pmove++ = utemp;
    }

  bb_piece = BB_BSILVER;
  while ( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      BBAnd( bb_desti, bb_capture, abb_b_silver_attacks[ifrom] );
      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  utemp = ( To2Move(ito) | From2Move(ifrom) | Cap2Move(-BOARD[ito])
		    | Piece2Move(silver) );
	  if ( ito < A6 || ifrom < A6 ) { *pmove++ = utemp | FLAG_PROMO; }
	  *pmove++ = utemp;
	}
    }

  bb_piece = BB_BTGOLD;
  while( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      BBAnd( bb_desti, bb_capture, abb_b_gold_attacks[ifrom] );
      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  *pmove++ = ( To2Move(ito) | From2Move(ifrom)
		       | Cap2Move(-BOARD[ito])
		       | Piece2Move(BOARD[ifrom]) );
	}
    }

  ifrom = SQ_BKING;
  BBAnd( bb_desti, bb_capture, abb_king_attacks[ifrom] );
  while ( BBTest( bb_desti ) )
    {
      ito = LastOne( bb_desti );
      Xor( ito, bb_desti );

      *pmove++ = ( To2Move(ito) | From2Move(ifrom)
		   | Cap2Move(-BOARD[ito]) | Piece2Move(king) );
    }

  bb_piece = BB_BBISHOP;
  while ( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      AttackBishop( bb_desti, ifrom );
      bb_desti.p[0] &= bb_movable.p[0];
      if ( ifrom < A6 )
	{
	  bb_desti.p[1] &= bb_movable.p[1];
	  bb_desti.p[2] &= bb_movable.p[2];
	}
      else {
	bb_desti.p[1] &= bb_capture.p[1];
	bb_desti.p[2] &= bb_capture.p[2];
      }

      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  utemp = ( To2Move(ito) | From2Move(ifrom)
		    | Cap2Move(-BOARD[ito]) | Piece2Move(bishop) );
	  if ( ito < A6 || ifrom < A6 ) { utemp |= FLAG_PROMO; }
	  *pmove++ = utemp;
	}
    }

  bb_piece = BB_BROOK;
  while ( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      AttackRook( bb_desti, ifrom );
      bb_desti.p[0] &= bb_movable.p[0];
      if ( ifrom < A6 )
	{
	  bb_desti.p[1] &= bb_movable.p[1];
	  bb_desti.p[2] &= bb_movable.p[2];
	}
      else {
	bb_desti.p[1] &= bb_capture.p[1];
	bb_desti.p[2] &= bb_capture.p[2];
      }

      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  utemp = ( To2Move(ito) | From2Move(ifrom)
		    | Cap2Move(-BOARD[ito]) | Piece2Move(rook) );
	  if ( ito < A6 || ifrom < A6 ) { utemp |= FLAG_PROMO; }
	  *pmove++ = utemp;
	}
    }

  bb_piece = BB_BHORSE;
  while ( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      AttackHorse( bb_desti, ifrom );
      BBAnd( bb_desti, bb_desti, bb_capture );
      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  *pmove++ = ( To2Move(ito) | From2Move(ifrom)
		       | Cap2Move(-BOARD[ito]) | Piece2Move(horse) );
	}
    }

  bb_piece = BB_BDRAGON;
  while ( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      AttackDragon( bb_desti, ifrom );
      BBAnd( bb_desti, bb_desti, bb_capture );
      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  *pmove++ = ( To2Move(ito) | From2Move(ifrom)
		       | Cap2Move(-BOARD[ito]) | Piece2Move(dragon) );
	}
    }

  bb_piece = BB_BLANCE;
  while( BBTest( bb_piece ) )
    {
      ifrom = LastOne( bb_piece );
      Xor( ifrom, bb_piece );

      bb_desti = AttackFile( ifrom );
      BBAnd( bb_desti, bb_desti, abb_minus_rays[ifrom] );
      bb_desti.p[0] &= bb_movable.p[0];
      bb_desti.p[1] &= bb_capture.p[1];
      bb_desti.p[2] &= bb_capture.p[2];

      while ( BBTest( bb_desti ) )
	{
	  ito = LastOne( bb_desti );
	  Xor( ito, bb_desti );

	  utemp = ( To2Move(ito) | From2Move(ifrom)
		    | Cap2Move(-BOARD[ito]) | Piece2Move(lance) );
	  if      ( ito < A7 ) { *pmove++ = utemp | FLAG_PROMO; }
	  else if ( ito < A6 )
	    {
	      *pmove++ = utemp | FLAG_PROMO;
	      if ( UToCap(utemp) ) { *pmove++ = utemp; }
	    }
	  else { *pmove++ = utemp; }
	}
   