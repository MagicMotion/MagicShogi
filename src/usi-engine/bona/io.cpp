// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#if defined(_WIN32)
#  include <io.h>
#  include <conio.h>
#else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif
#include "shogi.h"

#if defined(_MSC_VER)
#  include <Share.h>
#  define fopen( file, mode ) _fsopen( file, mode, _SH_DENYNO )
#endif

#if defined(NO_STDOUT) || defined(WIN32_PIPE)
static int out_board0( FILE *pf, int piece, int i, int ito, int ifrom );
#  define OutBoard0(a,b,c,d,e,f) out_board0(a,b,c,d,e)
#else
static int out_board0( FILE *pf, int piece, int i, int ito, int ifrom,
		       int is_promote );
#  define OutBoard0(a,b,c,d,e,f) out_board0(a,b,c,d,e,f)
#endif

static void CONV out_file( FILE *pf, const char *format, ... );
static int CONV check_input_buffer( void );
static int CONV read_command( char **pstr_line_end );
static void CONV out_hand( FILE *pf, unsigned int hand,
			   const char *str_prefix );
static void CONV out_hand0( FILE *pf, int n, const char *str_prefix,
			    const char *str );

#if ! ( defined(NO_STDOUT) && defined(NO_LOGGING) )
void
out( const char *format, ... )
{
  va_list arg;

#  if ! defined(NO_STDOUT)
  if ( !( game_status & flag_nostdout ) )
    {
      va_start( arg, format );
      vprintf( format, arg );
      va_end( arg );
      fflush( stdout );
    }
#  endif

#  if ! defined(NO_LOGGING)
  if ( ( strchr( format, '\n' ) != NULL || strchr( format, '\r' ) == NULL )
       && pf_log != NULL )
    {
      va_start( arg, format );
      vfprintf( pf_log, format, arg ); 
      va_end( arg );
      fflush( pf_log );
    }
#  endif
}
#endif


#if defined(USI)
void CONV
usi_out( const char *format, ... )
{
  va_list arg;

  va_start( arg, format );
  vprintf( format, arg );
  va_end( arg );
  fflush( stdout );

#  if ! defined(NO_LOGGING)
  if ( ( strchr( format, '\n' ) != NULL || strchr( format, '\r' ) == NULL )
       && pf_log != NULL )
    {
      fprintf( pf_log, "OUT: " );
      va_start( arg, format );
      vfprintf( pf_log, format, arg ); 
      va_end( arg );
      fflush( pf_log );
    }
#  endif
}
#endif


#if defined(CSASHOGI)
void
out_csashogi( const char *format, ... )
{
  va_list arg;

  va_start( arg, format );
  vprintf( format, arg );
  va_end( arg );

  fflush( stdout );
}
#endif


void
out_warning( const char *format, ... )
{
  va_list arg;

#if defined(TLP) || defined(DFPN_CLIENT)
  lock( &io_lock );
#endif

  if ( !( game_status & flag_nostdout ) )
    {
      fprintf( stderr, "\n%s", str_warning );
      va_start( arg, format );
      vfprintf( stderr, format, arg );
      va_end( arg );
      fprintf( stderr, "\n\n" );
      fflush( stderr );
    }

#if ! defined(NO_LOGGING)
  if ( pf_log != NULL )
    {
      fprintf( pf_log, "\n%s", str_warning );
      va_start( arg, format );
      vfprintf( pf_log, format, arg ); 
      va_end( arg );
      fprintf( pf_log, "\n\n" );
      fflush( pf_log );
    }
#endif

#if defined(TLP) || defined(DFPN_CLIENT)
  unlock( &io_lock );
#endif
}


void
out_error( const char *format, ... )
{
  va_list arg;
  
  if ( !( game_status & flag_nostdout ) )
    {
      fprintf( stderr, "\nERROR: " );
      va_start( arg, format );
      vfprintf( stderr, format, arg );
      va_end( arg );
      fprintf( stderr, "\n\n" );
      fflush( stderr );
    }

#if ! defined(NO_LOGGING)
  if ( pf_log != NULL )
    {
      fprintf( pf_log, "\nERROR: " );
      va_start( arg, format );
      vfprintf( pf_log, format, arg );
      va_end( arg );
      fprintf( pf_log, "\n\n" );
      fflush( pf_log );
    }
#endif
  
}


FILE *
file_open( const char *str_file, const char *str_mode )
{
  FILE *pf;

  pf = fopen( str_file, str_mode );
  if ( pf == NULL )
    {
      snprintf( str_message, SIZE_MESSAGE,
		"%s, %s", str_fopen_error, str_file );
      str_error = str_message;
      return NULL;
    }
  
  return pf;
}


int
file_close( FILE *pf )
{
  if ( pf == NULL ) { return 1; }
  if ( ferror( pf ) )
    {
      fclose( pf );
      str_error = str_io_error;
      return -2;
    }
  if ( fclose( pf ) )
    {
      str_error = str_io_error;
      return -2;
    }

  return 1;
}


