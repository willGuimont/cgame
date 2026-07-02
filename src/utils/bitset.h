#pragma once

#include <assert.h>

#include "common.h"

typedef struct {
    u64 bits;
} Bitset;

static void Bitset_Init(Bitset *bitset) { bitset->bits = 0; }

static void Bitset_InitBits(Bitset *bitset, const u64 bits) { bitset->bits = bits; }

static void Bitset_Clear(Bitset *bitset) { bitset->bits = 0; }

static bool Bitset_BitInBounds(const u64 bit) { return bit < 64; }

static void Bitset_SetBit(Bitset *bitset, const u64 bit) {
    assert(Bitset_BitInBounds(bit));
    if (!Bitset_BitInBounds(bit))
        return;
    bitset->bits |= 1ULL << bit;
}

static void Bitset_ClearBit(Bitset *bitset, const u64 bit) {
    assert(Bitset_BitInBounds(bit));
    if (!Bitset_BitInBounds(bit))
        return;
    bitset->bits &= ~(1ULL << bit);
}

static void Bitset_Toggle(Bitset *bitset, const u64 bit) {
    assert(Bitset_BitInBounds(bit));
    if (!Bitset_BitInBounds(bit))
        return;
    bitset->bits ^= 1ULL << bit;
}

static bool Bitset_IsSet(const Bitset *bitset, const u64 bit) {
    assert(Bitset_BitInBounds(bit));
    if (!Bitset_BitInBounds(bit))
        return false;
    return (bitset->bits & 1ULL << bit) != 0;
}
