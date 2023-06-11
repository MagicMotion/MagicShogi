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
unmake_move_b( tree_t * restrict ptree, unsigned i