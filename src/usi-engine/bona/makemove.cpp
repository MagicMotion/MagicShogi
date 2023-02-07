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
                     DropB( PAWN,   pawn   );  break;
	case lance:  DropB( LANCE,  lance  );  break;
	case knight: DropB( KNIGHT, knight );  break;
	case silver: DropB( SILVER, silver );  break;
	case gold:   DropB( GOLD,   gold   );
                     Xor( to, BB_BTGOLD );     break;
	case bishop: DropB( BISHOP, bishop );
                     Xor( to, BB_B_BH );       break;
	default:     assert( From2Drop(from) == rook );
                     DropB( ROOK,  rook );
		     Xor( to, BB_B_RD );       break;
	}
      Xor( to, BB_BOCCUPY );
      XorFile( to, OCCUPIED_FILE );
      XorDiag2( to, OCCUPIED_DIAG2 );
      XorDiag1( to, OCCUPIED_DIAG1 );
    }
  else {
    const int ipiece_move = (int)I2PieceMove(move);
    const int ipiece_cap  = (int)UToCap(move);
    const int is_promote  = (int)I2IsPromote(move);
    bitboard_t bb_set_clear;

    BBOr( bb_set_clear, abb_mask[from], abb_mask[to] );
    SetClear( BB_BOCCUPY );
    BOARD[from] = empty;

    if ( is_promote ) switch( ipiece_move )
      {
      case pawn:   Xor( to, BB_BPAWN_ATK );
                   Xor( to, BB_BTGOLD );
                   NocapProB( PAWN,   PRO_PAWN,   pawn,   pro_pawn );   break;
      case lance:  Xor( to, BB_BTGOLD );
                   NocapProB( LANCE,  PRO_LANCE,  lance,  pro_lance );  break;
      case knight: Xor( to, BB_BTGOLD );
                   NocapProB( KNIGHT, PRO_KNIGHT, knight, pro_knight ); break;
      case silver: Xor( to, BB_BTGOLD );
                   NocapProB( SILVER, PRO_SILVER, silver, pro_silver ); break;
      case bishop: Xor( to, BB_B_HDK );
		   SetClear( BB_B_BH );
                   NocapProB( BISHOP, HORSE,      bishop, horse );      break;
      default:     assert( ipiece_move == rook );
                   Xor( to, BB_B_HDK );
		   SetClear( BB_B_RD );
                   NocapProB( ROOK,   DRAGON,     rook,   dragon );     break;
      }
    else switch ( ipiece_move )
      {
      case pawn:       Xor( to-nfile, BB_BPAWN_ATK );
                       Xor( to,       BB_BPAWN_ATK );
                       NocapNoproB( PAWN,   pawn);       break;
      case lance:      NocapNoproB( LANCE,  lance);      break;
      case knight:     NocapNoproB( KNIGHT, knight);     break;
      case silver:     NocapNoproB( SILVER, silver);     break;
      case gold:       NocapNoproB( GOLD,   gold);
                       SetClear( BB_BTGOLD );             break;
      case bishop:     SetClear( BB_B_BH );
                       NocapNoproB( BISHOP, bishop);     break;
      case rook:       NocapNoproB( ROOK,   rook);
                       SetClear( BB_B_RD );                break;
      case king:       HASH_KEY ^= b_king_rand[to] ^ b_king_rand[from];
                       SetClear( BB_B_HDK );
                       BOARD[to] = king;
           