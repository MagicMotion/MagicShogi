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
    if ( ! strcmp( str_line, str_repet