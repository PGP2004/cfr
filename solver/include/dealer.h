#pragma once
#include "indexer.h"      
#include "evaluator.h"  
#include <array>
#include <cstdint>
#include <random>

class Dealer{
public:
    std::array<std::array<uint8_t, 7>, 2> cards;
    std::array<std::array<int, 4>, 2> card_ids;
    std::array<uint8_t,52> deck;
    int winner; // 0 or 1 means player 0/1 is the winner. -1 is tie

    int get_card_id(int player, size_t street) const {
        return card_ids[player][street];
    }

    void deal(std::mt19937& rng);
    
    Dealer();

};

