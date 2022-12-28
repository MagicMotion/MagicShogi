// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include "shogi.h"

#if ! defined(MINIMUM)

#define GO_THROUGH_ALL_PARAMETERS_BY_FOO                                  \
  for ( i=0; i < nsquare*pos_n; i++ )           { Foo( pc_on_sq[0][i] ) } \
  for ( i=0; i < nsquare*nsquare*kkp_end; i++ ) { Foo( kkp[0][0][i] ) }


static void rmt( const double avalue[16], int pc );
static void rparam( short *pv, float dv );
static void fv_sym( void );
static int brand( void );
static int make_list( const tree_t * restrict ptree, int list0[52],
		      int list1[52], int anpiece[16], param_t * restrict pd,
		      float f );


void
ini_param( param_t *p )
{
  int i;

  p->pawn       = p->lance      = p->knight     = p->silver     = 0.0;
  p->gold       = p->bishop     = p->rook       = p->pro_pawn   = 0.0;
  p->pro_lance  = p->pro_knight = p->pro_silver = p->horse      = 0.0;
  p->dragon     = 0.0;

#define Foo(x) p->x = 0;
  GO_THROUGH_ALL_PARAMETERS_BY_FOO;
#undef Foo
}


void
add_param( param_t *p1, const param_t *p2 )
{
  int i;

  p1->pawn       += p2->pawn;
  p1->lance      += p2->lance;
  p1->knight     += p2->knight;
  p1->