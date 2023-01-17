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

	PcPcOnSq(k0l,il,jl) = PcPcOnSq(k0r,ir,jr);
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

	kkp[k0l][k1l][il] = kkp[k0r][k1r][ir];
      }
    }
  }
}


double
calc_penalty( void )
{
  uint64_t u64sum;
  int i;

  u64sum = 0;

#define Foo(x) u64sum += (uint64_t)abs((int)x);
  GO_THROUGH_ALL_PARAMETERS_BY_FOO;
#undef Foo

  return (double)u64sum * FV_PENALTY;
}


void
renovate_param( const param_t *pd )
{
  double *pv[14], *p;
  double v[16];
  unsigned int u32rand, u;
  int i, j;

  v[pawn]       = pd->pawn;         v[lance]      = pd->lance;
  v[knight]     = pd->knight;       v[silver]     = pd->silver;
  v[gold]       = pd->gold;         v[bishop]     = pd->bishop;
  v[rook]       = pd->rook;         v[pro_pawn]   = pd->pro_pawn;
  v[pro_lance]  = pd->pro_lance;    v[pro_knight] = pd->pro_knight;
  v[pro_silver] = pd->pro_silver;   v[horse]      = pd->horse;
  v[dragon]     = pd->dragon;       v[king]       = FLT_MAX;

  pv[ 0] = v + pawn;         pv[ 1] = v + lance;
  pv[ 2] = v + knight;       pv[ 3] = v + silver;
  pv[ 4] = v + gold;         pv[ 5] = v + bishop;
  pv[ 6] = v + rook;         pv[ 7] = v + pro_pawn;
  pv[ 8] = v + pro_lance;    pv[ 9] = v + pro_knight;
  pv[10] = v + pro_silver;   pv[11] = v + horse;
  pv[12] = v + dragon;       pv[13] = v + king;

  /* insertion sort */
  for ( i = 13 - 2; i >= 0; i-- )
    {
      p = pv[i];
      for ( j = i+1; *pv[j] < *p; j++ ) { pv[j-1] = pv[j]; }
      pv[j-1] = p;
    }

  u32rand = rand32();
  u       = u32rand % 7U;
  u32rand = u32rand / 7U;
  p = pv[u+6];  pv[u+6] = pv[12];  pv[12] = p;
  for ( i = 5; i > 0; i-- )
    {
      u       = u32rand % (i+1);
      u32rand = u32rand / (i+1);
      p = pv[u];  pv[u] = pv[i];  pv[i] = p;

      u       = u32rand % (i+1);
      u32rand = u32rand / (i+1);
      p = pv[u+6];  pv[u+6] = pv[i+6];  pv[i+6] = p;
    }

  *pv[ 0] = *pv[ 1] = -2.0;
  *pv[ 2] = *pv[ 3] = *pv[ 4] = -1.0;
  *pv[ 5] = *pv[ 6] = *pv[ 7] =  0.0;
  *pv[ 8] = *pv[ 9] = *pv[10] =  1.0;
  *pv[11] = *pv[12] =  2.0;

  rmt( v, pawn );        rmt( v, lance );       rmt( v, knight );
  rmt( v, silver );      rmt( v, gold );        rmt( v, bishop );
  rmt( v, rook );        rmt( v, pro_pawn );    rmt( v, pro_lance );
  rmt( v, pro_knight );  rmt( v, pro_silver );  rmt( v, horse );
  rmt( v, dragon );

#define Foo(v) rparam( &v, pd->v );
  GO_THROUGH_ALL_PARAMETERS_BY_FOO;
#undef Foo

  fv_sym();
  set_derivative_param();
  ehash_clear();
}


int
out_param( void )
{
  size_t size;
  FILE *pf;
  int apc[17], apv[17];
  int iret, i, j, pc, pv;

  for ( i = 0; i < 17; i++ ) { apc[i] = i;  apv[i] = INT_MAX; }
  apv[pawn]       = p_value_ex[15+pawn];
  apv[lance]      = p_value_ex[15+lance];
  apv[knight]     = p_value_ex[15+knight];
  apv[silver]     = p_value_ex[15+silver];
  apv[gold]       = p_value_ex[15+gold];
  apv[bishop]     = p_value_ex[15+bishop];
  apv[rook]       = p_value_ex[15+rook];
  apv[pro_pawn]   = p_value_ex[15+pro_pawn];
  apv[pro_lance]  = p_value_ex[15+pro_lance];
  apv[pro_knight] = p_value_ex[15+pro_knight];
  apv[pro_silver] = p_value_ex[15+pro_silver];
  apv[horse]      = p_value_ex[15+horse];
  apv[dragon]     = p_value_ex[15+dragon];

  /* insertion sort */
  for ( i = dragon-1; i >= 0; i-- )
    {
      pv = apv[i];  pc = apc[i];
      for ( j = i+1; apv[j] < pv; j++ )
	{
	  apv[j-1] = apv[j];
	  apc[j-1] = apc[j];
	}
      apv[j-1] = pv;  apc[j-1] = pc;
    }
      
  pf = file_open( "param.h_", "w" );
  if ( pf == NULL ) { return -2; }
  
  for ( i = 0; i < 13; i++ )
    {
      fprintf( pf, "#define " );
      switch ( apc[i] )
	{
	case pawn:        fprintf( pf, "DPawn     " );  break;
	case lance:       fprintf( pf, "DLance    " );  break;
	case knight:      fprintf( pf, "DKnight   " );  break;
	case silver:      fprintf( pf, "DSilver   " );  break;
	case gold:        fprintf( pf, "DGold     " );  break;
	case bishop:      fprintf( pf, "DBishop   " );  break;
	case rook:        fprintf( pf, "DRook     " );  break;
	case pro_pawn:    fprintf( pf, "DProPawn  " );  break;
	case pro_lance:   fprintf( pf, "DProLance " );  break;
	case pro_knight:  fprintf( pf, "DProKnight" );  break;
	case pro_silver:  fprintf( pf, "DProSilver" );  break;
	case horse:       fprintf( pf, "DHorse    " );  break;
	case dragon:      fprintf( 