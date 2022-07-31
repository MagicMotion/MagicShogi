// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include "shogi.h"

static void out_CSA_header( const tree_t * restrict ptree, record_t *pr );
static int str2piece( const char *str );
static int skip_comment( record_t *pr );
static int read_char( record_t *pr );
static int read_CSA_line( record_t *pr, char *str );
static int in_CSA_header( tree_t * restrict ptree, record_t *pr, int flag );
static int read_board_rep2( const char *str_line, min_posi_t *pmin_posi );
static int read_board_rep3( const char *str_line, min_posi_t *pmin_posi );

int
read_record( tree_t * restrict ptree, const char *str_file,
	     unsigned int moves, int flag )
{
  record_t record;
  int iret;

  iret = record_open( &record, str_file, mode_read, NULL, NULL );
  if ( iret < 0 ) { return iret; }

  if ( ! moves )
    {
      iret = in_CSA_header( ptree, &record, flag );
      if ( iret < 0 )
	{
	  record_close( &record );
	  return iret;
	}
    }
  else do {
    iret = in_CSA( ptree, &record, NULL, flag );
    if ( iret < 0 )
      {
	record_close( &record );
	return iret;
      }
  } while ( iret != record_next
	    && iret != record_eof
	    && moves > record.moves );
  
  return record_close( &record );
}


int
record_open( record_t *pr, const char *str_file, record_mode_t record_mode,
	     const char *str_name1, const char *str_name2 )
{
  pr->games = pr->moves = pr->lines = 0;
  pr->str_name1[0] = '\0';
  pr->str_name2[0] = '\0';

  if ( str_name1 )
    {
      strncpy( pr->str_name1, str_name1, SIZE_PLAYERNAME-1 );
      pr->str_name1[SIZE_PLAYERNAME-1] = '\0';
    }
  
  if ( str_name2 )
    {
      strncpy( pr->str_name2, str_name2, SIZE_PLAYERNAME-1 );
      pr->str_name2[SIZE_PLAYERNAME-1] = '\0';
    }

  if ( record_mode == mode_write )
    {
      pr->pf = file_open( str_file, "w" );
      if ( pr->pf == NULL ) { return -2; }
    }
  else if ( record_mode == mode_read_write )
    {
      pr->pf = file_open( str_file, "wb+" );
      if ( pr->pf == NULL ) { return -2; }
    }
  else {
    assert( record_mode == mode_read );

    pr->pf = file_open( str_file, "rb" );
    if ( pr->pf == NULL ) { return -2; }
  }

  return 1;
}


int
record_close( record_t *pr )
{
  int iret = file_close( pr->pf );
  pr->pf = NULL;
  return iret;
}


void
out_CSA( tree_t * restrict ptree, record_t *pr, unsigned int move )
{
  const char *str_move;
  unsigned int sec;

  /* print move */
  if ( move == MOVE_RESIGN )
    {
      if ( ! pr->moves ) { out_CSA_header( ptree, pr ); }
      fprintf( pr->pf, "%s\n", str_resign );
      pr->lines++;
    }
  else {
    if ( ! pr->moves )
      {
	root_turn = Flip(root_turn);
	UnMakeMove( root_turn, move, 1 );
	out_CSA_header( ptree, pr );
	MakeMove( root_turn, move, 1 );
	root_turn = Flip(root_turn);
      }
    str_move = str_CSA_move( move );
    fprintf( pr->pf, "%c%s\n", ach_turn[Flip(root_turn)], str_move );
    pr->lines++;
    pr->moves++;
  }

  /* print time */
  sec = root_turn ? sec_b_total : sec_w_total;

  fprintf( pr->pf, "T%-7u,'%03u:%02u \n", sec_elapsed, sec / 60U, sec % 60U );
  pr->lines++;

  /* print repetition or mate status */
  if ( game_status & flag_mated )
    {
      fprintf( pr->pf, "%%TSUMI\n" );
      pr->lines++;
    }
  else if ( game_status & flag_drawn )
    {
      fprintf( pr->pf, "%s\n", str_repetition );
      pr->lines++;
    }

  fflush( pr->pf );
}


int
record_wind( record_t *pr )
{
  char str_line[ SIZE_CSALINE ];
  int iret;
  for (;;)
    {
      iret = read_CSA_line( pr, str_line );
      if ( iret < 0 ) { return iret; }
      if ( ! iret ) { return record_eof; }
      if ( ! strcmp( str_line, "/" ) ) { break; }
    }
  pr->games++;
  pr->moves = 0;
  return record_next;
}


#if ! defined(MINIMUM)
int
record_rewind( record_t *pr )
{
  pr->games = pr->moves = pr->lines = 0;
  if ( fseek( pr->pf, 0, SEEK_SET ) ) { return -2; }

  return 1;
}


