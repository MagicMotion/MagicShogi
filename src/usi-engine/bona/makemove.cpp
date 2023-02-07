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
                       SQ_BKING  = (char)to;           break;
      case pro_pawn:   NocapNoproB( PRO_PAWN, pro_pawn );
                       SetClear( BB_BTGOLD );             break;
      case pro_lance:  NocapNoproB( PRO_LANCE, pro_lance );
                       SetClear( BB_BTGOLD );             break;
      case pro_knight: NocapNoproB( PRO_KNIGHT, pro_knight );
                       SetClear( BB_BTGOLD );             break;
      case pro_silver: NocapNoproB( PRO_SILVER, pro_silver );
                       SetClear( BB_BTGOLD );             break;
      case horse:      NocapNoproB( HORSE, horse );
                       SetClear( BB_B_HDK );
                       SetClear( BB_B_BH );                break;
      default:         assert( ipiece_move == dragon );
                       NocapNoproB( DRAGON, dragon );
                       SetClear( BB_B_HDK );
                       SetClear( BB_B_RD );                break;
      }
    
    if ( ipiece_cap )
      {
	switch( ipiece_cap )
	  {
	  case pawn:       CapW( PAWN, pawn, pawn );
                           Xor( to+nfile, BB_WPAWN_ATK );               break;
	  case lance:      CapW( LANCE,  lance, lance );       break;
	  case knight:     CapW( KNIGHT, knight, knight );      break;
	  case silver:     CapW( SILVER, silver, silver );      break;
	  case gold:       CapW( GOLD,   gold,   gold );
                           Xor( to, BB_WTGOLD );                       break;
	  case bishop:     CapW( BISHOP, bishop, bishop );
                           Xor( to, BB_W_BH );                          break;
	  case rook:       CapW( ROOK, rook, rook);
                           Xor( to, BB_W_RD );                          break;
	  case pro_pawn:   CapW( PRO_PAWN, pawn, pro_pawn );
                           Xor( to, BB_WTGOLD );                       break;
	  case pro_lance:  CapW( PRO_LANCE, lance, pro_lance );
                           Xor( to, BB_WTGOLD );                       break;
	  case pro_knight: CapW( PRO_KNIGHT, knight, pro_knight );
                           Xor( to, BB_WTGOLD );                       break;
	  case pro_silver: CapW( PRO_SILVER, silver, pro_silver );
                           Xor( to, BB_WTGOLD );                       break;
	  case horse:      CapW( HORSE, bishop, horse );
                           Xor( to, BB_W_HDK );
			   Xor( to, BB_W_BH );                          break;
	  default:         assert( ipiece_cap == dragon );
                           CapW( DRAGON, rook, dragon );
                           Xor( to, BB_W_HDK );
			   Xor( to, BB_W_RD );                         break;
	  }
	Xor( to, BB_WOCCUPY );
	XorFile( from, OCCUPIED_FILE );
	XorDiag2( from, OCCUPIED_DIAG2 );
	XorDiag1( from, OCCUPIED_DIAG1 );
      }
    else {
      SetClearFile( from, to, OCCUPIED_FILE );
      SetClearDiag1( from, to, OCCUPIED_DIAG1 );
      SetClearDiag2( from, to, OCCUPIED_DIAG2 );
    }
  }

  assert( exam_bb( ptree ) );
}


