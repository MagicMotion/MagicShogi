
﻿// 2019 Team AobaZero
// This source code is in the public domain.
#include "../config.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#if !defined(_MSC_VER)
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>

#include "shogi.h"

#include "lock.h"
#include "yss_var.h"
#include "yss_dcnn.h"
#include "process_batch.h"

#include "../GTP.h"
#include "../Utils.h"

int NOT_USE_NN = 0;

// 棋譜と探索木を含めた局面図
//min_posi_t record_plus_ply_min_posi[REP_HIST_LEN];

int nVisitCount = 0;	//30;	// この手数まで最大でなく、回数分布で選ぶ
int nVisitCountSafe = 0;
int fAddNoise = 0;				// always add dirichlet noise on root node.
int fUSIMoveCount;	// USIで上位ｎ手の訪問回数も返す
int fPrtNetworkRawPath = 0;
int fVerbose = 1;
int fClearHashAlways = 0;
int fUsiInfo = 0;
bool fLCB = false;
double MinimumKLDGainPerNode = 0;	//0.000002;	0で無効, lc0は 0.000005
bool fResetRootVisit = false;
bool fDiffRootVisit = false;

int nLimitUctLoop = 100;
double dLimitSec = 0;
int nDrawMove = 0;		// 引き分けになる手数。0でなし。floodgateは256, 選手権は321

const float FIXED_RESIGN_WINRATE = 0.10;	// 自己対戦でこの勝率以下なら投了。0で投了しない。0.10 で勝率10%。 0 <= x <= +1.0
float resign_winrate = 0;

int    fAutoResign = 0;				// 過渡的なフラグ。投了の自動調整ありで、go visit で勝率も返す
double dAutoResignWinrate = 0;

double dSelectRandom = 0;			// この確率で乱数で選んだ手を指す 0 <= x <= 1.0。0 でなし無効
int nHandicapRate[HANDICAP_TYPE];
const int TEMP_RATE_MAX = 1400;		// このレート差まではsoftmaxの温度で調整
char engine_name[SIZE_CMDLINE];

std::vector <HASH_SHOGI> hash_shogi_table;
const int HASH_SHOGI_TABLE_SIZE_MIN = 1024*4*4;
int Hash_Shogi_Table_Size = HASH_SHOGI_TABLE_SIZE_MIN;
int Hash_Shogi_Mask;
int hash_shogi_use = 0;
int hash_shogi_sort_num = 0;
int thinking_age = 0;

const int REHASH_MAX = (2048*1);
const int REHASH_SHOGI = (REHASH_MAX-1);

int rehash[REHASH_MAX-1];	// バケットが衝突した際の再ハッシュ用の場所を求めるランダムデータ
int rehash_flag[REHASH_MAX];	// 最初に作成するために

// 81*81*2 + (81*7) = 13122 + 567 = 13689 * 512 = 7008768.  7MB * 8 = 56MB
//uint64_t sequence_hash_from_to[SEQUENCE_HASH_SIZE][81][81][2];	// [from][to][promote]
//uint64_t sequence_hash_drop[SEQUENCE_HASH_SIZE][81][7];
uint64_t (*sequence_hash_from_to)[81][81][2];
uint64_t (*sequence_hash_drop)[81][7];

int usi_go_count = 0;		// bestmoveを送った直後にstopが来るのを防ぐため
int usi_bestmove_count = 0;

void PRT_sub(const char *fmt, va_list arg)
{
	va_list arg2;
	va_copy(arg2, arg);
	vfprintf(stderr, fmt, arg);

	if ( 0 ) {
		FILE *fp = fopen("aoba_log.txt","a");
		if ( fp ) {
			vfprintf( fp, fmt, arg2);
			fclose(fp);
		}
	}
	va_end(arg2);
}

void PRT(const char *fmt, ...)
{
	if ( fVerbose == 0 ) return;
	va_list arg;
	va_start( arg, fmt );
	PRT_sub(fmt, arg);
	va_end( arg );
}

const int TMP_BUF_LEN = 256*2;
static char debug_str[TMP_BUF_LEN];

void debug_set(const char *file, int line)
{
	char str[TMP_BUF_LEN];
	strncpy(str, file, TMP_BUF_LEN-1);
	const char *p = strrchr(str, '\\');
	if ( p == NULL ) p = file;
	else p++;
	sprintf(debug_str,"%s Line %d\n\n",p,line);
}

void debug_print(const char *fmt, ... )
{
	va_list ap;
	static char text[TMP_BUF_LEN];
	va_start(ap, fmt);
#if defined(_MSC_VER)
	_vsnprintf( text, TMP_BUF_LEN-1, fmt, ap );
#else
	 vsnprintf( text, TMP_BUF_LEN-1, fmt, ap );
#endif
	va_end(ap);
	static char text_out[TMP_BUF_LEN*2];
	sprintf(text_out,"%s%s",debug_str,text);
	PRT("%s\n",text_out);
	debug();
}

#if defined(_MSC_VER)
#include <process.h>
int getpid_YSS() { return _getpid(); }
#else 
int getpid_YSS() { return getpid(); }
#endif

const int CLOCKS_PER_SEC_MS = 1000;	// CLOCKS_PER_SEC を統一。linuxではより小さい.
int get_clock()
{
#if defined(_MSC_VER)
	if ( CLOCKS_PER_SEC_MS != CLOCKS_PER_SEC ) { PRT("CLOCKS_PER_SEC=%d Err. not Windows OS?\n"); debug(); }
	return clock();
#else
	struct timeval  val;
	struct timezone zone;
	if ( gettimeofday( &val, &zone ) == -1 ) { PRT("time err\n"); debug(); }
	return val.tv_sec*1000 + (val.tv_usec / 1000);
#endif
}
double get_diff_sec(int diff_ct)
{
	return (double)diff_ct / CLOCKS_PER_SEC_MS;
}
double get_spend_time(int ct1)
{
	return get_diff_sec(get_clock()+1 - ct1);	// +1をつけて0にならないように。
}

#if 0
// Ｍ系列乱数による方法（アルゴリズム辞典、奥村晴彦著より）周期 2^521 - 1 の他次元分布にも優れた方法
#define M32(x)  (0xffffffff & (x))
static int rnd521_count = 521;	// 初期化忘れの場合のチェック用
static unsigned long mx521[521];

void rnd521_refresh(void)
{
	int i;
	for (i= 0;i< 32;i++) mx521[i] ^= mx521[i+489];
	for (i=32;i<521;i++) mx521[i] ^= mx521[i- 32];
}

