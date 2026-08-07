#include "dealer.h"
#include "indexer.h"      
#include "evaluator.h"  
#include <array>
#include <cstdint>
#include <stdexcept>
#include <random>
#include "action_tree.h"

using hands_t = std::array<std::array<uint8_t, 7>, 2>;
using hand_ids_t = std::array<std::array<int, 4>, 2>;

static void write_hands(std::mt19937& rng, hands_t& hands) {
    //fisher yates for the first 9 cards

    static std::array<uint8_t, 52> deck = [] {
        std::array<uint8_t, 52> d; 
        for (int i = 0; i < 52; ++i) d[i] = i;
        return d;
    }();

    static std::array<uint8_t,52> deck;
    for (int i = 0; i < 9; ++i) {
        std::uniform_int_distribution<int> dist(i, 51);
        int j = dist(rng);
        std::swap(deck[i], deck[j]);
    }

    hands[0][0] = deck[0]; hands[0][1] = deck[1];
    hands[1][0] = deck[2]; hands[1][1] = deck[3];

    size_t count = 2;
    for (int i = 4; i < 9; ++i) {
    hands[0][count] = deck[i];
    hands[1][count] = deck[i];
        count += 1;
    }
}

static void write_hand_ids(hands_t& hands, hand_ids_t hand_ids){

    static inline const std::array<uint8_t, 1> preflop_counts = {2};
    static inline const std::array<uint8_t, 2> flop_counts = {2, 3};
    static inline const std::array<uint8_t, 2> turn_counts = {2, 4};
    static inline const std::array<uint8_t, 2> river_counts = {2, 5};

    static inline Indexer preflop_indexer{preflop_counts.size(), preflop_counts.data()};
    static inline Indexer flop_indexer{flop_counts.size(),flop_counts.data()};
    static inline Indexer turn_indexer{turn_counts.size(),turn_counts.data()};
    static inline Indexer river_indexer{river_counts.size(), river_counts.data()};

    static std::array<uint8_t, 52> deck;

    for (size_t p = 0; p < 2; ++p) {
        hand_ids[p][0] = static_cast<int>(hand_index_last(&preflop_indexer.h, hands[p].data()));
        hand_ids[p][1] = static_cast<int>(hand_index_last(&flop_indexer.h,hands[p].data()));
        hand_ids[p][2] = static_cast<int>(hand_index_last(&turn_indexer.h,hands[p].data()));
        hand_ids[p][3] = static_cast<int>(hand_index_last(&river_indexer.h,hands[p].data()));
    }

}

void write_equities(hands_t& hands, std::array<double,2>& equities) {
    uint8_t r0[7], s0[7];
    uint8_t r1[7], s1[7];

    for (int i = 0; i < 7; ++i) {
        uint8_t c0 = hands[0][i];
        uint8_t c1 = hands[1][i];
        r0[i] = card_rank(c0);  s0[i] = card_suit(c0);
        r1[i] = card_rank(c1);  s1[i] = card_suit(c1);
    }

    uint32_t score0 = evaluate_raw(r0, s0, 7);
    uint32_t score1 = evaluate_raw(r1, s1, 7);

    if (score0 > score1){
        equities[0] = 1.0; equities[1] = 0.0;
    } else if (score1 > score0){
        equities[0] = 0.0; equities[1] = 1.0;
    } else{
        equities[0] = 0.5; equities[1] = 0.5;
    }
};

void Dealer::deal(std::mt19937& rng){
    write_hands(rng, hands);
    write_hand_ids(hands, hand_ids);
    write_equities(hands, equities);
}

double Dealer::get_reward(int player, ActionTree& at){

    if (!at.is_terminal()){
        throw std::runtime_error("cannot get reward for non-terminal node");
    }

    int opp = 1 - player;

    //if someone folded in the game
    if (at.folded()){
        bool won = (player == at.active_player());
        if (won) return at.get_payoff(player);
        return -at.get_payoff(opp);
    }

    // if no one folded in the game. Look at the equities
    if (equities[player] == 0.5) return 0.0;
    else if (equities[player] == 1.0) return at.get_payoff(player);
    else if (equities[player] == 0.0) return - at.get_payoff(opp);

    throw std::runtime_error("Should not be able to get here");
    return 0.0;
}

