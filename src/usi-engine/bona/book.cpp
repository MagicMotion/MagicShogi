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
      *pposition += size;
    }
  if ( book_section + size_section <= p ) { return 0; }

  for ( moves = 0, u = BK_SIZE_HEADER; u < size; moves++, u += BK_SIZE_MOVE )
    {
      pbook_move[moves].smove  = *(unsigned short *)(p+u+0);
      pbook_move[moves].freq   = *(unsigned short *)(p+u+2);
    }

  return moves;
}


static ft_t CONV
flip_ft( ft_t ft, int turn, int is_flip )
{
  int ito_rank, ito_file, ifrom_rank, ifrom_file;

  ito_rank = airank[ft.to];
  ito_file = aifile[ft.to];
  if ( ft.from < nsquare )
    {
      ifrom_rank = airank[ft.from];
      ifrom_file = aifile[ft.from];
    }
  else { ifrom_rank = ifrom_file = 0; }

  if ( turn )
    {
      ito_rank = rank9 - ito_rank;
      ito_file = file9 - ito_file;
      if ( ft.from < nsquare )
	{
	  ifrom_rank = rank9 - ifrom_rank;
	  ifrom_file = file9 - ifrom_file;
	}
    }

  if ( is_flip )
    {
      ito_file = file9 - ito_file;
      if ( ft.from < nsquare ) { ifrom_file = file9 - ifrom_file; }
    }

  ft.to = ito_rank * nfile + ito_file;
  if ( ft.from < nsquare ) { ft.from = ifrom_rank * nfile + ifrom_file; }

  return ft;
}


static unsigned int CONV
bm2move( const tree_t * restrict ptree, unsigned int bmove, int is_flip )
{
  ft_t ft;
  unsigned int move;
  int is_promote;

  ft.to      = I2To(bmove);
  ft.from    = I2From(bmove);
  ft         = flip_ft( ft, root_turn, is_flip );
  is_promote = I2IsPromote(bmove);

  move  = (unsigned int)( is_promote | From2Move(ft.from) | ft.to );
  if ( ft.from >= nsquare ) { return move; }

  if ( root_turn )
    {
      move |= Cap2Move(BOARD[ft.to]);
      move |= Piece2Move(-BOARD[ft.from]);
    }
  else {
    move |= Cap2Move(-BOARD[ft.to]);
    move |= Piece2Move(BOARD[ft.from]);
  }

  return move;
}