void init_rnd521(unsigned long u_seed)
{
	int i,j;
	unsigned long u = 0;
	for (i=0;i<=16;i++) {
		for (j=0;j<32;j++) {
			u_seed = u_seed*1566083941UL + 1;
			u = ( u >> 1 ) | (u_seed & (1UL << 31));	// 最上位bitから32個取り出している
		}
		mx521[i] = u;
	}
	mx521[16] = M32(mx521[16] << 23) ^ (mx521[0] >> 9) ^ mx521[15];
	for (i=17;i<=520;i++) mx521[i] = M32(mx521[i-17] << 23) ^ (mx521[i-16] >> 9) ^ mx521[i-1];
	rnd521_refresh();	rnd521_refresh();	rnd521_refresh();	// warm up
	rnd521_count = 520;
}

unsigned long rand_m521()
{
	if ( ++rnd521_count >= 521 ) { 
		if ( rnd521_count == 522 ) init_rnd521(4357);	// 初期化してなかった場合のFail Safe
		rnd521_refresh();
		rnd521_count = 0;
	}
	return mx521[rnd521_count];
}
#else

std::mt19937 get_mt_rand;

void init_rnd521(unsigned long u_seed)
{
	get_mt_rand.seed(u_seed);
//	PRT("u_seed=%d, mt()=%d\n",u_seed,get_mt_rand());
}
unsigned long rand_m521()
{
	return get_mt_rand();
}
#endif


float f_rnd()
{
//	double f = (double)rand_m521() / (0xffffffffUL + 1.0);	// 0 <= rnd() <  1, ULONG_MAX は gcc 64bitで違う
	double f = (double)rand_m521() / (0xffffffffUL + 0.0);	// 0 <= rnd() <= 1
//	PRT("f_rnd()=%f\n",f);
	return (float)f;
}

void print_path() {}

// from gen_legal_moves()
int generate_all_move(tree_t * restrict ptree, int turn, int ply)
{
	unsigned int * restrict pmove = ptree->move_last[0];
	ptree->move_last[1] = GenCaptures( turn, pmove );
	ptree->move_last[1] = GenNoCaptures( turn, ptree->move_last[1] );
	ptree->move_last[1] = GenCapNoProEx2( turn, ptree->move_last[1] );
	ptree->move_last[1] = GenNoCapNoProEx2( turn, ptree->move_last[1] );
	ptree->move_last[1] = GenDrop( turn, ptree->move_last[1] );
	int num_move = (int)( ptree->move_last[1] - pmove );
	int i;
	for (i = 0; i < num_move; i++) {
		MakeMove( turn, pmove[i], ply );
		if ( InCheck(turn) ) {
			UnMakeMove( turn, pmove[i], ply );
			pmove[i] = 0;
			continue;
		}
		UnMakeMove( turn, pmove[i], ply );
	}
	int num_legal = 0;
	for (i = 0; i < num_move; i++) {
		if ( pmove[i]==0 ) continue;
		pmove[num_legal++] = pmove[i];
	}
//	PRT("num_legal=%d/%d,ply=%d\n",num_legal,num_move,ply);
	if ( num_legal > SHOGI_MOVES_MAX ) { PRT("num_legal=%d Err\n",num_legal); debug(); }
	return num_legal;
}

int is_drop_pawn_mate(tree_t * restrict ptree, int turn, int ply)
{
	int move_num = generate_all_move( ptree, turn, ply );
	unsigned int * restrict pmove = ptree->move_last[0];
	int i;
	for ( i = 0; i < move_num; i++ ) {
		int move = pmove[i];
		int tt = root_turn;
		if ( ! is_move_valid( ptree, move, tt ) ) {
			PRT("illegal move?=%08x\n",move);
		}
		int not_mate = 0;
		MakeMove( tt, move, ply );
		if ( InCheck(tt) ) {
			PRT("illegal. check\n");
		} else {
			not_mate = 1;
		}
		UnMakeMove( tt, move, ply );
		if ( not_mate ) return 0;
	}
	return 1;
}

const int USI_BESTMOVE_LEN = MAX_LEGAL_MOVES*(8+5)+10;

int YssZero_com_turn_start( tree_t * restrict ptree )
{
	if ( 0 ) {
		if ( ptree->nrep > 180 ) {
			dLimitSec = 1.8;
		} else {
			dLimitSec = 4.8;
		}
	}

	if ( 0 ) {
		int ct1 = get_clock();
		int i;
		for(i=0;i<10000;i++) {
			generate_all_move( ptree, root_turn, 1 );
//			make_root_move_list( ptree );	// 100回で12秒, root は探索してる
		}
		PRT("%.2f sec\n",get_spend_time(ct1));
	}

	int ply = 1;	// 1 から始まる
/*
	int move_num = generate_all_move( ptree, root_turn );
	PRT("move_num=%d,root_turn=%d,nrep=%d\n",move_num,root_turn,ptree->nrep);

	unsigned int * restrict pmove = ptree->move_last[0];
	int i;
	for ( i = 0; i < move_num; i++ ) {
//		int move = root_move_list[i].move;
		int move = pmove[i];
  
		int tt = root_turn;
		if ( ! is_move_valid( ptree, move, tt ) ) {
			PRT("illegal move?=%08x\n",move); debug();
		}
		int from = (int)I2From(move);
		int to   = (int)I2To(move);
		int cap  = (int)UToCap(move);
		int drop = (int)From2Drop(from);
		int piece_m	= (int)I2PieceMove(move);
		int is_promote = (int)I2IsPromote(move);
		PRT("%3d:%s(%d), from=%2d,to=%2d,cap=%2d,drop=%3d,is_promote=%d,peice_move=%d\n",i,str_CSA_move(move),tt,from,to,cap,drop,is_promote,piece_m);
		
		MakeMove( tt, move, ply );
		if ( InCheck(tt) ) PRT("illegal. check\n");
		if ( InCheck(Flip(tt)) ) {
			PRT("check\n");
//			if ( drop == pawn && is_drop_pawn_mate( ptree, Flip(tt), ply+1 ) ) {	// 打歩詰？
//				PRT("drop pawn check_mate!\n");	// BonaはGenDropで認識して生成しない？ -> しないね。かしこい。
//			}
		}
//		tt = Flip(tt);
		UnMakeMove( tt, move, ply );
	}
*/
	char buf_move_count[USI_BESTMOVE_LEN];
	int m = uct_search_start( ptree, root_turn, ply, buf_move_count );

	char buf[7];
	if ( m == 0 ) {
		sprintf(buf,"%s","resign");
	} else {
		csa2usi( ptree, str_CSA_move(m), buf );
	}
	char str_best[USI_BESTMOVE_LEN+11+7];
	if ( fUSIMoveCount ) {
		sprintf( str_best,"bestmove %s,%s\n",buf,buf_move_count );
	} else {
		sprintf( str_best,"bestmove %s\n",   buf );
	}
	if ( fUsiInfo && is_declare_win_root(ptree, root_turn) ) {
		sprintf( str_best,"bestmove win\n");
	}

	set_latest_bestmove(str_best);

	if ( 0 && m ) {	// test fClearHashAlways
		char buf_tmp[USI_BESTMOVE_LEN];
		make_move_root( ptree, m, 0 );
		uct_search_start( ptree, root_turn, ply, buf_tmp );
	}

	send_latest_bestmove();
	return 1;
}

