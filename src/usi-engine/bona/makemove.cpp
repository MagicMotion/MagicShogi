// 2019 Team AobaZero
// This source code is in the public domain.
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "shogi.h"

#define DropB( PIECE, piece )  Xor( to, BB_B ## PIECE );                    \
                               HASH_KEY    ^= ( b_ ## piece ## _rand )[to]; \
                               HAND_B      -= flag_hand_ ## piece;          \
                               BOARD[to]  = piece

#define DropW( PIECE, piece )  Xor( to, BB_W ## PIECE );                    \
                               HASH_KEY    ^= ( w_ ## piece ## _rand )[to]; \
                               HAND_W      -= flag_hand_ ## piece;          \
                               BOARD[to]  = - piece

#define CapB( PIECE, piece, pro_piece )                   \
          Xor( to, BB_B ## PIECE );                       \
          HASH_KEY  ^= ( b_ ## pro_piece ## _rand )[to];  \
          HAND_W    += flag_hand_ ## piece;               \
          MATERIAL  -= MT_CAP_ ## PIECE

#define CapW( PIECE, piece, pro_piece )                   \
          Xor( to, BB_W ## PIECE );                       \
          HASH_KEY  ^= ( w_ ## pro_piece ## _rand )[to];  \
          HAND_B    += flag_hand_ ## piece;               \
          MATERIAL  += MT_CAP_ ## PIECE

#define NocapProB( PIECE, PRO_PIECE, piece, pro_piece )       \
          Xor( from, BB_B ## PIECE );                         \
          Xor( to,   BB_B ## PRO_PIECE );                     \
          HASH_KEY    ^= ( b_ ## pro_piece ## _rand )[to]     \
                       ^ ( b_ ## piece     ## _rand )[from];  \
          MATERIAL    += MT_PRO_ ## PIECE;                    \
          BOARD[to] = pro_piece

#define NocapProW( PIECE, PRO_PIECE, piece, pro_piece )       \
          Xor( from, BB_W ## PIECE );                         \
          Xor( to,   BB_W ## PRO_PIECE );                     \
          HASH_KEY    ^= ( w_ ## pro_piece ## _rand )[to]     \
                       ^ ( w_ ## piece     ## _rand )[from];  \
          MATERIAL    -= MT_PRO_ ## PIECE;                    \
          BOARD[to]  = - pro_piece
 
#define NocapNoproB( PIECE, piece )                          \
          SetClear( BB_B ## PIECE );                         \
          HASH_KEY    ^= ( b_ ## piece ## _rand )[to]        \
                       ^ ( b_ ## piece ## _rand )[from];     \
          BOARD[to] = piece

#define NocapNoproW( PIECE, piece )                          \
          SetClear( BB_W ## PIECE );                         \
          HASH_KEY    ^= ( w_ ## piece ## _rand )[to]        \
                       ^ ( w_ ## piece ## _rand )[from];     \
          BOARD[to] = - piece


void CONV
make_move_b( tree_t * restrict ptree, unsigned int move, int ply )
{
  const int from = (int)I2From(move);
  const int to   = (int)I2To(move);
  const int nrep = ptree->nrep + ply - 1;

  assert( UToCap(move) != king );
  assert( move );
  assert( is_move_valid( ptree, move, black ) );

  ptree->rep_board_list[nrep]    = HASH_KEY;
  ptree->rep_hand_list[nrep]     = HAND_B;
  ptree->save_material[ply]      = (short)MATERIAL;
  ptree->save_eval[ply+1]        = INT_MAX;

#if defined(YSS_ZERO)
  ptree->history_in_check[nrep]   = InCheck(black);
  ptree->keep_sequence_hash[nrep] = ptree->sequence_hash;
  if ( from >= nsquare ) {
    ptree->sequence_hash ^= get_sequence_hash_drop(nrep & (SEQUENCE_HASH_SIZE-1), to, From2Drop(from)-1);
//    PRT("b:nrep=%3d(%3d):        to=%2d,drop=%d\n",nrep,nrep & (SEQUENCE_HASH_SIZE-1),to,From2Drop(from)-1);
  } else {
    ptree->sequence_hash ^= get_sequence_hash_from_to(nrep & (SEQUENCE_HASH_SIZE-1), from, to,(I2IsPromote(move)!=0));
//    PRT("b:nrep=%3d(%3d):from=%2d,to=%2d,prom=%d,%016" PRIx64 "",nrep,nrep & (SEQUENCE_HASH_SIZE-1),from,to,(I2IsPromote(move)!=0),sequence_hash_from_to[nrep & (SEQUENCE_HASH_SIZE-1)][from][to][(I2IsPromote(move)!=0)]);
  }
  if ( ptree->keep_sequence_hash[nrep] == ptree->sequence_hash ) { PRT("sequence_hash err!\n"); debug(); }
#endif


  if ( from >= nsquare )
    {
      switch ( From2Drop(from) )
	{
	case pawn:   Xor( to-nfile, BB_BPAWN_ATK );
               