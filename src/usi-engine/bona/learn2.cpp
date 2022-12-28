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
  p1->silver     += p2->silver;
  p1->gold       += p2->gold;
  p1->bishop     += p2->bishop;
  p1->rook       += p2->rook;
  p1->pro_pawn   += p2->pro_pawn;
  p1->pro_lance  += p2->pro_lance;
  p1->pro_knight += p2->pro_knight;
  p1->pro_silver += p2->pro_silver;
  p1->horse      += p2->horse;
  p1->dragon     += p2->dragon;

#define Foo(x) p1->x += p2->x;
  GO_THROUGH_ALL_PARAMETERS_BY_FOO;
#undef Foo
}


void
fill_param_zero( void )
{
  int i;

  p_value[15+pawn]       = 100;
  p_value[15+lance]      = 300;
  p_value[15+knight]     = 300;
  p_value[15+silver]     = 400;
  p_value[15+gold]       = 500;
  p_value[15+bishop]     = 600;
  p_value[15+rook]       = 700;
  p_value[15+pro_pawn]   = 400;
  p_value[15+pro_lance]  = 400;
  p_value[15+pro_knight] = 400;
  p_value[15+pro_silver] = 500;
  p_value[15+horse]      = 800;
  p_value[15+dragon]     = 1000;

#define Foo(x) x = 0;
  GO_THROUGH_ALL_PARAMETERS_BY_FOO;
#undef Foo

  set_derivative_param();
}


void
param_sym( param_t *p )
{
  int q, r, il, ir, ir0, jl, jr, k0l, k0r, k1l, k1r;

  for ( k0l = 0; k0l < nsquare; k0l++ ) {
    q = k0l / nfile;
    r = k0l % nfile;
    k0r = q*nfile + nfile-1-r;
    if ( k0l > k0r ) { continue; }

    for ( il = 0; il < fe_end; il++ ) {
      if ( il < fe_hand_end ) { ir0 = il; }
      else {
	q = ( il- fe_hand_end ) / nfile;
	r = ( il- fe_hand_end ) % nfile;
	ir0 = q*nfile + nfile-1-r + fe_hand_end;
      }

      for ( jl = 0; jl <= il; jl++ ) {
	if ( jl < fe_hand_end )
	  {
	    ir = ir0;
	    jr = jl;
	  }
	else {
	  q = ( jl - fe_hand_end ) / nfile;
	  r = ( jl - fe_hand_end ) % nfile;
	  jr = q*nfile + nfile-1-r + fe_hand_end;
	  if ( jr > ir0 )
	    {
	      ir = jr;
	      jr = ir0;
	    }
	  else { ir = ir0; }
	}
	if ( k0l == k0r && il*(il+1)/2+jl >= ir*(ir+1)/2+jr ) { continue; }

	p->PcPcOnSq(k0l,il,jl)
	  = p->PcPcOnSq(k0r,ir,jr)
	  = p->PcPcOnSq(k0l,il,jl) + p->PcPcOnSq(k0r,ir,jr);
      }
    }
  }

  for ( k0l = 0; k0l < nsquare; k0l++ ) {
    q = k0l / nfile;
    r = k0l % nfile;
    k0r = q*nfile + nfile-1-r;
    if ( k0l > k0r ) { continue; }

    for ( k1l = 0; k1l < nsquare; k1l++ ) {
      q = k1l / nfile;
      r = k1l % nfile;
      k1r = q*nfile + nfile-1-r;
      if ( k0l == k0r && k1l > k1r ) { continue; }

      for ( il = 0; il < kkp_end; il++ ) {
	if ( il < kkp_hand_end ) { ir = il; }
	else {
	  q  = ( il- kkp_hand_end ) / nfile;
	  r  = ( il- kkp_hand_end ) % nfile;
	  ir = q*nfile + nfile-1-r + kkp_hand_end;
	}
	if ( k0l == k0r && k1l == k1r && il >= ir ) { continue; }

	p->kkp[k0l][k1l][il]
	  = p->kkp[k0r][k1r][ir]
	  = p->kkp[k0l][k1l][il] + p->kkp[k0r][k1r][ir];
      }
    }
  }
}


static void fv_sym( void )
{
  int q, r, il, ir, ir0, jl, jr, k0l, k0r, k1l, k1r;

  for ( k0l = 0; k0l < nsquare; k0l++ ) {