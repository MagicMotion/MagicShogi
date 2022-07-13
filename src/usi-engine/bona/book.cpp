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
  