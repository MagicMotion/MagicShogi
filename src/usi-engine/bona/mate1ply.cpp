
// 2019 Team AobaZero
// This source code is in the public domain.
#include <assert.h>
#include <stdlib.h>
#include "shogi.h"

#define DebugOut { static int count = 0; \
                   if ( count++ < 16 ) { out_CSA_posi( ptree, stdout, 0 ); } }

static int CONV can_w_king_escape( tree_t * restrict ptree, int to,
				   const bitboard_t * restrict pbb );
static int CONV can_b_king_escape( tree_t * restrict ptree, int to,
				   const bitboard_t * restrict pbb );
static int CONV can_w_piece_capture( const tree_t * restrict ptree, int to );
static int CONV can_b_piece_capture( const tree_t * restrict ptree, int to );


unsigned int CONV
is_b_mate_in_1ply( tree_t * restrict ptree )
{
  bitboard_t bb, bb_temp, bb_check, bb_check_pro, bb_attacks, bb_drop, bb_move;
  unsigned int ubb;
  int to, from, idirec;

  assert( ! is_black_attacked( ptree, SQ_BKING ) );

  /*  Drops  */
  BBOr( bb_drop, BB_BOCCUPY, BB_WOCCUPY );
  BBNot( bb_drop, bb_drop );

  if ( IsHandRook(HAND_B) ) {

    BBAnd( bb, abb_w_gold_attacks[SQ_WKING], abb_b_gold_attacks[SQ_WKING] );
    BBAnd( bb, bb, bb_drop );
    while( BBTest(bb) )
      {
	to = FirstOne( bb );
	Xor( to, bb );

	if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
	BBOr( bb_attacks, abb_file_attacks[to][0], abb_rank_attacks[to][0] );
	if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	if ( can_w_piece_capture( ptree, to ) )            { continue; }
	return To2Move(to) | Drop2Move(rook);
      }

  } else if ( IsHandLance(HAND_B) && SQ_WKING <= I2 ) {

    to = SQ_WKING+nfile;
    if ( ! BOARD[to] && is_white_attacked( ptree, to ) )
      {
	bb_attacks = abb_file_attacks[to][0];
	if ( ! can_w_king_escape( ptree, to, &bb_attacks )
	     && ! can_w_piece_capture( ptree, to ) )
	  {
	    return To2Move(to) | Drop2Move(lance);
	  }
      }
  }

  if ( IsHandBishop(HAND_B) ) {

    BBAnd( bb, abb_w_silver_attacks[SQ_WKING],
	   abb_b_silver_attacks[SQ_WKING] );
    BBAnd( bb, bb, bb_drop );
    while( BBTest(bb) )
      {
	to = FirstOne( bb );
	Xor( to, bb );

	if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
	BBOr( bb_attacks, abb_bishop_attacks_rr45[to][0],
	      abb_bishop_attacks_rl45[to][0] );
	if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	if ( can_w_piece_capture( ptree, to ) )            { continue; }
	return To2Move(to) | Drop2Move(bishop);
      }
  }

  if ( IsHandGold(HAND_B) ) {

    if ( IsHandRook(HAND_B) )
      {
	BBAnd( bb, abb_b_gold_attacks[SQ_WKING],
	       abb_b_silver_attacks[SQ_WKING] );
	BBNotAnd( bb, bb_drop, bb );
	BBAnd( bb, bb, abb_w_gold_attacks[SQ_WKING] );
      }
    else { BBAnd( bb, bb_drop, abb_w_gold_attacks[SQ_WKING] ); }

    while ( BBTest(bb) )
      {
	to = FirstOne( bb );
	Xor( to, bb );
	
	if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
	bb_attacks = abb_b_gold_attacks[to];
	if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	if ( can_w_piece_capture( ptree, to ) )            { continue; }
	return To2Move(to) | Drop2Move(gold);
      }
  }
  
  if ( IsHandSilver(HAND_B) ) {
    
    if ( IsHandGold(HAND_B) )
      {
	if ( IsHandBishop(HAND_B) ) { goto b_silver_drop_end; }
	BBNotAnd( bb,
		  abb_w_silver_attacks[SQ_WKING],
		  abb_w_gold_attacks[SQ_WKING]  );
	BBAnd( bb, bb, bb_drop );
      }
    else {
      BBAnd( bb, bb_drop, abb_w_silver_attacks[SQ_WKING] );
      if ( IsHandBishop(HAND_B) )
	{
	  BBAnd( bb, bb, abb_w_gold_attacks[SQ_WKING] );
	}
    }
    
    while ( BBTest(bb) )
      {
	to = FirstOne( bb );
	Xor( to, bb );
	
	if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
	bb_attacks = abb_b_silver_attacks[to];
	if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	if ( can_w_piece_capture( ptree, to ) )            { continue; }
	return To2Move(to) | Drop2Move(silver);
      }
  }
 b_silver_drop_end:
 
  if ( IsHandKnight(HAND_B) ) {
    
    BBAnd( bb, bb_drop, abb_w_knight_attacks[SQ_WKING] );
    while ( BBTest(bb) )
      {
	to = FirstOne( bb );
	Xor( to, bb );
	
	BBIni( bb_attacks );
	if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	if ( can_w_piece_capture( ptree, to ) )            { continue; }
	return To2Move(to) | Drop2Move(knight);
      }
  }

  /*  Moves  */
  BBNot( bb_move, BB_BOCCUPY );

  bb = BB_BDRAGON;
  while ( BBTest(bb) ) {
    from = FirstOne( bb );
    Xor( from, bb );

    AttackDragon( bb_attacks, from );
    BBAnd( bb_check, bb_move,  bb_attacks );
    BBAnd( bb_check, bb_check, abb_king_attacks[SQ_WKING] );
    if ( ! BBTest(bb_check) ) { continue; }

    Xor( from, BB_B_HDK );
    Xor( from, BB_B_RD );
    Xor( from, BB_BOCCUPY );
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }

      if ( (int)adirec[SQ_WKING][to] & flag_cross )
	{
	  BBOr( bb_attacks, abb_file_attacks[to][0], abb_rank_attacks[to][0] );
	  BBOr( bb_attacks, bb_attacks, abb_king_attacks[to] );
	}
      else { AttackDragon( bb_attacks, to ); }

      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      Xor( from, BB_BOCCUPY );
      Xor( from, BB_B_RD );
      Xor( from, BB_B_HDK );
      return ( To2Move(to) | From2Move(from)
	       | Cap2Move(-BOARD[to]) | Piece2Move(dragon) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    Xor( from, BB_BOCCUPY );
    Xor( from, BB_B_RD );
    Xor( from, BB_B_HDK );
  }

  bb.p[0] = BB_BROOK.p[0];
  while ( bb.p[0] ) {
    from = last_one0( bb.p[0] );
    bb.p[0] ^= abb_mask[from].p[0];

    AttackRook( bb_attacks, from );
    BBAnd( bb_check, bb_move, bb_attacks );
    BBAnd( bb_check, bb_check, abb_king_attacks[SQ_WKING] );
    if ( ! BBTest(bb_check) ) { continue; }

    BB_B_RD.p[0]    ^= abb_mask[from].p[0];
    BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      if ( (int)adirec[SQ_WKING][to] & flag_cross )
	{
	  BBOr( bb_attacks, abb_file_attacks[to][0], abb_rank_attacks[to][0] );
	  BBOr( bb_attacks, bb_attacks, abb_king_attacks[to] );
	}
      else { AttackDragon( bb_attacks, to ); }

      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
      BB_B_RD.p[0]    ^= abb_mask[from].p[0];
      return ( To2Move(to) | From2Move(from) | FLAG_PROMO
	       | Cap2Move(-BOARD[to]) | Piece2Move(rook) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
    BB_B_RD.p[0]    ^= abb_mask[from].p[0];
  }

  bb.p[1] = BB_BROOK.p[1];
  bb.p[2] = BB_BROOK.p[2];
  while ( bb.p[1] | bb.p[2] ) {
    from = first_one12( bb.p[1], bb.p[2] );
    bb.p[1] ^= abb_mask[from].p[1];
    bb.p[2] ^= abb_mask[from].p[2];

    AttackRook( bb_attacks, from );
    BBAnd( bb_check, bb_move, bb_attacks );
    bb_check.p[0] &= abb_king_attacks[SQ_WKING].p[0];
    bb_check.p[1] &= abb_b_gold_attacks[SQ_WKING].p[1];
    bb_check.p[2] &= abb_b_gold_attacks[SQ_WKING].p[2];
    bb_check.p[1] &= abb_w_gold_attacks[SQ_WKING].p[1];
    bb_check.p[2] &= abb_w_gold_attacks[SQ_WKING].p[2];
    if ( ! BBTest(bb_check) ) { continue; }

    BB_B_RD.p[1]    ^= abb_mask[from].p[1];
    BB_B_RD.p[2]    ^= abb_mask[from].p[2];
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      if ( to <= I7 ) {
	if ( (int)adirec[SQ_WKING][to] & flag_cross )
	  {
	    BBOr(bb_attacks, abb_file_attacks[to][0], abb_rank_attacks[to][0]);
	    bb_attacks.p[0] |= abb_king_attacks[to].p[0];
	    bb_attacks.p[1] |= abb_king_attacks[to].p[1];
	  }
	else { AttackDragon( bb_attacks, to ); }

      } else {
	BBOr( bb_attacks, abb_file_attacks[to][0], abb_rank_attacks[to][0] );
      }
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
      BB_B_RD.p[1]    ^= abb_mask[from].p[1];
      BB_B_RD.p[2]    ^= abb_mask[from].p[2];
      return ( To2Move(to) | From2Move(from)
	       | ( (to < A6) ? FLAG_PROMO : 0 )
	       | Cap2Move(-BOARD[to]) | Piece2Move(rook) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
    BB_B_RD.p[1]    ^= abb_mask[from].p[1];
    BB_B_RD.p[2]    ^= abb_mask[from].p[2];
  }

  bb = BB_BHORSE;
  while ( BBTest(bb) ) {
    from = FirstOne( bb );
    Xor( from, bb );

    AttackHorse( bb_attacks, from );
    BBAnd( bb_check, bb_move,  bb_attacks );
    BBAnd( bb_check, bb_check, abb_king_attacks[SQ_WKING] );
    if ( ! BBTest(bb_check) ) { continue; }

    Xor( from, BB_B_HDK );
    Xor( from, BB_B_BH );
    Xor( from, BB_BOCCUPY );
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      BBOr( bb_attacks, abb_bishop_attacks_rr45[to][0],
	    abb_bishop_attacks_rl45[to][0] );
      BBOr( bb_attacks, bb_attacks, abb_king_attacks[to] );
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      Xor( from, BB_BOCCUPY );
      Xor( from, BB_B_BH );
      Xor( from, BB_B_HDK );
      return ( To2Move(to) | From2Move(from)
	       | Cap2Move(-BOARD[to]) | Piece2Move(horse) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    Xor( from, BB_BOCCUPY );
    Xor( from, BB_B_BH );
    Xor( from, BB_B_HDK );
  }

  bb.p[0] = BB_BBISHOP.p[0];
  while ( bb.p[0] ) {
    from = last_one0( bb.p[0] );
    bb.p[0] ^= abb_mask[from].p[0];

    AttackBishop( bb_attacks, from );
    BBAnd( bb_check, bb_move, bb_attacks );
    BBAnd( bb_check, bb_check, abb_king_attacks[SQ_WKING] );
    if ( ! BBTest(bb_check) ) { continue; }

    BB_B_BH.p[0]    ^= abb_mask[from].p[0];
    BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      BBOr( bb_attacks, abb_bishop_attacks_rr45[to][0],
	    abb_bishop_attacks_rl45[to][0] );
      BBOr( bb_attacks, bb_attacks, abb_king_attacks[to] );
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
      BB_B_BH.p[0]    ^= abb_mask[from].p[0];
      return ( To2Move(to) | From2Move(from) | FLAG_PROMO
	       | Cap2Move(-BOARD[to]) | Piece2Move(bishop) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
    BB_B_BH.p[0]    ^= abb_mask[from].p[0];
  }

  bb.p[1] = BB_BBISHOP.p[1];
  bb.p[2] = BB_BBISHOP.p[2];
  while ( bb.p[1] | bb.p[2] ) {
    from = first_one12( bb.p[1], bb.p[2] );
    bb.p[1] ^= abb_mask[from].p[1];
    bb.p[2] ^= abb_mask[from].p[2];

    AttackBishop( bb_attacks, from );
    BBAnd( bb_check, bb_move, bb_attacks );
    bb_check.p[0] &= abb_king_attacks[SQ_WKING].p[0];
    bb_check.p[1] &= abb_b_silver_attacks[SQ_WKING].p[1];
    bb_check.p[2] &= abb_b_silver_attacks[SQ_WKING].p[2];
    bb_check.p[1] &= abb_w_silver_attacks[SQ_WKING].p[1];
    bb_check.p[2] &= abb_w_silver_attacks[SQ_WKING].p[2];
    if ( ! BBTest(bb_check) ) { continue; }

    BB_B_BH.p[1]    ^= abb_mask[from].p[1];
    BB_B_BH.p[2]    ^= abb_mask[from].p[2];
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      BBOr( bb_attacks, abb_bishop_attacks_rr45[to][0],
	    abb_bishop_attacks_rl45[to][0] );
      if ( to <= I7 ) {
	bb_attacks.p[0] |= abb_king_attacks[to].p[0];
	bb_attacks.p[1] |= abb_king_attacks[to].p[1];
      }
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
      BB_B_BH.p[1]    ^= abb_mask[from].p[1];
      BB_B_BH.p[2]    ^= abb_mask[from].p[2];
      return ( To2Move(to) | From2Move(from)
	       | ( (to < A6) ? FLAG_PROMO : 0 )
	       | Cap2Move(-BOARD[to]) | Piece2Move(bishop) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
    BB_B_BH.p[1]    ^= abb_mask[from].p[1];
    BB_B_BH.p[2]    ^= abb_mask[from].p[2];
  }

  BBAnd( bb, BB_BTGOLD, b_chk_tbl[SQ_WKING].gold );
  while ( BBTest(bb) ) {
    from = FirstOne( bb );
    Xor( from, bb );

    BBAnd( bb_check, bb_move, abb_b_gold_attacks[from] );
    BBAnd( bb_check, bb_check, abb_w_gold_attacks[SQ_WKING] );
    if ( ! BBTest(bb_check) ) { continue; }

    Xor( from, BB_BTGOLD );
    Xor( from, BB_BOCCUPY );
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = FirstOne( bb_check );
      Xor( to, bb_check );

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      bb_attacks = abb_b_gold_attacks[to];
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      Xor( from, BB_BOCCUPY );
      Xor( from, BB_BTGOLD );
      return ( To2Move(to) | From2Move(from)
	       | Cap2Move(-BOARD[to]) | Piece2Move(BOARD[from]) );
    } while ( BBTest(bb_check) );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    Xor( from, BB_BOCCUPY );
    Xor( from, BB_BTGOLD );
  }

  BBAnd( bb, BB_BSILVER, b_chk_tbl[SQ_WKING].silver );
  while ( bb.p[0] ) {
    from = last_one0( bb.p[0] );
    bb.p[0] ^= abb_mask[from].p[0];

    bb_check_pro.p[0] = bb_move.p[0] & abb_b_silver_attacks[from].p[0]
      & abb_w_gold_attacks[SQ_WKING].p[0];
    bb_check_pro.p[1] = bb_move.p[1] & abb_b_silver_attacks[from].p[1]
      & abb_w_gold_attacks[SQ_WKING].p[1];

    bb_check.p[0] = bb_move.p[0] & abb_b_silver_attacks[from].p[0]
      & abb_w_silver_attacks[SQ_WKING].p[0]
      & ~abb_w_gold_attacks[SQ_WKING].p[0];
    bb_check.p[1] = bb_move.p[1] & abb_b_silver_attacks[from].p[1]
      & abb_w_silver_attacks[SQ_WKING].p[1]
      & ~abb_w_gold_attacks[SQ_WKING].p[1];

    if ( ! ( bb_check_pro.p[0] | bb_check_pro.p[1]
	     | bb_check.p[0]| bb_check.p[1] ) ) { continue; }

    BB_BSILVER.p[0] ^= abb_mask[from].p[0];
    BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    while ( bb_check_pro.p[0] | bb_check_pro.p[1] ) {
      to = first_one01( bb_check_pro.p[0], bb_check_pro.p[1] );
      bb_check_pro.p[0] ^= abb_mask[to].p[0];
      bb_check_pro.p[1] ^= abb_mask[to].p[1];
      
      if ( ! is_white_attacked( ptree, to ) ) { continue; }
      
      bb_attacks = abb_b_gold_attacks[to];
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
      
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
      BB_BSILVER.p[0] ^= abb_mask[from].p[0];
      return ( To2Move(to) | From2Move(from) | FLAG_PROMO
	       | Cap2Move(-BOARD[to]) | Piece2Move(silver) );
    }

    while ( bb_check.p[0] | bb_check.p[1] ) {
      to = first_one01( bb_check.p[0], bb_check.p[1] );
      bb_check.p[0] ^= abb_mask[to].p[0];
      bb_check.p[1] ^= abb_mask[to].p[1];
      
      if ( ! is_white_attacked( ptree, to ) ) { continue; }
      
      bb_attacks = abb_b_silver_attacks[to];
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
      
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
      BB_BSILVER.p[0] ^= abb_mask[from].p[0];
      return ( To2Move(to) | From2Move(from)
	       | Cap2Move(-BOARD[to]) | Piece2Move(silver) );
    }

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
    BB_BSILVER.p[0] ^= abb_mask[from].p[0];
  }

  ubb = bb.p[1] & 0x7fc0000U;
  while ( ubb ) {
    from = last_one1( ubb );
    ubb ^= abb_mask[from].p[1];

    bb_check_pro.p[0] = bb_move.p[0] & abb_b_silver_attacks[from].p[0]
      & abb_w_gold_attacks[SQ_WKING].p[0];

    bb_check.p[0] = bb_move.p[0] & abb_b_silver_attacks[from].p[0]
      & abb_w_silver_attacks[SQ_WKING].p[0]
      & ~abb_w_gold_attacks[SQ_WKING].p[0];
    bb_check.p[1] = bb_move.p[1] & abb_b_silver_attacks[from].p[1]
      & abb_w_silver_attacks[SQ_WKING].p[1];

    if ( ! (bb_check_pro.p[0]|bb_check.p[0]|bb_check.p[1]) ) { continue; }

    BB_BSILVER.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    while ( bb_check_pro.p[0] ) {
      to = last_one0( bb_check_pro.p[0] );
      bb_check_pro.p[0] ^= abb_mask[to].p[0];
      
      if ( ! is_white_attacked( ptree, to ) ) { continue; }
      
      bb_attacks = abb_b_gold_attacks[to];
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
      
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      BB_BSILVER.p[1] ^= abb_mask[from].p[1];
      return ( To2Move(to) | From2Move(from) | FLAG_PROMO
	       | Cap2Move(-BOARD[to]) | Piece2Move(silver) );
    }

    while ( bb_check.p[0] | bb_check.p[1] ) {
      to = first_one01( bb_check.p[0], bb_check.p[1] );
      bb_check.p[0] ^= abb_mask[to].p[0];
      bb_check.p[1] ^= abb_mask[to].p[1];
      
      if ( ! is_white_attacked( ptree, to ) ) { continue; }
      
      bb_attacks = abb_b_silver_attacks[to];
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
      
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      BB_BSILVER.p[1] ^= abb_mask[from].p[1];
      return ( To2Move(to) | From2Move(from)
	       | Cap2Move(-BOARD[to]) | Piece2Move(silver) );
    }

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BSILVER.p[1] ^= abb_mask[from].p[1];
  }

  bb.p[1] &= 0x003ffffU;
  while ( bb.p[1] | bb.p[2] ) {
    from = first_one12( bb.p[1], bb.p[2] );
    bb.p[1] ^= abb_mask[from].p[1];
    bb.p[2] ^= abb_mask[from].p[2];

    bb_check.p[1] = bb_move.p[1] & abb_b_silver_attacks[from].p[1]
      & abb_w_silver_attacks[SQ_WKING].p[1];
    bb_check.p[2] = bb_move.p[2] & abb_b_silver_attacks[from].p[2]
      & abb_w_silver_attacks[SQ_WKING].p[2];
    if ( ! ( bb_check.p[1] | bb_check.p[2] ) ) { continue; }

    BB_BSILVER.p[1] ^= abb_mask[from].p[1];
    BB_BSILVER.p[2] ^= abb_mask[from].p[2];
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    do {
      to = first_one12( bb_check.p[1], bb_check.p[2] );
      bb_check.p[1] ^= abb_mask[to].p[1];
      bb_check.p[2] ^= abb_mask[to].p[2];

      if ( ! is_white_attacked( ptree, to ) ) { continue; }
	
      bb_attacks = abb_b_silver_attacks[to];
      if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
      if ( IsDiscoverWK( from, to ) );
      else if ( can_w_piece_capture( ptree, to ) )       { continue; }
      if ( IsDiscoverBK( from, to ) )                    { continue; }
	
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
      BB_BSILVER.p[1] ^= abb_mask[from].p[1];
      BB_BSILVER.p[2] ^= abb_mask[from].p[2];
      return ( To2Move(to) | From2Move(from)
	       | Cap2Move(-BOARD[to]) | Piece2Move(silver) );
    } while ( bb_check.p[1] | bb_check.p[2] );

    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );
    BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
    BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
    BB_BSILVER.p[1] ^= abb_mask[from].p[1];
    BB_BSILVER.p[2] ^= abb_mask[from].p[2];
  }

  BBAnd( bb, BB_BKNIGHT, b_chk_tbl[SQ_WKING].knight );
  while ( BBTest(bb) ) {
    from = FirstOne( bb );
    Xor( from, bb );

    bb_check.p[0] = bb_move.p[0] & abb_b_knight_attacks[from].p[0]
      & abb_w_gold_attacks[SQ_WKING].p[0];

    if ( bb_check.p[0] ) {
      BB_BKNIGHT.p[0] ^= abb_mask[from].p[0];
      BB_BKNIGHT.p[1] ^= abb_mask[from].p[1];
      BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );

      do {
	to = last_one0( bb_check.p[0] );
	bb_check.p[0] ^= abb_mask[to].p[0];
      
	if ( ! is_white_attacked( ptree, to ) ) { continue; }
      
	bb_attacks = abb_b_gold_attacks[to];
	if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	if ( IsDiscoverWK( from, to ) );
	else if ( can_w_piece_capture( ptree, to ) )       { continue; }
	if ( IsDiscoverBK( from, to ) )                    { continue; }
      
	XorFile( from, OCCUPIED_FILE );
	XorDiag2( from, OCCUPIED_DIAG2 );
	XorDiag1( from, OCCUPIED_DIAG1 );
	BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
	BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
	BB_BKNIGHT.p[0] ^= abb_mask[from].p[0];
	BB_BKNIGHT.p[1] ^= abb_mask[from].p[1];
	return ( To2Move(to) | From2Move(from) | FLAG_PROMO
		 | Cap2Move(-BOARD[to]) | Piece2Move(knight) );
      } while ( bb_check.p[0] );

      XorFile( from, OCCUPIED_FILE );
      XorDiag2( from, OCCUPIED_DIAG2 );
      XorDiag1( from, OCCUPIED_DIAG1 );
      BB_BOCCUPY.p[0] ^= abb_mask[from].p[0];
      BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
      BB_BKNIGHT.p[0] ^= abb_mask[from].p[0];
      BB_BKNIGHT.p[1] ^= abb_mask[from].p[1];
    } else {

      BBAnd( bb_check, bb_move, abb_b_knight_attacks[from] );
      BBAnd( bb_check, bb_check, abb_w_knight_attacks[SQ_WKING] );
      
      if ( BBTest(bb_check) ) {
	BB_BKNIGHT.p[1] ^= abb_mask[from].p[1];
	BB_BKNIGHT.p[2] ^= abb_mask[from].p[2];
	BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
	BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
	XorFile( from, OCCUPIED_FILE );
	XorDiag2( from, OCCUPIED_DIAG2 );
	XorDiag1( from, OCCUPIED_DIAG1 );
	
	do {
	  to = FirstOne( bb_check );
	  Xor( to, bb_check );
      
	  BBIni( bb_attacks );
	  if ( can_w_king_escape( ptree, to, &bb_attacks ) ) { continue; }
	  if ( IsDiscoverWK( from, to ) );
	  else if ( can_w_piece_capture( ptree, to ) )       { continue; }
	  if ( IsDiscoverBK( from, to ) )                    { continue; }
      
	  XorFile( from, OCCUPIED_FILE );
	  XorDiag2( from, OCCUPIED_DIAG2 );
	  XorDiag1( from, OCCUPIED_DIAG1 );
	  BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
	  BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
	  BB_BKNIGHT.p[1] ^= abb_mask[from].p[1];
	  BB_BKNIGHT.p[2] ^= abb_mask[from].p[2];
	  return ( To2Move(to) | From2Move(from)
		   | Cap2Move(-BOARD[to]) | Piece2Move(knight) );
	} while ( BBTest(bb_check) );

	XorFile( from, OCCUPIED_FILE );
	XorDiag2( from, OCCUPIED_DIAG2 );
	XorDiag1( from, OCCUPIED_DIAG1 );
	BB_BOCCUPY.p[1] ^= abb_mask[from].p[1];
	BB_BOCCUPY.p[2] ^= abb_mask[from].p[2];
	BB_BKNIGHT.p[1] ^= abb_mask[from].p[1];
	BB_BKNIGHT.p[2] ^= abb_mask[from].p[2];
      }
    }
  }

  BBAnd( bb, BB_BLANCE, b_chk_tbl[SQ_WKING].lance );
  while ( BBTest(bb) ) {
    from = FirstOne( bb );
    Xor( from, bb );

    bb_attacks = AttackFile(from);
    BBAnd( bb_attacks, bb_attacks, abb_minus_rays[from] );
    BBAnd( bb_attacks, bb_attacks, bb_move );

    BBAnd( bb_check, bb_attacks, abb_mask[SQ_WKING+nfile] );
    bb_check_pro.p[0] = bb_attacks.p[0] & abb_w_gold_attacks[SQ_WKING].p[0];

    if ( ! ( bb_check_pro.p[0] | bb_check.p[0]
	     | bb_check.p[1] | bb_check.p[2] ) ) { continue; }

    Xor( from, BB_BLANCE );
    Xor( from, BB_BOCCUPY );
    XorFile( from, OCCUPIED_FILE );
    XorDiag2( from, OCCUPIED_DIAG2 );
    XorDiag1( from, OCCUPIED_DIAG1 );

    bb_check.p[0] &= 0x1ffU;
    if ( BBTest(bb_check) ) {

      to = SQ_WKING+nfile;
      if ( ! is_white_attacked( ptree, to ) ) {
	bb_check.p[0] &= ~abb_mask[to].p[0];
	goto b_lance_next;
      }