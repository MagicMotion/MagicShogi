
// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "shogi.h"

#define PcPcOnSqAny(k,i,j) ( i >= j ? PcPcOnSq(k,i,j) : PcPcOnSq(k,j,i) )

static int CONV doacapt( const tree_t * restrict ptree, int pc, int turn,
			 int hand_index, const int list0[52],
			 const int list1[52], int nlist );
static int CONV doapc( const tree_t * restrict ptree, int pc, int sq,
		       const int list0[52], const int list1[52], int nlist );
static int CONV ehash_probe( uint64_t current_key, unsigned int hand_b,
			     int * restrict pscore );
static void CONV ehash_store( uint64_t key, unsigned int hand_b, int score );
static int CONV calc_difference( const tree_t * restrict ptree, int ply,
				 int turn, int list0[52], int list1[52],
				 int * restrict pscore );
static int CONV make_list( const tree_t * restrict ptree,
			   int * restrict pscore,
			   int list0[52], int list1[52] );

#if defined(INANIWA_SHIFT)
static int inaniwa_score( const tree_t * restrict ptree );
#endif

int CONV
eval_material( const tree_t * restrict ptree )
{
  int material, itemp;

  itemp     = PopuCount( BB_BPAWN )   + (int)I2HandPawn( HAND_B );
  itemp    -= PopuCount( BB_WPAWN )   + (int)I2HandPawn( HAND_W );
  material  = itemp * p_value[15+pawn];

  itemp     = PopuCount( BB_BLANCE )  + (int)I2HandLance( HAND_B );
  itemp    -= PopuCount( BB_WLANCE )  + (int)I2HandLance( HAND_W );
  material += itemp * p_value[15+lance];

  itemp     = PopuCount( BB_BKNIGHT ) + (int)I2HandKnight( HAND_B );
  itemp    -= PopuCount( BB_WKNIGHT ) + (int)I2HandKnight( HAND_W );
  material += itemp * p_value[15+knight];

  itemp     = PopuCount( BB_BSILVER ) + (int)I2HandSilver( HAND_B );
  itemp    -= PopuCount( BB_WSILVER ) + (int)I2HandSilver( HAND_W );
  material += itemp * p_value[15+silver];

  itemp     = PopuCount( BB_BGOLD )   + (int)I2HandGold( HAND_B );
  itemp    -= PopuCount( BB_WGOLD )   + (int)I2HandGold( HAND_W );
  material += itemp * p_value[15+gold];

  itemp     = PopuCount( BB_BBISHOP ) + (int)I2HandBishop( HAND_B );
  itemp    -= PopuCount( BB_WBISHOP ) + (int)I2HandBishop( HAND_W );
  material += itemp * p_value[15+bishop];

  itemp     = PopuCount( BB_BROOK )   + (int)I2HandRook( HAND_B );
  itemp    -= PopuCount( BB_WROOK )   + (int)I2HandRook( HAND_W );
  material += itemp * p_value[15+rook];

  itemp     = PopuCount( BB_BPRO_PAWN );
  itemp    -= PopuCount( BB_WPRO_PAWN );
  material += itemp * p_value[15+pro_pawn];

  itemp     = PopuCount( BB_BPRO_LANCE );
  itemp    -= PopuCount( BB_WPRO_LANCE );
  material += itemp * p_value[15+pro_lance];

  itemp     = PopuCount( BB_BPRO_KNIGHT );
  itemp    -= PopuCount( BB_WPRO_KNIGHT );
  material += itemp * p_value[15+pro_knight];

  itemp     = PopuCount( BB_BPRO_SILVER );
  itemp    -= PopuCount( BB_WPRO_SILVER );
  material += itemp * p_value[15+pro_silver];

  itemp     = PopuCount( BB_BHORSE );
  itemp    -= PopuCount( BB_WHORSE );
  material += itemp * p_value[15+horse];

  itemp     = PopuCount( BB_BDRAGON );
  itemp    -= PopuCount( BB_WDRAGON );
  material += itemp * p_value[15+dragon];

  return material;
}


int CONV
evaluate( tree_t * restrict ptree, int ply, int turn )
{
  int list0[52], list1[52];
  int nlist, score, sq_bk, sq_wk, k0, k1, l0, l1, i, j, sum;

  assert( 0 < ply );
  ptree->neval_called++;

  if ( ptree->save_eval[ply] != INT_MAX )