static uint64_t CONV
book_hash_func( const tree_t * restrict ptree, int *pis_flip )
{
  uint64_t key, key_flip;
  unsigned int hand;
  int i, iflip, irank, ifile, piece;

  key = 0;
  hand = root_turn ? HAND_W : HAND_B;
  i = I2HandPawn(hand);    if ( i ) { key ^= b_hand_pawn_rand[i-1]; }
  i = I2HandLance(hand);   if ( i ) { key ^= b_hand_lance_rand[i-1]; }
  i = I2HandKnight(hand);  if ( i ) { key ^= b_hand_knight_rand[i-1]; }
  i = I2HandSilver(hand);  if ( i ) { key ^= b_hand_silver_rand[i-1]; }
  i = I2HandGold(hand);    if ( i ) { key ^= b_hand_gold_rand[i-1]; }
  i = I2HandBishop(hand);  if ( i ) { key ^= b_hand_bishop_rand[i-1]; }
  i = I2HandRook(hand);    if ( i ) { key ^= b_hand_rook_rand[i-1]; }

  hand = root_turn ? HAND_B : HAND_W;
  i = I2HandPawn(hand);    if ( i ) { key ^= w_hand_pawn_rand[i-1]; }
  i = I2HandLance(hand);   if ( i ) { key ^= w_hand_lance_rand[i-1]; }
  i = I2HandKnight(hand);  if ( i ) { key ^= w_hand_knight_rand[i-1]; }
  i = I2HandSilver(hand);  if ( i ) { key ^= w_hand_silver_rand[i-1]; }
  i = I2HandGold(hand);    if ( i ) { key ^= w_hand_gold_rand[i-1]; }
  i = I2HandBishop(hand);  if ( i ) { key ^= w_hand_bishop_rand[i-1]; }
  i = I2HandRook(hand);    if ( i ) { key ^= w_hand_rook_rand[i-1]; }

  key_flip = key;

  for ( irank = rank1; irank <= rank9; irank++ )
    for ( ifile = file1; ifile <= file9; ifile++ )
      {
	if ( root_turn )
	  {
	    i     = ( rank9 - irank ) * nfile + file9 - ifile;
	    iflip = ( rank9 - irank ) * nfile + ifile;
	    piece = -(int)BOARD[nsquare-i-1];
	  }
	else {
	  i     = irank * nfile + ifile;
	  iflip = irank * nfile + file9 - ifile;
	  piece = (int)BOARD[i];
	}

#define Foo(t_pc)  key      ^= (t_pc ## _rand)[i];     \
                   key_flip ^= (t_pc ## _rand)[iflip];
	switch ( piece )
	  {
	  case  pawn:        Foo( b_pawn );        break;
	  case  lance:       Foo( b_lance );       break;
	  case  knight:      Foo( b_knight );      break;
	  case  silver:      Foo( b_silver );      break;
	  case  gold:        Foo( b_gold );        break;
	  case  bishop:      Foo( b_bishop );      break;
	  case  rook:        Foo( b_rook );        break;
	  case  king:        Foo( b_king );        break;
	  case  pro_pawn:    Foo( b_pro_pawn );    break;
	  case  pro_lance:   Foo( b_pro_lance );   break;
	  case  pro_knight:  Foo( b_pro_knight );  break;
	  case  pro_silver:  Foo( b_pro_silver );  break;
	  case  horse:       Foo( b_horse );       break;
	  case  dragon:      Foo( b_dragon );      break;
	  case -pawn:        Foo( w_pawn );        break;
	  case -lance:       Foo( w_lance );       break;
	  case -knight:      Foo( w_knight );      break;
	  case -silver:      Foo( w_silver );      break;
	  case -gold:        Foo( w_gold );        break;
	  case -bishop:      Foo( w_bishop );      break;
	  case -rook:        Foo( w_rook );        break;
	  case -king:        Foo( w_king );        break;
	  case -pro_pawn:    Foo( w_pro_pawn );    break;
	  case -pro_lance:   Foo( w_pro_lance );   break;
	  case -pro_knight:  Foo( w_pro_knight );  break;
	  case -pro_silver:  Foo( w_pro_silver );  break;
	  case -horse:       Foo( w_horse );       break;
	  case -dragon:      Foo( w_dragon );      break;
	  }
#undef Foo
      }

  if ( key > key_flip )
    {
      key       = key_flip;
      *pis_flip = 1;
    }
  else { *pis_flip = 0; }

  return key;
}


static int CONV
normalize_book_move( book_move_t * restrict pbook_move, int moves )
{
  book_move_t swap;
  double dscale;
  unsigned int u, norm;
  int i, j;

  /* insertion sort by nwin */
  pbook_move[moves].freq = 0;
  for ( i = moves-2; i >= 0; i-- )
    {
      u    = pbook_move[i].freq;
      swap = pbook_move[i];
      for ( j = i+1; pbook_move[j].freq > u; j++ )
	{
	  pbook_move[j-1] = pbook_move[j];
	}
      pbook_move[j-1] = swap;
    }
      
  /* normalization */
  for ( norm = 0, i = 0; i < moves; i++ ) { norm += pbook_move[i].freq; }
  dscale = (double)USHRT_MAX / (double)norm;
  for ( norm = 0, i = 0; i < moves; i++ )
    {
      u = (unsigned int)( (double)pbook_move[i].freq * dscale );
      if ( ! u )           { u = 1U; }
      if ( u > USHRT_MAX ) { u = USHRT_MAX; }
      
      pbook_move[i].freq = (unsigned short)u;
      norm              += u;
    }
  if ( norm > (unsigned int)pbook_move[0].freq + USHRT_MAX )
    {
      str_error = "normalization error";
      return -2;
    }

  pbook_move[0].freq
    = (unsigned short)( pbook_move[0].freq + USHRT_MAX - norm );
  
  return 1;
}


#if ! defined(MINIMUM)

#define MaxNumCell      0x400000
#if defined(BK_SMALL) || defined(BK_TINY)
#  define MaxPlyBook    64
#else
#  define MaxPlyBook    128
#endif

typedef struct {
  unsigned int nwin, ngame, nwin_bnz, ngame_bnz, move;
} record_move_t;

typedef struct {
  uint64_t key;
  unsigned short smove;
  unsigned char result;
} cell_t;

static unsigned int CONV move2bm( unsigned int move, int turn, int is_flip );
static int CONV find_min_cell( const cell_t *pcell, int ntemp );
static int CONV read_a_cell( cell_t *pcell, FILE *pf );
static int compare( const void * p1, const void *p2 );
static int CONV dump_cell( cell_t *pcell, int ncell, int num_tmpfile );
static int CONV examine_game( tree_t * restrict ptree, record_t *pr,
			      int *presult, unsigned int *pmoves );
static int CONV move_selection( const record_move_t *p, int ngame, int nwin );
static int CONV make_cell_csa( tree_t * restrict ptree, record_t *pr,
			       cell_t *pcell, int num_tmpfile );
static int CONV merge_cell( record_move_t *precord_move, FILE **ppf,
			    int num_tmpfile );
static int CONV read_anti_book( tree_t * restrict ptree, record_t * pr );

int CONV
book_create( tree_t * restrict ptree )
{
  record_t record;
  FILE *ppf[101];
  char str_filename[SIZE_FILENAME];
  record_move_t *precord_move;
  cell_t *pcell;
  int iret, num_tmpfile, i, j;


  num_tmpfile = 0;

  pcell = (cell_t*)memory_alloc( sizeof(cell_t) * MaxNumCell );
  if ( pcell == NULL ) { return -2; }

  Out("\n  [book.csa]\n");
  
  iret = record_open( &record, "book.csa", mode_read, NULL, NULL );
  if ( iret < 0 ) { return iret; }

  num_tmpfile = make_cell_csa( ptree, &record, pcell, num_tmpfile );
  if ( num_tmpfile < 0 )
    {
      memory_free( pcell );
      record_close( &record );
      return num_tmpfile;
    }

  iret = record_close( &record );
  if ( iret < 0 )
    {
      memory_free( pcell );
      return iret;
    }

  memory_free( pcell );

  if ( ! num_tmpfile )
    {
      str_error = "No book data";
      return -2;
    }

  if ( num_tmpfile > 100 )
    {
      str_error = "Number of tmp??.bin files are too large.";
      return -2;
    }

  iret = book_off();
