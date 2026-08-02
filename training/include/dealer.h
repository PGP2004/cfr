#include "indexer.h"      
#include "evaluator.h"  

#include <array>
#include <cstdint>
#include <random>

extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}

class Dealer {
private:
   
    static inline const std::array<uint8_t, 1> preflop_counts = {2};
    static inline const std::array<uint8_t, 2> flop_counts = {2, 3};
    static inline const std::array<uint8_t, 2> turn_counts = {2, 4};
    static inline const std::array<uint8_t, 2> river_counts = {2, 5};

    static inline Indexer preflop_indexer{preflop_counts.size(), preflop_counts.data()};
    static inline Indexer flop_indexer{flop_counts.size(),flop_counts.data()};
    static inline Indexer turn_indexer{turn_counts.size(),turn_counts.data()};
    static inline Indexer river_indexer{river_counts.size(), river_counts.data()};

public:
    std::array<std::array<int, 4>, 2> hand_ids;
    std::array<std::array<uint8_t, 7>, 2> hands;
    std::array<double, 2> equities;
    std::array<uint8_t, 52> deck;

    Dealer() {
        for (auto& h : hand_ids) h.fill(1);
        equities.fill(-1.0);
        for (int i = 0; i < 52; ++i)
            deck[i] = static_cast<uint8_t>(i);
    }

    void update_equities() {
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

        if (score0 > score1) equities = {1.0, 0.0};
        else if (score1 > score0) equities = {0.0, 1.0};
        else equities = {0.5, 0.5};
        return;
    }

    void deal(std::mt19937& rng) {
        //fisher yates for the first 9 cards
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

        for (size_t p = 0; p < 2; ++p) {
            hand_ids[p][0] = static_cast<int>(hand_index_last(&preflop_indexer.h, hands[p].data()));
            hand_ids[p][1] = static_cast<int>(hand_index_last(&flop_indexer.h,hands[p].data()));
            hand_ids[p][2] = static_cast<int>(hand_index_last(&turn_indexer.h,hands[p].data()));
            hand_ids[p][3] = static_cast<int>(hand_index_last(&river_indexer.h,hands[p].data()));
        }
    }

    void deal_and_update_equities(std::mt19937& rng){
        deal(rng);
        update_equities();
    }

    int get_hand_id(int player, size_t street) const {
        return hand_ids[player][street];
    }
};