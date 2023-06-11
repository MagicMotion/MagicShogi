// 2019 Team AobaZero
// This source code is in the public domain.
#include <assert.h>
#include "shogi.h"


#define CapW( PIECE, piece, pro_piece )  Xor( to, BB_W ## PIECE );           \
                                         HAND_B      -= flag_hand_ ## piece; \
                                         BOARD[to]  = -pro_piece

#define CapB( PIECE, piece, pro_piece )  Xor( to, BB_B ## PIECE );           \
                                         HAND_W      -= flag_hand_ ## piece; \
                                         BOARD[to]  = pro_piece

#define NocapNopro( PIECE, piece )  SetClear( BB_ ## PIECE ); \
                                    BOARD[from] = piece 

#define NocapPro( PIECE , PRO_PIECE, piece )  Xor( from, BB_ ## PIECE );     \
                                              Xor( to,   BB_ ## PRO_PIECE ); \
                                              BOARD[from] = piece


void CONV
unmake_move_b( tree_t * restrict ptree, unsigned int move, int ply )
{
  int from = (int)I2From(move);
  int to   = (int)I2To(move);
  int nrep = ptree->nrep + ply - 1;

  HASH_KEY = ptree->rep_board_list[nrep];
  MATERIAL = ptree->save_material[ply];

#if defined(YSS_ZERO)
  ptree->sequence_hash = ptree->keep_sequence_hash[nrep];
#endif

  if ( from >= nsquare )
    {
      switch( From2Drop(from) )
	{
	case pawn:    Xor( to, BB_BPAWN );
                      Xor( to-nfile, BB_BPAWN_ATK );
                      HAND_B += flag_hand_pawn;     break;
	case lance:   Xor( to, BB_BLANCE );
                      HAND_B += flag_hand_lance;    break;
	case knight:  Xor( to, BB_BKNIGHT );
                      HAND_B += flag_hand_knight;   break;
	case silver:  Xor( to, BB_BSILVER );
                      HAND_B += flag_hand_silver;   break;
	case gold:    Xor( to, BB_BGOLD );
                      Xor( to, BB_BTGOLD );
                      HAND_B += flag_hand_gold;     break;
	case bishop:  Xor( to, BB_BBISHOP );
                      Xor( to, BB_B_BH );
                      HAND_B += flag_hand_bishop;   break;
	default:      assert( From2Drop(from) == rook );
                      Xor( to, BB_BROOK );
                      Xor( to, BB_B_RD );
                      HAND_B += flag_hand_rook;  break;
	}

      BOARD[to] = empty;
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

    if ( is_promote ) switch( ipiece_move )
      {
      case pawn:    NocapPro( BPAWN, BPRO_PAWN, pawn );
                    Xor( to, BB_BPAWN_ATK );
                    Xor( to, BB_BTGOLD );                        break;
      case lance:   NocapPro( BLANCE,  BPRO_LANCE, lance );
                    Xor( to, BB_BTGOLD );                        break;
      case knight:  NocapPro( BKNIGHT, BPRO_KNIGHT, knight );
                    Xor( to, BB_BTGOLD );                        break;
      case silver:  NocapPro( BSILVER, BPRO_SILVER, silver );
                    Xor( to, BB_BTGOLD );                        break;
      case bishop:  NocapPro( BBISHOP, BHORSE, bishop );
                    Xor( to, BB_B_HDK );
		    SetClear( BB_B_BH );                         break;
      default:      assert( ipiece_move == rook );
                    NocapPro( BROOK, BDRAGON, rook );
                    Xor( to, BB_B_HDK );
		    SetClear( BB_B_RD );                         break;
      }
    else switch ( ipiece_move )
      {
      case pawn:	NocapNopro( BPAWN, pawn );
                        X