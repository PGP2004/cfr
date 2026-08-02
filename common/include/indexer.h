#pragma once
#include <cstdint>

extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}

struct Indexer {
    hand_indexer_t h;
    Indexer(uint32_t rounds, const uint8_t* cpr) { hand_indexer_init(rounds, cpr, &h); }
    ~Indexer() { hand_indexer_free(&h); }
    Indexer(const Indexer&) = delete; 
    Indexer& operator=(const Indexer&) = delete;
};