int
record_getpos( record_t *pr, rpos_t *prpos )
{
  if ( fgetpos( pr->pf, &prpos->fpos ) )
    {
      str_error = "fgetpos() failed.";
      return -2;
    }
  prpos->games = pr->games;
  prpos->moves = pr->moves;
  prpos->lines = pr->lines;

  return 1;
}


int
record_setpos( record_t *pr, const rpos_t *prpos )
{
  if ( fsetpos( pr->pf, &prpos->fpos ) )
    {
      str_error = "fsetpos() failed.";
      return -2;
    }
  pr->games = prpos->games;
  pr->moves = prpos->moves;
  pr->lines = prpos->lines;

  return 1;
}
#endif /* no MINIMUM */


int
in_CSA( tree_t * restrict ptree, record_t *pr, unsigned int *pmove, int flag )
{
  char str_line[ SIZE_CSALINE ];
  char *ptr;
  unsigned int move;
  long l;
  int iret;
  
  if ( pr->moves == 0 )
    {
      iret = in_CSA_header( ptree, pr, flag );
      if ( iret < 0 ) { return iret; }
    }

  do {
    iret = read_CSA_line( pr, str_line );
    if ( iret < 0 ) { return iret; }
    if ( ! iret ) { return record_eof; }
    if ( ! strcmp( str_line, str_resign ) )
      {
	game_status |= flag_resigned;
	return record_resign;
      }
    if ( ! strcmp( str_line, str_repetition )
	 || ! strcmp( str_line, str_jishogi ) )
      {
	game_status |= flag_drawn;
	return record_drawn;
      }
    if ( ! strcmp( str_line, str_record_error ) )
      {
	return record_error;
      }
  } while ( str_line[0] == 'T' || str_line[0] == '%' );

  if ( ! strcmp( str_line, "/" ) )
    {
      pr->games++;
      pr->moves = 0;
      return record_next;
    }

  if ( game_status & mask_game_end )
    {
      snprintf( str_message, SIZE_MESSAGE, str_fmt_line,
		pr->lines, str_bad_record );
      str_error = str_message;
      return -2;
    }

  iret = interpret_CSA_move( ptree, &move, str_line+1 );
  if ( iret < 0 )
    {
      snprintf( str_message, SIZE_MESSAGE, str_fmt_line,
		pr->lines, str_error );
      str_error = str_message;
      return -2;
    }
  if ( pmove != NULL ) { *pmove = move; }

  /* do time */
  if ( flag & flag_time )
    {
      iret = read_CSA_line( pr, str_line );
      if ( iret < 0 ) { return iret; }
      if ( ! iret )
	{
	  snprintf( str_message, SIZE_MESSAGE, str_fmt_line,
		    pr->lines, str_unexpect_eof );
	  str_error = str_message;
	  return -2;
	}
      if ( str_line[0] != 'T' )
	{
	  snprintf( str_message, SIZE_MESSAGE, str_fmt_line, pr->lines,
		   "Time spent is not available." );
	  str_error = str_message;
	  return -2;
	}
      l = strtol( str_line+1, &ptr, 0 );
      if ( ptr == str_line+1 || l == LONG_MAX || l < 0 )
	{
	  snprintf( str_message, SIZE_MESSAGE, str_fmt_line,
		    pr->lines, str_bad_record );
	  str_error = str_message;
	  return -2;
	}
    }
  else { l = 0; }
  sec_elapsed = (unsigned int)l;
  if ( root_turn ) { sec_w_total += (unsigned int)l; }
  else             { sec_b_total += (unsigned int)l; }

  iret = make_move_root( ptree, move, flag & ~flag_time );
  if ( iret < 0 )
    {
      snprintf( str_message, SIZE_MESSAGE, str_fmt_line,
		pr->lines, str_error );
      str_error = str_message;
      return iret;
    }

  pr->moves++;

  return record_misc;
}


int
interpret_CSA_move( tree_t * restrict ptree, unsigned int *pmove,
		    const char *str )
{
  int ifrom_file, ifrom_rank, ito_file, ito_rank, ipiece;
  int ifrom, ito;
  unsigned int move;
  unsigned int *pmove_last;
  unsigned int *p;

  ifrom_file = str[0]-'0';
  ifrom_rank = str[1]-'0';
  ito_file   = str[2]-'0';
  ito_rank   = str[3]-'0';

  ito_file   = 9 - ito_file;
  ito_rank   = ito_rank - 1;
  ito        = ito_rank * 9 + ito_file;
  ipiece     = str2piece( str+4 );
  if ( ipiece < 0 )
    {
      str_error = str_illegal_move;
      return -2;
    }

  if ( ! ifrom_file && ! ifrom_rank )
    {
      move  = To2Move(ito) | Drop2Move(ipiece);
      ifrom = nsquare;
    }
  else {
    ifrom_file = 9 - ifrom_file;
    ifrom_rank = ifrom_rank - 1;
    ifrom      = ifrom_rank * 9 + ifrom_file;
    if ( abs(BOARD[ifrom]) + promote == ipiece )
      {
	ipiece -= promote;
	move    = FLA