void
show_prompt( void )
{
  if ( game_status & flag_noprompt ) { return; }
  
  if ( game_status & flag_drawn ) { Out( "Drawn> " ); }
  else if ( game_status & flag_mated )
    {
      if ( root_turn ) { Out( "Black Mated> " ); }
      else             { Out( "White Mated> " ); }
    }
  else if ( game_status & flag_resigned )
    {
      if ( root_turn ) { Out( "White Resigned> " ); }
      else             { Out( "Black Resigned> " ); }
    }
  else if ( game_status & flag_suspend )
    {
      if ( root_turn ) { Out( "White Suspend> " ); }
      else             { Out( "Black Suspend> " ); }
    }
  else if ( root_turn ) { Out( "White %d> ", record_game.moves+1 ); }
  else                  { Out( "Black %d> ", record_game.moves+1 ); }
}


int
open_history( const char *str_name1, const char *str_name2 )
{
#if defined(YSS_ZERO)
  return 1;
#endif
#if defined(NO_LOGGING)
//  char str_file[SIZE_FILENAME];
  int iret;

  iret = record_close( &record_game );
  if ( iret < 0 ) { return -1; }

  iret = record_open( &record_game, "game.csa", mode_read_write,
		      str_name1, str_name2 );
  if ( iret < 0 ) { return -1; }

  return 1;
#else
  FILE *pf;
  int i, iret;
  char str_file[SIZE_FILENAME];
  
  if ( record_game.pf != NULL && ! record_game.moves )
    {
      record_game.str_name1[0] = '\0';
      record_game.str_name2[0] = '\0';

      if ( str_name1 )
	{
	  strncpy( record_game.str_name1, str_name1, SIZE_PLAYERNAME-1 );
	  record_game.str_name1[SIZE_PLAYERNAME-1] = '\0';
	}
      
      if ( str_name2 )
	{
	  strncpy( record_game.str_name2, str_name2, SIZE_PLAYERNAME-1 );
	  record_game.str_name2[SIZE_PLAYERNAME-1] = '\0';
	}
      return 1;
    }

  if ( ( ( game_status & flag_nonewlog )
#  if defined(USI)
	 ||  usi_mode != usi_off
#  endif
	 ) && 0 <= record_num )
    {
      iret = record_close( &record_game );
      if ( iret < 0 ) { return -1; }
      
      snprintf( str_file, SIZE_FILENAME, "%s/game%03d.csa",
		str_dir_logs, record_num );
      iret = record_open( &record_game, str_file, mode_read_write,
			  str_name1, str_name2 );
      if ( iret < 0 ) { return -1; }
    }
  else
    {
      iret = file_close( pf_log );
      if ( iret < 0 ) { return -1; }
      
      iret = record_close( &record_game );
      if ( iret < 0 ) { return -1; }
      
      for ( i = 0; i < 999; i++ )
	{
	  snprintf( str_file, SIZE_FILENAME, "%s/game%03d.csa",
		    str_dir_logs, i );
	  pf = file_open( str_file, "r" );
	  if ( pf == NULL ) { break; }
	  iret = file_close( pf );
	  if ( iret < 0 ) { return -1; }
	}
      record_num = i;
      
      snprintf( str_file, SIZE_FILENAME, "%s/n%03d.log",
		str_dir_logs, i );
      pf_log = file_open( str_file, "w" );
      if ( pf_log == NULL ) { return -1; }
      
      snprintf( str_file, SIZE_FILENAME, "%s/game%03d.csa",
		str_dir_logs, i );
      iret = record_open( &record_game, str_file, mode_read_write,
			  str_name1, str_name2 );
      if ( iret < 0 ) { return -1; }
    }
  
  return 1;
#endif
}


int
out_board( const tree_t * restrict ptree, FILE *pf, unsigned int move,
	   int is_strict )
{
  int irank, ifile, i, iret, ito, ifrom;

#if ! defined(WIN32_PIPE)
  int is_promote;
#endif

  if ( game_status & flag_nostdout ) { return 1; }

  if ( ! is_strict && move )
    {
      ito        = I2To( move );
      ifrom      = I2From( move );
#if ! defined(NO_STDOUT) && ! defined(WIN32_PIPE)
      is_promote = I2IsPromote( move );
#endif
    }
  else {
    ito = ifrom = nsquare;
#if ! defined(NO_STDOUT) && ! defined(WIN32_PIPE)
    is_promote = 0;
#endif
  }
  
  if ( ( game_status & flag_reverse ) && ! is_strict )
    {
      fprintf( pf, "          <reversed>        \n" );
      fprintf( pf, "'  1  2  3  4  5  6  7  8  9\n" );

      for ( irank = rank9; irank >= rank1; irank-- )
	{
	  fprintf( pf, "P%d", irank + 1 );
	  
	  for ( ifile = file9; ifile >= file1; ifile-- )
	    {
	      i = irank * nfile + ifile;
	      iret = OutBoard0( pf, BOARD[i], i, ito, ifrom, is_promote );
	      if ( iret < 0 ) { return iret; }
	    }
	  fprintf( pf, "\n" );
	}
    }
  else {
    fprintf( pf, "'  9  8  7  6  5  4  3  2  1\n" );

    for ( irank = rank1; irank <= rank9; irank++ )
      {
	fprintf( pf, "P%d", irank + 1 );
	
	for ( ifile = file1; ifile <= file9; ifile++ )
	  {
	    i = irank * nfile + ifile;
	    iret = OutBoard0( pf, BOARD[i], i, ito, ifrom, is_promote