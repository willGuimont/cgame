#ifndef CGAME_BITSET_H
#define CGAME_BITSET_H

#include "../common.h"

typedef struct {
    u64 bits;
} Bitset;

static void BitSet_Init(Bitset *bitset) { bitset->bits = 0; }

static void BitSet_InitBits(Bitset *bitset, u64 bits) { bitset->bits = bits; }

static void BitSet_Clear(Bitset *bitset) { bitset->bits = 0; }

static void BitSet_SetBit(Bitset *bitset, u64 bit) { bitset->bits |= (1ULL << bit); }

static void BitSet_ClearBit(Bitset *bitset, u64 bit) { bitset->bits &= ~(1ULL << bit); }

static void BitSet_Toggle(Bitset *bitset, u64 bit) { bitset->bits ^= (1ULL << bit); }

static bool BitSet_IsSet(Bitset *bitset, u64 bit) { return (bitset->bits & (1ULL << bit)) != 0; }

#endif // CGAME_BITSET_H