char latest_bestmove[USI_BESTMOVE_LEN] = "bestmove resign\n";
void set_latest_bestmove(char *str)
{
	strcpy(latest_bestmove,str);
}
void send_latest_bestmove()
{
	usi_bestmove_count++;
	USIOut( "%s", latest_bestmove);
}

void init_seqence_hash()
{
	static int fDone = 0;
	if ( fDone ) return;
	fDone = 1;

	sequence_hash_from_to = (uint64_t(*)[81][81][2])malloc( SEQUENCE_HASH_SIZE*81*81*2 * sizeof(uint64_t) );
	sequence_hash_drop    = (uint64_t(*)[81][7])    malloc( SEQUENCE_HASH_SIZE*81*7    * sizeof(uint64_t) );
	if ( sequence_hash_from_to == NULL || sequence_hash_drop == NULL ) { PRT("Fail sequence_hash malloc()\n"); debug(); }

	int m,i,j,k;
	for (m=0;m<SEQUENCE_HASH_SIZE;m++) {
		for (i=0;i<81;i++) {
			for (j=0;j<81;j++) {
				for (k=0;k<2;k++) {
					sequence_hash_from_to[m][i][j][k] = ((uint64)(rand_m521()) << 32) | rand_m521();
//					if ( i==0 && j==0 ) { PRT("%016" PRIx64 ",",sequence_hash_from_to[m][i][j][k]); if ( k==1 ) PRT("\n"); }
				}
			}
			for (j=0;j<7;j++) {
				sequence_hash_drop[m][i][j] = ((uint64)(rand_m521()) << 32) | rand_m521();
//				PRT("%016" PRIx64 ",",sequence_hash_drop[m][i][j]);
			}
		}
	}
}

uint64_t get_sequence_hash_from_to(int moves, int from, int to, int promote)
{
	uint64_t ret = 0;
	if ( moves < 0 || moves >= SEQUENCE_HASH_SIZE || from < 0 || from >= 81 || to < 0 || to >= 81 || promote < 0 || promote >= 2 ) { PRT("Err. sequence move\n"); debug(); }
	if ( is_process_batch() ) {
		ret = get_process_mem(moves * (81*81*2) + from * (81*2) + to * (2) + promote);
	} else {
		ret = sequence_hash_from_to[moves][from][to][promote];
	}
//	PRT("moves=%3d(%3d),from=%2d,to=%2d,prom=%d,%016" PRIx64 "\n",moves,moves & (SEQUENCE_HASH_SIZE-1),from,to,promote,ret);
	return ret;
}
uint64_t get_sequence_hash_drop(int moves, int to, int piece)
{
	if ( moves < 0 || moves >= SEQUENCE_HASH_SIZE || to < 0 || to >= 81 || piece < 0 || piece >= 7 ) { PRT("Err. sequence drop\n"); debug(); }
	if ( is_process_batch() ) {
		return get_process_mem(moves * (81*7) + to * (7) + piece + (SEQUENCE_HASH_SIZE*81*81*2));
	} else {
		return sequence_hash_drop[moves][to][piece];
	}
}


void set_Hash_Shogi_Table_Size(int playouts)
{
	int n = playouts * 3;
	
	Hash_Shogi_Table_Size = HASH_SHOGI_TABLE_SIZE_MIN;
	for (;;) {
		if ( Hash_Shogi_Table_Size > n ) break;
		Hash_Shogi_Table_Size *= 2;
	}
}

void hash_shogi_table_reset()
{
	for (int i=0;i<Hash_Shogi_Table_Size;i++) {
		HASH_SHOGI *pt = &hash_shogi_table[i];
		pt->deleted = 1;
		LockInit(pt->entry_lock);
//		pt->lock = false;
#ifdef CHILD_VEC
		std::vector<CHILD>().swap(pt->child);	// memory free hack for vector. 
#endif
	}
	hash_shogi_use = 0;
}

void hash_shogi_table_clear()
{
	Hash_Shogi_Mask       = Hash_Shogi_Table_Size - 1;
	HASH_ALLOC_SIZE size = sizeof(HASH_SHOGI) * Hash_Shogi_Table_Size;
	hash_shogi_table.resize(Hash_Shogi_Table_Size);	// reserve()だと全要素のコンストラクタが走らないのでダメ
	PRT("HashShogi=%7d(%3dMB),sizeof(HASH_SHOGI)=%d,Hash_SHOGI_Mask=%d\n",Hash_Shogi_Table_Size,(int)(size/(1024*1024)),sizeof(HASH_SHOGI),Hash_Shogi_Mask);
	hash_shogi_table_reset();
}

void inti_rehash()
{
	int i = 0;
	rehash_flag[0] = 1;	// 0 は使わない
	for ( ;; ) {
		int k = (rand_m521() >> 8) & (REHASH_MAX-1);
		if ( rehash_flag[k] ) continue;	// 既に登録済み
		rehash[i] = k;
		rehash_flag[k] = 1;
		i++;
		if ( i == REHASH_MAX-1 ) break;
	}
//	for (i=0;i<REHASH_MAX-1;i++) PRT("%08x,",rehash[i]);
}

int IsHashFull()
{
	if ( hash_shogi_use >= Hash_Shogi_Table_Size*90/100 ) {
		PRT("hash full! hash_shogi_use=%d,Hash_Shogi_Table_Size=%d\n",hash_shogi_use,Hash_Shogi_Table_Size);
		return 1;
	}
	return 0; 
}
void all_hash_go_unlock()
{
	for (int i=0;i<Hash_Shogi_Table_Size;i++) UnLock(hash_shogi_table[i].entry_lock);
}

uint64 get_marge_hash(tree_t * restrict ptree, int sideToMove)
{
	uint64 key = HASH_KEY ^ HAND_B;
	if ( ! sideToMove ) key = ~key;
	return key;
};

