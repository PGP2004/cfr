#include "dealer.h"
#include "indexer.h"      
#include "evaluator.h"  

#include <utility>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <random>
#include "action_tree.h"

using cards_t = std::array<std::array<uint8_t, 7>, 2>;
using hand_ids_t = std::array<std::array<int, 4>, 2>;

static void write_cards(std::mt19937& rng, cards_t& cards, std::array<uint8_t,52>& deck) {
    //fisher yates for the first 9 cards

    for (int i = 0; i < 9; ++i) {
        std::uniform_int_distribution<int> dist(i, 51);
        int j = dist(rng);
        std::swap(deck[i], deck[j]);
    }

    cards[0][0] = deck[0]; cards[0][1] = deck[1];
    cards[1][0] = deck[2]; cards[1][1] = deck[3];

    size_t count = 2;
    for (int i = 4; i < 9; ++i) {
        cards[0][count] = deck[i];
        cards[1][count] = deck[i];
        count += 1;
    }
}

static void write_card_ids(cards_t& cards, hand_ids_t& hand_ids){

    static const std::array<uint8_t, 1> preflop_counts = {2};
    static const std::array<uint8_t, 2> flop_counts = {2, 3};
    static const std::array<uint8_t, 2> turn_counts = {2, 4};
    static const std::array<uint8_t, 2> river_counts = {2, 5};

    static Indexer preflop_indexer{preflop_counts.size(), preflop_counts.data()};
    static Indexer flop_indexer{flop_counts.size(),flop_counts.data()};
    static Indexer turn_indexer{turn_counts.size(),turn_counts.data()};
    static Indexer river_indexer{river_counts.size(), river_counts.data()};

    for (size_t p = 0; p < 2; ++p) {
        hand_ids[p][0] = static_cast<int>(hand_index_last(&preflop_indexer.h, cards[p].data()));
        hand_ids[p][1] = static_cast<int>(hand_index_last(&flop_indexer.h,cards[p].data()));
        hand_ids[p][2] = static_cast<int>(hand_index_last(&turn_indexer.h,cards[p].data()));
        hand_ids[p][3] = static_cast<int>(hand_index_last(&river_indexer.h,cards[p].data()));
    }

}

static int get_winner(cards_t& cards) {
    uint8_t r0[7], s0[7];
    uint8_t r1[7], s1[7];

    for (int i = 0; i < 7; ++i) {
        uint8_t c0 = cards[0][i];
        uint8_t c1 = cards[1][i];
        r0[i] = card_rank(c0);  s0[i] = card_suit(c0);
        r1[i] = card_rank(c1);  s1[i] = card_suit(c1);
    }

    uint32_t score0 = evaluate_raw(r0, s0, 7);
    uint32_t score1 = evaluate_raw(r1, s1, 7);

    if (score0 > score1) return 0;
    else if (score1 > score0) return 1;
    return -1;
}

void Dealer::deal(std::mt19937& rng){
    write_cards(rng, cards, deck);
    write_card_ids(cards, card_ids);
    winner = get_winner(cards);
}


Dealer::Dealer(){
    for (int i = 0; i < 52; ++i) deck[i] = i; 
}
