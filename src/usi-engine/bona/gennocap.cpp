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
      *pmove++ = To2Move