void hash_half_del(tree_t * restrict ptree, int sideToMove)
{
	uint64 hash64pos  = get_marge_hash(ptree, sideToMove);
	uint64 hashcode64 = ptree->sequence_hash;

	int i,sum = 0;
	for (i=0;i<Hash_Shogi_Table_Size;i++) if ( hash_shogi_table[i].deleted==0 ) sum++;
	if ( sum != hash_shogi_use ) PRT("warning! sum=%d,hash_shogi_use=%d\n",sum,hash_shogi_use);
	hash_shogi_use = sum;	// hash_shogi_useはロックしてないので12スレッドだと頻繁にずれる

	int max_sum = 0;
	int del_games = max_sum * 5 / 10000;	// 0.05%以上。5%程度残る。メモリを最大限まで使い切ってる場合のみ。age_minus = 2 に。

	const double limit_occupy = 50;		// 50%以上空くまで削除
	const int    limit_use    = (int)(limit_occupy*Hash_Shogi_Table_Size / 100);
	int del_sum=0,age_minus = 4;
	for (;age_minus>=0;age_minus--) {
		for (i=0;i<Hash_Shogi_Table_Size;i++) {
			HASH_SHOGI *pt = &hash_shogi_table[i];
			int del = 0;
			if ( pt->deleted == 0 && hashcode64 == pt->hashcode64 && hash64pos == pt->hash64pos ) {
//				PRT("root node, del hash\n");
//				del = 1;
			}
			if ( pt->deleted == 0 && (pt->age <= thinking_age - age_minus || pt->games_sum < del_games) ) {
				del = 1;
			}
			if ( del ) {
#ifdef CHILD_VEC
				std::vector<CHILD>().swap(pt->child);	// memory free hack for vector. 
#else
//				memset(pt,0,sizeof(HASH_SHOGI));
#endif
				pt->deleted = 1;
				hash_shogi_use--;
				del_sum++;
			}
//			if ( hash_go_use < limit_use ) break;	// いきなり10分予測読みして埋めてしまっても全部消さないように --> 前半ばっかり消して再ハッシュでエラーになる。
		}
		double occupy = hash_shogi_use*100.0/Hash_Shogi_Table_Size;
		PRT("hash del=%d,age=%d,minus=%d, %.0f%%(%d/%d)\n",del_sum,thinking_age,age_minus,occupy,hash_shogi_use,Hash_Shogi_Table_Size);
		if ( hash_shogi_use < limit_use ) break;
		if ( age_minus==0 ) { PRT("age_minus=0\n"); debug(); }
	}
}

#if 0
template<class T>
void atomic_lock(std::atomic<T> &f) {
	T old = f.load();
	while (!f.compare_exchange_weak(old, old + 1));
}
template<class T>
void atomic_unlock(std::atomic<T> &f) {
	f.store(0);
}

void ttas_lock(std::atomic<bool> &f)
{
	do {	// Test and Test-And-Set
		while ( f.load() ) continue;
	} while ( f.exchange(true) );
}
void ttas_sleep_lock(std::atomic<bool> &f)
{
	int min_delay = 100;
	int max_delay = 1600;
	int limit = min_delay;
	do {
		if ( f.load() ) {
			int delay = rand_m521() % limit;
			limit *= 2;
			if ( limit > max_delay ) limit = max_delay;
			std::this_thread::sleep_for(std::chrono::nanoseconds(delay));
			continue;
		}
	} while ( f.exchange(true) );
}
void ttas_unlock(std::atomic<bool> &f)
{
	f.store(false);
}


HASH_SHOGI* HashShogiReadLock(tree_t * restrict ptree, int sideToMove)
{
research_empty_block:
	int n,first_n,loop = 0;

	uint64 hash64pos  = get_marge_hash(ptree, sideToMove);
	uint64 hashcode64 = ptree->sequence_hash;
//	PRT("ReadLock hash=%016" PRIx64 "\n",hashcode64);

	n = (int)hashcode64 & Hash_Shogi_Mask;
	first_n = n;
	const int TRY_MAX = 8;

	HASH_SHOGI *pt_first = NULL;

	for (;;) {
		HASH_SHOGI *pt = &hash_shogi_table[n];

//		Lock(pt->entry_lock);		// Lockをかけっぱなしにするように
		ttas_sleep_lock(pt->lock);
		if ( pt->deleted == 0 ) {
			if ( hashcode64 == pt->hashcode64 && hash64pos == pt->hash64pos ) {
				return pt;
			}
		} else {
			if ( pt_first == NULL ) pt_first = pt;
		}
		ttas_unlock(pt->lock);
//		UnLock(pt->entry_lock);

		// 違う局面だった
		if ( loop == REHASH_SHOGI ) break;	// 見つからず
		if ( loop >= TRY_MAX && pt_first ) break;	// 妥協。TRY_MAX回探してなければ未登録扱い。
		n = (rehash[loop++] + first_n ) & Hash_Shogi_Mask;
	}
	if ( pt_first ) {
		// 検索中に既にpt_firstが使われてしまっていることもありうる。もしくは同時に同じ場所を選んでしまうケースも。
		ttas_sleep_lock(pt_first->lock);
//		Lock(pt_first->entry_lock);
		if ( pt_first->deleted == 0 ) {	// 先に使われてしまった！
//			UnLock(pt_first->entry_lock);
			ttas_unlock(pt_first->lock);
			goto research_empty_block;
		}
		ttas_unlock(pt_first->lock);
		return pt_first;	// 最初にみつけた削除済みの場所を利用
	}
	int sum = 0;
	for (int i=0;i<Hash_Shogi_Table_Size;i++) { sum = hash_shogi_table[i].deleted; PRT("%d",hash_shogi_table[i].deleted); }
	PRT("\nno child hash Err loop=%d,hash_shogi_use=%d,first_n=%d,del_sum=%d(%.1f%%)\n",loop,hash_shogi_use,first_n,sum, 100.0*sum/Hash_Shogi_Table_Size); debug(); return NULL;
}
#endif




HASH_SHOGI* HashShogiReadLock(tree_t * restrict ptree, int sideToMove)
{
research_empty_block:
	int n,first_n,loop = 0;

	uint64 hash64pos  = get_marge_hash(ptree, sideToMove);
	uint64 hashcode64 = ptree->sequence_hash;
//	PRT("ReadLock hash=%016" PRIx64 "\n",hashcode64);

	n = (int)hashcode64 & Hash_Shogi_Mask;
	first_n = n;
	const int TRY_MAX = 8;

	HASH_SHOGI *pt_first = NULL;

	for (;;) {
		HASH_SHOGI *pt = &hash_shogi_table[n];
		Lock(pt->entry_lock);		// Lockをかけっぱなしにするように
		if ( pt->deleted == 0 ) {
			if ( hashcode64 == pt->hashcode64 && hash64pos == pt->hash64pos ) {
				return pt;
			}
		} else {
			if ( pt_first == NULL ) pt_first = pt;
		}

		UnLock(pt->entry_lock);
		// 違う局面だった
		if ( loop == REHASH_SHOGI ) break;	// 見つからず
		if ( loop >= TRY_MAX && pt_first ) break;	// 妥協。TRY_MAX回探してなければ未登録扱い。
		n = (rehash[loop++] + first_n ) & Hash_Shogi_Mask;
	}
//	{ static int count, loop_sum; count++; loop_sum+=loop; PRT("%d,",loop); if ( (count%100)==0 ) PRT("loop_ave=%.1f\n",(float)loop_sum/count); }
	if ( pt_first ) {
		// 検索中に既にpt_firstが使われてしまっていることもありうる。もしくは同時に同じ場所を選んでしまうケースも。
		Lock(pt_first->entry_lock);
		if ( pt_first->deleted == 0 ) {	// 先に使われてしまった！
			UnLock(pt_first->entry_lock);
			goto research_empty_block;
		}
		return pt_first;	// 最初にみつけた削除済みの場所を利用
	}
	int sum = 0;
	for (int i=0;i<Hash_Shogi_Table_Size;i++) { sum = hash_shogi_table[i].deleted; PRT("%d",hash_shogi_table[i].deleted); }
	PRT("\nno child hash Err loop=%d,hash_shogi_use=%d,first_n=%d,del_sum=%d(%.1f%%)\n",loop,hash_shogi_use,first_n,sum, 100.0*sum/Hash_Shogi_Table_Size); debug(); return NULL;
}

