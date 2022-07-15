// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include "shogi.h"

/*
  Opening Book Data Structure: Index BookData

    Index:         IndexEntry.. (NUM_SECTION times)

      IndexEntry:  SectionPointer SectionSize

    BookData:      Section.. (NUM_SECTION times)

      Section:     [DataEntry]..

        DataEntry: Header Move...


- SectionPointer
  4 byte:  position of the section in character

- SectionSize
  2 byte:  size of the section in character
 
- Header
  1 byte:  number of bytes for the DataEntry
  8 byte:  hash key
  
- Move
  2 byte:  a book move
  2 byte:  frequency
 */

#define BK_SIZE_INDEX     6
#define BK_SIZE_HEADER    9
#define BK_SIZE_MOVE      4
#define BK_MAX_MOVE       32

#if ( BK_SIZE_HEADER + BK_SIZE_MOVE * BK_MAX_MOVE > UCHAR_MAX )
#  error "Maximum size of DataEntry is larger than UCHAR_MAX"
#endif

typedef struct { unsigned short smove, freq; } book_move_t;

typedef struct { int from, to; } ft_t;

static int CONV book_read( uint64_t key, book_move_t *pbook_move,
		      unsigned int *pposition );
static uint64_t CONV book_hash_func( const tree_t * restrict ptree,
				     int *pis_flip );
static unsigned int CONV bm2move( const tree_t * restrict ptree,
				  unsigned int bmove, int is_flip );
static ft_t CONV flip_ft( ft_t ft, int turn, int is_flip );
static int CONV normalize_book_move( book_move_t * restrict pbook_move,
				     int moves );


int CONV
book_on( void )
{
  int iret = file_close( pf_book );
  if ( iret < 0 ) { return iret; }

  pf_book = file_open( str_book, "rb+" );
  if ( pf_book == NULL ) { return -2; }

  return 1;
}


int CONV
book_off( void )
{
  int iret = file_close( pf_book );
  if ( iret < 0 ) { return iret; }

  pf_book = NULL;

  return 1;
}


int CONV
book_probe( tree_t * restrict ptree )
{
  book_move_t abook_move[ BK_MAX_MOVE+1 ];
  uint64_t key;
  double dscore, drand;
  unsigned int move, position, freq_lower_limit;
  int is_flip, i, j, moves, ply;

  key   = book_hash_func( ptree, &is_flip );

  moves = book_read( key, abook_move, &position );
  if ( moves <= 0 ) { return moves; }

#if ! defined(MINIMUM) || ! defined(NDEBUG)
  for ( j = i = 0; i < moves; i++ ) { j += abook_move[i].freq; }
  if ( j != USHRT_MAX )
    {
      str_error = "normalization error (book.bin)";
      return -1;
    }
#endif

  /* decision of book move based on pseudo-random number */
  if ( game_status & flag_puzzling ) { j = 0; }
  else {
    drand = (double)rand64() / (double)UINT64_MAX;

    if ( game_status & flag_narrow_book )
      {
#if defined(BK_ULTRA_NARROW)
	freq_lower_limit = abook_move[0].freq;
#else
	freq_lower_limit = abook_move[0].freq / 2U;
#endif

	for ( i = 1; i < moves; i++ )
	  {
	    if ( abook_move[i].freq < freq_lower_limit ) { break; }
	  }
	moves = i;
	normalize_book_move( abook_move, moves );
      }

    for ( j = moves-1; j > 0; j-- )
      {
	dscore = (double)( abook_move[j].freq ) / (double)USHRT_MAX;
	if ( drand <= dscore ) { break; }
	drand -= dscore;
      }
    if ( ! abook_move[j].freq ) { j = 0; }
  }

  /* show results */
  if ( ! ( game_status & ( flag_pondering | flag_puzzling ) ) )
    {
      Out( "    move     freq\n" );
      OutCsaShogi( "info" );
      for ( i = 0; i < moves; i++ )
	{
	  const char *str;
	  
	  dscore = (double)abook_move[i].freq / (double)USHRT_MAX;
	  move = bm2move( ptree, (unsigned int)abook_move[i].smove, is_flip );
	  str  = str_CSA_move( move );
	  
	  Out( "  %c %s  %5.1f\n", i == j ? '*' : ' ', str, dscore * 100.0 );
	  OutCsaShogi( " %s(%.0f%%)", str, dscore * 100.0 );
	}
      OutCsaShogi( "\n" );
    }

  move = bm2move( ptree, (unsigned int)abook_move[j].smove, is_flip );
  if ( ! is_move_valid( ptree, move, root_turn ) )
    {
      out_warning( "BAD BOOK MOVE!! " );
      return 0;
    }

  ply = record_game.moves;
  if ( game_status & flag_pondering ) { ply++; }

  ptree->current_move[1] = move;

  return 1;
}


static int CONV
book_read( uint64_t key, book_move_t *pbook_move, unsigned int *pposition )
{
  uint64_t book_key;
  const unsigned char *p;
  unsigned int position, size_section, size, u;
  int ibook_section, moves;
  unsigned short s;

  ibook_section = (int)( (unsigned int)key & (unsigned int)( NUM_SECTION-1 ) );

  if ( fseek( pf_book, BK_SIZE_INDEX*ibook_section, SEEK_SET ) == EOF )
    {
      str_error = str_io_error;
      return -2;
    }
  
  if ( fread( &position, sizeof(int), 1, pf_book ) != 1 )
    {
      str_error = str_io_error;
      return -2;
    }
  
  if ( fread( &s, sizeof(unsigned short), 1, pf_book ) != 1 )
    {
      str_error = str_io_error;
      return -2;
    }
  size_section = (unsigned int)s;
  if ( size_section > MAX_SIZE_SECTION )
    {
      str_error = str_book_error;
      return -2;
    }

  if ( fseek( pf_book, (long)position, SEEK_SET ) == EOF )
    {
      str_error = str_io_error;
      return -2;
    }
  if ( fread( book_section, sizeof(unsigned char), (size_t)size_section,
	      pf_book ) != (size_t)size_section )
    {
      str_error = str_io_error;
      return -2;
    }
  
  size       = 0;
  p          = book_section;
  *pposition = position;
  while ( book_section + size_section > p )
    {
      size     = (unsigned int)p[0];
      book_key = *(uint64_t *)( p + 1 );
      if ( book_key == key ) { break; }
      p          += size;
      *ppositi