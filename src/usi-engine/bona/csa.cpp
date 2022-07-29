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
record_close( record_t 