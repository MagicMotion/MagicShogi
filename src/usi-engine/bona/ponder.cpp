// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdlib.h>
#include <limits.h>
#include "shogi.h"


int
ponder( tree_t * restrict ptree )
{
  const char *str;
  unsigned int move;
  int iret;

  if ( ( game_status & ( mask_game_end | flag_noponder | flag_nopeek ) )
       || abs( last_root_value ) > score_max_eval
       || ! record_game.moves
       || sec_limit_up == UINT_MAX ) { return 1; }

  ponder_nmove = gen_legal_moves( ptree, ponder_move_list, 1 );

  if ( get_elapsed( &time_start ) < 0 ) { return -1; }

  Out( "\nSearch a move to ponder\n\n" );
  OutCsaShogi( "info ponder start\n" );

  game_status |= flag_puzzling;
  iret         = iterate( ptree );
  game_status &= ~flag_puzzling;
  if ( iret < 0 ) { return iret; }

  if ( game_status & ( flag_quit | flag_quit_ponder | flag_suspend ) )
    {
      OutCsaShogi( "info ponder end\n" );
      return 1;
    }

  if ( abs(last_root_value) > score_max_eval )
    {
      OutCsaShogi( "info ponder e