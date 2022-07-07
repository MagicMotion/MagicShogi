// 2019 Team AobaZero
// This source code is in the public domain.
#ifndef BITOP_H
#define BITOP_H

#define BBToU(b)            ( (b).p[0] | (b).p[1] | (b).p[2] )
#define BBToUShift(b)       ( (b).p[0]<<2 | (b).p[1]<<1 | (b).p[2])
#define PopuCount(bb)       popu_count012( bb.p[0], bb.p[1], bb.p[2] )
#define FirstOne(bb)        first_one012( bb.p[0], bb.p[1], bb.p[2] )
#define LastOne(bb)         last_one210( bb.p[2], bb.p[1], bb.p[0] )
#define BBCmp(b1,b2)        ( (b1).p[0] != (b2).p[0]                    \
				|| (b1).p[1] != (b2).p[1]               \
				|| (b1).p[2] != (b2).p[2] )
#define BBContractShift(b1,b2) ( ( (b1).p[0] & (b2).p[0] ) << 2         \
                               | ( (b1).p[1] & (b2).p[1] ) << 1         \
                               | ( (b1).p[2] & (b2).p[2] ) )

#if defined(HAVE_SSE2) || defined(HAVE_SSE4)

#if defined(HAVE_SS