const int PV_CSA = 0;
const int PV_USI = 1;

char *prt_pv_from_hash(tree_t * restrict ptree, int ply, int sideToMove, int fusi_str)
{
	static char str[TMP_BUF_LEN];
	if ( ply==1 ) str[0] = 0;
	HASH_SHOGI *phg = HashShogiReadLock(ptree, sideToMove);
	UnLock(phg->entry_lock);
	if ( phg->deleted ) return str;
//	if ( phg->hashcode64 != get_marge_hash(ptree, sideToMove) ) return str;
	if ( phg->hashcode64 != ptree->sequence_hash || phg->hash64pos != get_marge_hash(ptree, sideToMove) ) return str;
	if ( ply > 30 ) return str;

	int max_i = -1;
	int max_games = 0;
	int i;
	for (i=0;i<phg->child_num;i++) {
		CHILD *pc = &phg->child[i];
		if ( pc->games > max_games ) {
			max_games = pc->games;
			max_i = i;
		}
	}
	if ( max_i >= 0 ) {
		CHILD *pc = &phg->child[max_i];
		if ( ply > 1 ) strcat(str," ");

		if ( fusi_str ) {
			char buf[7];
			csa2usi( ptree, str_CSA_move(pc->move), buf );
			strcat(str,buf);
		} else {
			const char *sg[2] = { "-", "+" };
			strcat(str,sg[(root_turn + ply) & 1]);
			strcat(str,str_CSA_move(pc->move));
		}
		MakeMove( sideToMove, pc->move, ply );

		prt_pv_from_hash(ptree, ply+1, Flip(sideToMove), fusi_str);
		UnMakeMove( sideToMove, pc->move, ply );
	}
	return str;
}


int search_start_ct;
int stop_search_flag = 0;

void set_stop_search()
{
	stop_search_flag = 1;
}
int is_stop_search()
{
	return stop_search_flag;
}

std::mutex g_mtx;

std::atomic<int> uct_count(0);

int inc_uct_count()
{
	int count = uct_count.fetch_add(+1) + 1;
	if ( count >= nLimitUctLoop + 1 - (int)cfg_num_threads ) set_stop_search();
	return count;
}

int is_main_thread(tree_t * restrict ptree)
{
	return ( ptree == &tlp_atree_work[0] );
}
int get_thread_id(tree_t * restrict ptree)
{
	int i;
	for (i=0; i<(int)cfg_num_threads; i++) {
		if ( ptree == &tlp_atree_work[i] ) return i;
	}
	DEBUG_PRT("Err. get_thread_id()\n");
	return -1;
}
int is_limit_sec()
{
	if ( dLimitSec == 0 ) return 0;
	if ( get_spend_time(search_start_ct) >= dLimitSec ) return 1;
	return 0;
}

bool is_do_mate3() { return false; }
bool is_use_exact() { return false; }	// 有効だと駒落ちで最後の1手詰を見つけたら高確率でその1手だけを探索した、と扱われる

void uct_tree_loop(tree_t * restrict ptree, int sideToMove, int ply)
{
	ptree->sum_reached_ply = 0;
	ptree->max_reached_ply = 0;
	for (;;) {
		ptree->reached_ply = 0;
		int exact_value = EX_NONE;
		uct_tree(ptree, sideToMove, ply, &exact_value);
		ptree->sum_reached_ply += ptree->reached_ply;
		if ( ptree->reached_ply > ptree->max_reached_ply ) ptree->max_reached_ply = ptree->reached_ply;
		int count = inc_uct_count();
		if ( is_main_thread(ptree) ) {
			if ( is_send_usi_info(0) ) send_usi_info(ptree, sideToMove, ply, count, (int)(count/get_spend_time(search_start_ct)));
			if ( check_stop_input() == 1 ) set_stop_search();
			if ( IsHashFull() ) set_stop_search();
			if ( is_limit_sec() ) set_stop_search();
			if ( isKLDGainSmall(ptree, sideToMove) ) set_stop_search();
		}
		if ( is_stop_search() ) break;
		if ( is_use_exact() && (exact_value == EX_WIN || exact_value == EX_LOSS) ) break;
	}
}

