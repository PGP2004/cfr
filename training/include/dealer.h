#include "indexer.h"      
#include "evaluator.h"  
#include "action_tree.h"
#include <array>
#include <cstdint>
#include <random>

class Dealer{

private:
    std::array<std::array<uint8_t, 7>, 2> hands;
    std::array<std::array<int, 4>, 2> hand_ids;
    std::array<double, 2> equities;

public:

    int get_hand_id(int player, size_t street) const {
        return hand_ids[player][street];
    }

    void deal(std::mt19937& rng);
    double get_reward(int player, ActionTree& at);
};
