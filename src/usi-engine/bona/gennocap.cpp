// 2019 Team AobaZero
// This source code is in the public domain.
#include "shogi.h"

unsigned int * CONV
b_gen_nocaptures( const tree_t * restrict ptree,
		  unsigned int * restrict pmove )
{
  bitboard_t bb_empty, bb_piece, bb_desti;
  unsigned int utemp;
  int to, from;

  BBOr( bb_empty, BB_BOCCUPY, BB_WOCCUPY );
  BBNot( bb_empty, bb_empty );

  bb_piece.p[1] = BB_BPAWN_ATK.p[1] & bb_empty.p[1];
  bb_piece.p[2] = BB_BPAWN_ATK.p[2] & bb_empty.p[2];
  while( bb_piece.p[1] | bb_piece.p[2] )
    {
      to            = last_one12( bb_piece.p[1], bb_piece.p[2] );
      bb_piece.p[1] ^= abb_mask[to].p[1];
      bb_piece.p[2] ^= abb_mask[to].p[2];
      from          = to + 9;
      *pmove++ = To2Move( to ) | From2Move( from ) | Piece2Move( pawn );
    }

  bb_piece = BB_BSILVER;
  while( BBTest( bb_piece ) )
    {
      from   = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBAnd( bb_desti, bb_empty, abb_b_silver_attacks[from] );
      while ( BBTest( bb_desti ) )
	{
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );

	  utemp = To2Move( to ) | From2Move( from ) | Piece2Move( silver );
	  if ( from < A6 || to < A6 ) { *pmove++ = utemp | FLAG_PROMO; }
	  *pmove++ = utemp;
	}
    }

  bb_piece = BB_BTGOLD;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBAnd( bb_desti, bb_empty, abb_b_gold_attacks[from] );
      utemp = From2Move( from ) | Piece2Move( BOARD[from] ); 
      while ( BBTest( bb_desti ) )
	{
	  to = LastOne( bb_desti );
	  Xor( to, bb_desti );
	  *pmove++ = To2Move( to ) | utemp;
	}
    }

  from = SQ_BKING;
  BBAnd( bb_desti, bb_empty, abb_king_attacks[from] );
  utemp = From2Move( from ) | Piece2Move( king ); 
  while ( BBTest( bb_desti ) )
    {
      to = LastOne( bb_desti );
      Xor( to, bb_desti );
      *pmove++ = To2Move( to ) | utemp;
    }

  bb_piece.p[1] = BB_BBISHOP.p[1];
  bb_piece.p[2] = BB_BBISHOP.p[2];
  while( bb_piece.p[1] | bb_piece.p[2] )
    {
      from          = last_one12( bb_piece.p[1], bb_piece.p[2] );
      bb_piece.p[1] ^= abb_mask[from].p[1];
      bb_piece.p[2] ^= abb_mask[from].p[2];

      AttackBishop( bb_desti, from );
      bb_desti.p[1] &= bb_empty.p[1];
      bb_desti.p[2] &= bb_empty.p[2];

      utemp = From2Move( from ) | Piece2Move( bishop );
      while ( bb_desti.p[1] | bb_desti.p[2] )
	{
	  to            = last_one12( bb_desti.p[1], bb_desti.p[2] );
	  bb_desti.p[1] ^= abb_mask[to].p[1];
	  bb_desti.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move( to ) | utemp;
	}
    }

  bb_piece.p[1] = BB_BROOK.p[1];
  bb_piece.p[2] = BB_BROOK.p[2];
  while( bb_piece.p[1] | bb_piece.p[2] )
    {
      from          = last_one12( bb_piece.p[1], bb_piece.p[2] );
      bb_piece.p[1] ^= abb_mask[from].p[1];
      bb_piece.p[2] ^= abb_mask[from].p[2];

      AttackRook( bb_desti, from );
      bb_desti.p[1] &= bb_empty.p[1];
      bb_desti.p[2] &= bb_empty.p[2];

      utemp = From2Move( from ) | Piece2Move( rook );
      while ( bb_desti.p[1] | bb_desti.p[2] )
	{
	  to            = last_one12( bb_desti.p[1], bb_desti.p[2] );
	  bb_desti.p[1] ^= abb_mask[to].p[1];
	  bb_desti.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move( to ) | utemp;
	}
    }

  bb_piece = BB_BHORSE;
  while( BBTest( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );
      
      AttackHorse( bb_desti, from );
      BBAnd( bb_desti, bb_desti, bb