int uct_search_start(tree_t * restrict ptree, int sideToMove, int ply, char *buf_move_count)
{
	if ( fClearHashAlways ) {
		hash_shogi_table_clear();
	} else {
		thinking_age = (thinking_age + 1) & 0x7ffffff;
		if ( thinking_age == 0 ) thinking_age = 1;
		if ( thinking_age == 1 ) {
			hash_shogi_table_clear();
		} else {
			hash_half_del(ptree, sideToMove);
		}
	}
	
	HASH_SHOGI *phg = HashShogiReadLock(ptree, sideToMove);
	create_node(ptree, sideToMove, ply, phg);
	UnLock(phg->entry_lock);
//	int keep_root_games[MAX_LEGAL_MOVES];
	if ( fDiffRootVisit ) {
//		for (int i=0; i<phg->child_num; i++) keep_root_games[i] = phg->child[i].games;
	}
	if ( fResetRootVisit ) {
		for (int i=0; i<phg->child_num; i++) phg->child[i].games = 0;
	}
	const bool fPolicyRealization = false;
	float keep_root_policy[MAX_LEGAL_MOVES];
	if ( fPolicyRealization ) {
		for (int i=0; i<phg->child_num; i++) keep_root_policy[i] = phg->child[i].bias;
	}

	const float epsilon = 0.25f;	// epsilon = 0.25
	const float alpha   = 0.15f;	// alpha ... Chess = 0.3, Shogi = 0.15, Go = 0.03
	if ( fAddNoise ) add_dirichlet_noise(epsilon, alpha, phg);
//{ void test_dirichlet_noise(float epsilon, float alpha);  test_dirichlet_noise(0.25f, 0.03f); }
	PRT("root phg->hash=%" PRIx64 ", child_num=%d\n",phg->hashcode64,phg->child_num);

	init_KLDGain_prev_dist_visits_total(phg->games_sum);

	search_start_ct = get_clock();

	int thread_max = cfg_num_threads;
	std::vector<std::thread> ths(thread_max);
	uct_count = 0;
	stop_search_flag = 0;
	int i;
	for (i=1;i<thread_max;i++) {
		init_state( &tlp_atree_work[0], &tlp_atree_work[i]);
//		tlp_atree_work[i] = tlp_atree_work[0];	// lock_init()してるものがあるのでダメ
	}
	for (i=0;i<thread_max;i++) {
		ths[i] = std::thread(uct_tree_loop, &tlp_atree_work[i], sideToMove, ply);
	}

	for (std::thread& th : ths) {
		th.join();
	}

	int sum_r_ply = 0;
	int max_r_ply = 0;
	for (i=0;i<thread_max;i++) {
		tree_t * restrict ptree = &tlp_atree_work[i];
		sum_r_ply += ptree->sum_reached_ply;
		if ( ptree->max_reached_ply > max_r_ply ) max_r_ply = ptree->max_reached_ply;
	}

	int playouts = uct_count.load();
	double ave_reached_ply = (double)sum_r_ply / (playouts + (playouts==0));
	double ct = get_spend_time(search_start_ct);

	// select best
	int best_move = 0;
	int max_i = -1;
	int max_games = 0;
	const int SORT_MAX = MAX_LEGAL_MOVES;	// 593
	int sort_n = 0;
	bool found_mate = false;
	const float LARGE_NEGATIVE_VALUE = -1e6f;
	float max_lcb = LARGE_NEGATIVE_VALUE;
	typedef struct SORT_LCB {	// LCBを使わなくてもいったん代入
		int   move;
		int   games;
		float lcb;
		int   index;
	} SORT_LCB;
	SORT_LCB sort_lcb[SORT_MAX];	// 勝率、LCB、ゲーム数、着手、元のindexを保存

	if ( is_use_exact() ) for (i=0;i<phg->child_num;i++) {
		CHILD *pc = &phg->child[i];
		if ( pc->exact_value != EX_WIN ) continue;
		if ( pc->games == 0 ) DEBUG_PRT("");
		max_games = pc->games;
		max_i = i;

		SORT_LCB *p = &sort_lcb[sort_n];
		p->move  = pc->move;
		p->games = pc->games;
		p->lcb   = 0;
		p->index = i;
		sort_n++;
		found_mate = true;
		PRT("%3d(%3d)%7s,%5d,%6.3f,bias=%.10f\n",i,sort_n,str_CSA_move(pc->move),pc->games,pc->value,pc->bias);
		break;
	}

	if ( !found_mate ) for (i=0;i<phg->child_num;i++) {
		CHILD *pc = &phg->child[i];

		float lcb = 0;
		if ( fLCB ) {	// Lower confidence bound of winrate.
			int visits = pc->games;
			lcb = LARGE_NEGATIVE_VALUE + visits;	// large negative value if not enough visits.
			if (visits >= 2) {
				float mean = pc->value;
//				if ( sideToMove == white ) mean = -mean;	// AZ(-1<x<1), LZ (0<x<1), mean = 1.0f - mean;
				float eval_variance = visits > 1 ? pc->squared_eval_diff / (visits - 1) : 1.0f;
				auto stddev = std::sqrt(eval_variance / visits);
				auto z = Utils::cached_t_quantile(visits - 1);
				lcb = mean - z * stddev;
			}
			if ( lcb > max_lcb ) {
				max_lcb = lcb;
				max_i = i;
			}
		} else {
			if ( pc->games > max_games ) {
				max_games = pc->games;
				max_i = i;
			}
		}
		if ( pc->games ) {
			if ( sort_n >= SORT_MAX ) DEBUG_PRT("");
			SORT_LCB *p = &sort_lcb[sort_n];
			p->move  = pc->move;
			p->games = pc->games;
			p->lcb   = lcb;
			p->index = i;
			sort_n++;
			float v = pc->value;
			PRT("%3d(%3d)%7s,%5d,%6.3f,bias=%.10f,V=%6.2f%%,LCB=%6.2f%%\n",i,sort_n,str_CSA_move(pc->move),pc->games,pc->value,pc->bias,100.0*(v+1.0)/2.0,100.0*(lcb+1.0)/2.0);
		}
	}
	if ( max_i >= 0 ) {
		CHILD *pc = &phg->child[max_i];
		best_move = pc->move;
		double v = 100.0 * (pc->value + 1.0) / 2.0;
		PRT("best:%s,%3d,%6.2f%%(%6.3f),bias=%6.3f\n",str_CSA_move(pc->move),pc->games,v,pc->value,pc->bias);
//		if ( v < 5 ) { PRT("resign threshold. 5%%\n"); best_move = 0; }
		char *pv_str = prt_pv_from_hash(ptree, ply, sideToMove, PV_CSA); PRT("%s\n",pv_str);
	}

	if ( fDiffRootVisit ) {
//		for (int i=0; i<phg->child_num; i++) {
//			sort_lcb[i].games -= keep_root_games[i];	// 1対1の対応でない
//			if ( sort_lcb[i].games < 0 ) DEBUG_PRT("");
//		}
	}
	int sum_games = 0;
	for (i=0; i<sort_n; i++) sum_games += sort_lcb[i].games;

	for (i=0; i<sort_n-1; i++) {
		int max_i = i;
		int max_g = sort_lcb[i].games;
		int j;
		for (j=i+1; j<sort_n; j++) {
			SORT_LCB *p = &sort_lcb[j];
			if ( p->games <= max_g ) continue;
			max_g = p->games;
			max_i = j;
		}
		if ( max_i == i ) continue;
		SORT_LCB dummy;
		dummy           = sort_lcb[    i];
		sort_lcb[    i] = sort_lcb[max_i];
		sort_lcb[max_i] = dummy;
	}
	if ( fLCB ) {	// 再度ソート。ただしgamesは変更しない。
		for (i=0; i<sort_n-1; i++) {
			int   max_i   = i;
			float max_lcb = sort_lcb[i].lcb;
			int j;
			for (j=i+1; j<sort_n; j++) {
				SORT_LCB *p = &sort_lcb[j];
				if ( p->lcb <= max_lcb ) continue;
				max_lcb = p->lcb;
				max_i   = j;
			}
			if ( max_i == i ) continue;
			SORT_LCB dummy;
			dummy           = sort_lcb[    i];
			sort_lcb[    i] = sort_lcb[max_i];
			sort_lcb[max_i] = dummy;
			int dum_g             = sort_lcb[    i].games;	// 再度swapでgamesはそのまま
			sort_lcb[    i].games = sort_lcb[max_i].games;
			sort_lcb[max_i].games = dum_g;
		}
	}

	buf_move_count[0] = 0;

	if ( fAutoResign ) {
		double v = 0;
		if ( max_i >= 0 ) {
			CHILD *pbest = &phg->child[max_i];
			v = (pbest->value + 1.0) / 2.0;	// -1 <= x <= +1  -->  0 <= x <= +1
		}
		sprintf(buf_move_count,"v=%.04f,%d",v,sum_games);
	} else {
		sprintf(buf_move_count,"%d",sum_games);
	}
	for (i=0;i<sort_n;i++) {
		SORT_LCB *p = &sort_lcb[i];
		char buf[7];
		csa2usi( ptree, str_CSA_move(p->move), buf );
		if ( 0 ) strcpy(buf,str_CSA_move(p->move));
//		PRT("%s,%d,",str_CSA_move(p->move),p->games);
		char str[TMP_BUF_LEN];
		sprintf(str,",%s,%d",buf,p->games);
		strcat(buf_move_count,str);
//		PRT("%s",str);
	}
//	PRT("\n");

	// 30手までは通常の温度で選び、31手以上はハンデのレートで弱くする。
	int is_opening_random = (ptree->nrep < nVisitCount || ptree->nrep < nVisitCountSafe);
	double select_rand_prob = dSelectRandom;
	double softmax_temp = cfg_random_temp;
	int rate = nHandicapRate[nHandicap];
	int is_weaken = 0;

	// 1400点差まではsoftmaxの温度で。1400+1157 = 2557差までは合法手ランダムで。
	if ( rate > 0 && sideToMove == black ) {	// 先手、もしくは下手のみを弱く
		double t = 0;
		double s = 0;
		if ( rate <= TEMP_RATE_MAX ) {
			t = get_sigmoid_temperature_from_rate(rate);
		} else {
			t = 6.068;
			s = get_sel_rand_prob_from_rate(rate - TEMP_RATE_MAX);
		}
		if ( is_opening_random == 0 || (is_opening_random && t > softmax_temp) ) {	// 31手以上、または30手以下で温度1を超えてたら適用
			is_weaken = 1;
			softmax_temp     = t;
			select_rand_prob = s;
		}
	}

	// selects moves proportionally to their visit count
	if ( (is_opening_random || is_weaken) && sum_games > 0 && sort_n > 0 ) {
//		static std::mt19937_64 mt64;
		static std::uniform_real_distribution<> dist(0, 1);
		double indicator = dist(get_mt_rand);

		double inv_temperature = 1.0 / softmax_temp;
		double wheel[MAX_LEGAL_MOVES];
		double w_sum = 0.0;
		for (int i = 0; i < sort_n; i++) {
			double d = static_cast<double>(sort_lcb[i].games);
			wheel[i] = pow(d, inv_temperature);
			w_sum += wheel[i];
		}
		double factor = 1.0 / w_sum;

		int select_index = -1;
		double sum = 0.0;
		for (i = 0; i < sort_n; i++) {
			sum += factor * wheel[i];
			if (sum <= indicator && i + 1 < sort_n) continue;	// 誤差が出た場合は最後の手を必ず選ぶ
			select_index = i;
			break;
		}
		int r = (int)(indicator * sum_games);

		if ( select_index < 0 || i==sort_n ) DEBUG_PRT("Err. nVisitCount not found.\n");

		int org_index = sort_lcb[select_index].index;	// LCB適用前の手
		CHILD *pc  = &phg->child[org_index];
		bool fSwap = true;
		if ( nVisitCountSafe && max_i >= 0 ) {	// 勝率がそれほど下がらず、ある程度の回数試した手だけを選ぶ
			CHILD *pbest = &phg->child[max_i];
			fSwap = false;
			if ( fabs(pbest->value - pc->value) < 0.04 && pc->games*5 > pbest->games ) fSwap = true;	// 0.04 で勝率2%。1%だとelmo相手に同棋譜が800局で15局、2%で4局。
		}
		if ( fSwap ) {
			best_move = sort_lcb[select_index].move;
			PRT("rand select:%s,%3d,%6.3f,bias=%6.3f,r=%d/%d,softmax_temp=%.3f(rate=%d),select_rand_prob=%.3f\n",str_CSA_move(pc->move),pc->games,pc->value,pc->bias,r,sum_games,softmax_temp,rate,select_rand_prob);
		}
		if ( fPolicyRealization && ptree->nrep < 30 ) {
			static int games = 0;
			static int prev_nrep = +999;
			static double realization_prob = 1;	// 単純に掛けると非常に小さい数になる。FLT_MIN = 1.175494e-38, DBL_MIN = 2.225074e-308
			static double realization_log  = 0;
			static double realization_log_sum = 0;
			if ( ptree->nrep < prev_nrep ) {
				games++;
				realization_log_sum += realization_log;
				realization_prob = 1;
				realization_log  = 0;
			}
			prev_nrep = ptree->nrep;

			double b = keep_root_policy[org_index];
			realization_prob *= b;
			realization_log  += log(b);	// logを取って足す
			FILE *fp = fopen("policy_dist.log","a");
			if ( fp ) {
				fprintf(fp,"%7d:%4d:%2d,%7s,b=%10.7f(%10.7f),prob=%12g,log=%12f(%12f)\n",getpid_YSS(),games,ptree->nrep,str_CSA_move(best_move),b,phg->child[org_index].bias, realization_prob, realization_log,(float)realization_log_sum/(games-1+(games==1)));
				fclose(fp);
			}
		}
	}
	if ( select_rand_prob > 0 && phg->child_num > 0 ) {
		double r = f_rnd();
		if ( r < select_rand_prob ) {
			int ri = rand_m521() % phg->child_num;
			best_move = phg->child[ri].move;
			PRT("ri=%d,r=%.3f,select_rand_prob=%.3f,best=%s\n",ri,r,select_rand_prob,str_CSA_move(best_move));
		}
	}

	if ( 0 && is_selfplay() && resign_winrate == 0 ) {
		int id = get_nnet_id();
		int pid = getpid_YSS();
		static int count, games;
		char str[TMP_BUF_LEN];
		if ( ptree->nrep==0 ) {
			if ( ++games > 15 ) { games = 1; count++; }
		}
		sprintf(str,"res%03d_%05d_%05d.csa",id,pid,count);
		FILE *fp = fopen(str,"a");
		if ( fp==NULL ) { PRT("fail res open.\n"); debug(); }

		float best_v = -1;
		if ( max_i >= 0 ) {
			CHILD *pbest = &phg->child[max_i];
			best_v = pbest->value;
		}
		if ( ptree->nrep==0 ) fprintf(fp,"/\nPI\n+\n");
		if ( best_move==0 ) {
			fprintf(fp,"%%TORYO,'%7.4f\n",best_v);
		} else {
			char sg[2] = { '+','-' };
			fprintf(fp,"%c%s,'%7.4f,%s\n",sg[ptree->nrep & 1],str_CSA_move(best_move),best_v,buf_move_count);
		}
		fclose(fp);
	}
	if ( 0 ) {
		static int search_sum = 0;
		static int playouts_sum = 0;
		const int M = 81;
		static int playouts_dist[M] = { 0 };
		static int games = 0;
		static int prev_nrep = +999;
		if ( ptree->nrep < prev_nrep ) games++;
		prev_nrep = ptree->nrep;
		search_sum++;
		playouts_sum += playouts;
		int m = playouts / 100;
		if ( m > M-1 ) m = M-1;
		playouts_dist[m]++;
		FILE *fp = fopen("playouts_dist.log","a");
		if ( fp ) {
			fprintf(fp,"%7d:%4d:search_sum=%5d,playouts_ave=%7.2f:",getpid_YSS(),games,search_sum, (float)playouts_sum/search_sum );
			for (i=0;i<M;i++) fprintf(fp,"%d,",playouts_dist[i]);
			fprintf(fp,"\n");
			fclose(fp);
		}
	}

	if ( fAutoResign == 0 && is_selfplay() && resign_winrate > 0 && max_i >= 0 ) {
		CHILD *pbest = &phg->child[max_i];
		double v = (pbest->value + 1.0) / 2.0;	// -1 <= x <= +1  -->  0 <= x <= +1
		if ( v < resign_winrate ) {
			best_move = 0;
			if ( 0 ) {
				FILE *fp = fopen("restmp.log","a");
				if ( fp ) {
					fprintf(fp,"%3d:%5d:%3d:v=%7.4f(%7.4f), resign_winrate=%7.4f\n",get_nnet_id(), getpid_YSS(), ptree->nrep, v, pbest->value, resign_winrate);
					fclose(fp);
				}
			}
		}
	}
	if ( fAutoResign && dAutoResignWinrate > 0 && max_i >= 0 ) {
		CHILD *pbest = &phg->child[max_i];
		double v = (pbest->value + 1.0) / 2.0;	// -1 <= x <= +1  -->  0 <= x <= +1
		if ( v < dAutoResignWinrate ) {
			best_move = 0;
		}
	}


	PRT("%.2f sec, c=%d,net_v=%.6f,h_use=%d,po=%d,%.0f/s,ave_ply=%.1f/%d (%d/%d),Noise=%d,g=%d,mt=%d,b=%d\n",
		ct,phg->child_num,phg->net_value,hash_shogi_use,playouts,(double)playouts/ct,ave_reached_ply,max_r_ply,ptree->nrep,nVisitCount,fAddNoise,default_gpus.size(),thread_max,cfg_batch_size );

	return best_move;
}