void CONV
make_move_w( tree_t * restrict ptree, unsigned int move, int ply )
{
  const int from = (int)I2From(move);
  const int to   = (int)I2To(move);
  const int nrep  = ptree->nrep + ply - 1;

  assert( UToCap(move) != king );
  assert( move );
  assert( is_move_valid( ptree, move, white ) );

  ptree->rep_board_list[nrep]    = HASH_KEY;
  ptree->rep_hand_list[nrep]     = HAND_B;
  ptree->save_material[ply]      = (short)MATERIAL;
  ptree->save_eval[ply+1]        = INT_MAX;

#if defined(YSS_ZERO)
  ptree->history_in_check[nrep]   = InCheck(white);
  ptree->keep_sequence_hash[nrep] = ptree->sequence_hash;
  if ( from >= nsquare ) {
    ptree->sequence_hash ^= get_sequence_hash_drop(nrep & (SEQUENCE_HASH_SIZE-1), to, From2Drop(from)-1);
//    PRT("w:nrep=%3d(%3d):        to=%2d,drop=%d\n",nrep,nrep & (SEQUENCE_HASH_SIZE-1),to,From2Drop(from)-1);
  } else {
    ptree->sequence_hash ^= get_sequence_hash_from_to(nrep & (SEQUENCE_HASH_SIZE-1), from, to, (I2IsPromote(move)!=0));
//    PRT("w:nrep=%3d(%3d):from=%2d,to=%2d,prom=%d,%016" PRIx64 "",nrep,nrep & (SEQUENCE_HASH_SIZE-1),from,to,(I2IsPromote(move)!=0),sequence_hash_from_to[nrep & (SEQUENCE_HASH_SIZE-1)][from][to][(I2IsPromote(move)!=0)]);
  }
  if ( ptree->keep_sequence_hash[nrep] == ptree->sequence_hash ) { PRT("sequence_hash err!\n"); debug(); }
#endif

  if ( from >= nsquare )
    {
      switch( From2Drop(from) )
	{
	case pawn:   Xor( to+nfile, BB_WPAWN_ATK );
                     DropW( PAWN,   pawn );    break;
	case lance:  DropW( LANCE,  lance );   break;
	case knight: DropW( KNIGHT, knight );  break;
	case silver: DropW( SILVER, silver );  break;
	case gold:   DropW( GOLD,   gold );
                     Xor( to, BB_WTGOLD );     break;
	case bishop: DropW( BISHOP, bishop );
                     Xor( to, BB_W_BH );       break;
	default:     DropW( ROOK,   rook );
                     Xor( to, BB_W_RD );       break;
	}
      Xor( to, BB_WOCCUPY );
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
    SetClear( BB_WOCCUPY );
    BOARD[from] = empty;

    if ( is_promote) switch( ipiece_move )
      {
      case pawn:   NocapProW( PAWN, PRO_PAWN, pawn, pro_pawn );
                   Xor( to, BB_WPAWN_ATK );
                   Xor( to, BB_WTGOLD );                           break;
      case lance:  NocapProW( LANCE, PRO_LANCE, lance, pro_lance );
                   Xor( to, BB_WTGOLD );                           break;
      case knight: NocapProW( KNIGHT, PRO_KNIGHT, knight, pro_knight );
                   Xor( to, BB_WTGOLD );                           break;
      case silver: NocapProW( SILVER, PRO_SILVER, silver, pro_silver );
                   Xor( to, BB_WTGOLD );                           break;
      case bishop: NocapProW( BISHOP, HORSE, bishop, horse );
                   Xor( to, BB_W_HDK );
		   SetClear( BB_W_BH );                              break;
      default:     NocapProW( ROOK, DRAGON, rook, dragon);
                   Xor( to, BB_W_HDK );
		   SetClear( BB_W_RD );                              break;
      }
    else switch ( ipiece_move )
      {
      case pawn:       NocapNoproW( PAWN, pawn );
                       Xor( to+nfile, BB_WPAWN_ATK );
                       Xor( to,       BB_WPAWN_ATK );     break;
      case lance:      NocapNoproW( LANCE,     lance);      break;
      case knight:     NocapNoproW( KNIGHT,    knight);     break;
      case silver:     NocapNoproW( SILVER,    silver);     break;
      case gold:       NocapNoproW( GOLD,      gold);
                       SetClear( BB_WTGOLD );             break;
      case bishop:     NocapNoproW( BISHOP,    bishop);
                       SetClear( BB_W_BH );                break;
      case rook:       NocapNoproW( ROOK,      rook);
                       SetClear( BB_W_RD );                break;
      case king:       HASH_KEY    ^= w_king_rand[to] ^ w_king_rand[from];
                       BOARD[to]  = - king;
                       SQ_WKING   = (char)to;
                       SetClear( BB_W_HDK );               break;
      case pro_pawn:   NocapNoproW( PRO_PAWN,   pro_pawn);
                       SetClear( BB_WTGOLD );             break;
      case pro_lance:  NocapNoproW( PRO_LANCE,  pro_lance);
                       SetClear( BB_WTGOLD );             break;
      case pro_knight: NocapNoproW( PRO_KNIGHT, pro_knight);
                       SetClear( BB_WTGOLD );             break;
      case pro_silver: NocapNoproW( PRO_SILVER, pro_silver);
                       SetClear( BB_WTGOLD );             break;
      case horse:      NocapNoproW( HORSE, horse );
                       SetClear( BB_W_HDK );
                       SetClear( BB_W_BH );                break;
      default:         NocapNoproW( DRAGON, dragon );
                       SetClear( BB_W_HDK );
                       SetClear( BB_W_RD );              