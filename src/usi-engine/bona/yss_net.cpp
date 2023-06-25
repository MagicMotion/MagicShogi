
﻿// 2019 Team AobaZero
// This source code is in the public domain.
#include "../config.h"

#include <numeric>
#include <random>

#include <cstdint>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <chrono>
#include <exception>


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#if !defined(_MSC_VER)
#include <sys/time.h>
#include <unistd.h>
#endif

#include "../Network.h"
#include "../GTP.h"
#include "../Random.h"


#include "shogi.h"

#include "lock.h"
#include "yss_var.h"
#include "yss_dcnn.h"
#include "process_batch.h"

//using namespace Utils;
std::string default_weights;
std::vector<int> default_gpus;
void init_global_objects();	// Leela.cpp
void initialize_network();

bool fUseLeelaZeroOpenCL = false;	// only for test. disable USE_OPENCL_SELFCHECK

#ifdef USE_OPENCL
#define NN_PARALLEL
#endif

#ifdef NN_PARALLEL
#include "nnet-srv.hpp"
#include "nnet-ipc.hpp"

using std::copy_n;

std::vector<NNetIPC *> p_nnet_v;
std::vector<int> nNNetID_v;
int nNNetServiceNumber = -1;
SeqPRN *p_seq_prn;	// プロセスが呼ばれる時点で SeqPRNServiceで確保されてるはず
int nUseHalf = 0;
int nUseWmma = 0;
std::string sDirTune = "";
#endif