void create_node(tree_t * restrict ptree, int sideToMove, int ply, HASH_SHOGI *phg)
{
	if ( phg->deleted == 0 ) {
		PRT("already created? ply=%d,sideToMove=%d,games_sum=%d,child_num=%d\n",ply,sideToMove,phg->games_sum,phg->child_num); print_path();
		return;
	}
//PRT("create_node in..   ply=%d,sideToMove=%d,games_sum=%d,child_num=%d,slot=%d\n",ply,sideToMove,phg->games_sum,phg->child_num, ptree->tlp_slot);

if (0) {
	print_board(ptree);
	int i;
	for (i=0;i<81;i++) {
		int k = BOARD[i];
		PRT("%3d",k);
		if ( (i+1)%9==0 ) PRT("\n");
	}
	PRT("===kiki_count===\n");
	int kiki_count[2][81] = { 0 };
	kiki_count_indirect(ptree, kiki_count, NULL, false);
	for (int c=0;c<2;c++) for (i=0;i<81;i++) {
		PRT("%2d",kiki_count[c][i]);
		if ( (i+1)%9==0 ) PRT("\n");
		if ( i==80 ) PRT("\n");
	}

	int kiki_bit[14][2][81] = { 0 };
	kiki_count_indirect(ptree, NULL, kiki_bit, true);
	for (int c=0;c<2;c++) for (int k=0;k<14;k++) for (i=0;i<81;i++) {
		if ( i==0 ) PRT("k=%d,c=%d\n",k,c);
		PRT("%2d",kiki_bit[k][c][i]);
		if ( (i+1)%9==0 ) PRT("\n");
	}


	PRT("===\n");
	for (i=0;i<81;i++) {
		int k = count_square_attack(ptree, black, i);
		PRT("%2d",k);
		if ( (i+1)%9==0 ) PRT("\n");
	}
	PRT("---\n");
	for (i=0;i<81;i++) {
		int k = count_square_attack(ptree, white, i);
		PRT("%2d",k);
		if ( (i+1)%9==0 ) PRT("\n");
	}
	PRT("===\n");
}

	int move_num = generate_all_move( ptree, sideToMove, ply );

#ifdef CHILD_VEC
	phg->child.reserve(move_num);
#endif

	unsigned int * restrict pmove = ptree->move_last[0];
	int i;
	for ( i = 0; i < move_num; i++ ) {
		int move = pmove[i];
		CHILD *pc = &phg->child[i];
		pc->move = move;
		pc->bias = 0;
		if ( NOT_USE_NN ) pc->bias = f_rnd()*2.0f - 1.0f;	// -1 <= x <= +1
		pc->games = 0;
		pc->value = 0;
		pc->exact_value = EX_NONE;
		pc->squared_eval_diff = 1e-4f;	// Initialized to small non-zero value to avoid accidental zero variances at low visits.
		pc->acc_virtual_loss = 0;
	}
	phg->child_num      = move_num;

	if ( NOT_USE_NN ) {
		// softmax
		const float temperature = 1.0f;
		float max = -10000000.0f;
		for (i=0; i<move_num; i++) {
			CHILD *pc = &phg->child[i];
			if ( max < pc->bias ) max = pc->bias;
		}
		float sum = 0;
		for (i=0; i<move_num; i++) {
			CHILD *pc = &phg->child[i];
			pc->bias = (float)exp((pc->bias - max)/temperature);
			sum += pc->bias;
		}
		for(i=0; i<move_num; i++){
			CHILD *pc = &phg->child[i];
			pc->bias /= sum;
//			PRT("%2d:bias=%10f, sum=%f,ply=%d\n",i,pc->bias,sum,ply);
		}
	}

//PRT("create_node net.   ply=%d,sideToMove=%d,games_sum=%d,child_num=%d,slot=%d\n",ply,sideToMove,phg->games_sum,phg->child_num, ptree->tlp_slot);
	float v = 0;
	if ( NOT_USE_NN ) {
		float f = f_rnd()*2.0f - 1.0f;
		v = std::tanh(f);		// -1 <= x <= +1   -->  -0.76 <= x <= +0.76
//		{ static double va[2]; static int count[2]; va[sideToMove] += v; count[sideToMove]++; PRT("va[]=%10f,%10f\n",va[0]/(count[0]+1),va[1]/(count[1]+1)); }
//		PRT("f=%10f,tanh()=%10f\n",f,v);
	} else {
		if ( move_num == 0 ) {
			// get_network_policy_value() は常に先手が勝で(+1)、先手が負けで(-1)を返す。sideToMove は無関係
			v = -1;
			if ( sideToMove==white ) v = +1;	// 後手番で可能手がないなら先手の勝
		} else {
			v = get_network_policy_value(ptree, sideToMove, ply, phg);
		}
//		{ PRT("ply=%2d,sideToMove=%d(white=%d),move_num=%3d,v=%.5f\n",ply,sideToMove,white,move_num,v); print_board(ptree); }
	}
	if ( sideToMove==white ) v = -v;
