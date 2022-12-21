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
out_error( const char *format, ..