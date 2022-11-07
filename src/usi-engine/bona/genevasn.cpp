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
  XorDiag2( s