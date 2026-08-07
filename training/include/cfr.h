#pragma once
#include <random>
#include <vector>
#include <array>

#include "action.h"
#include "game_state.h"
#include "card_buckets.h"

#include "info_sets.h"
#include "action_tree.h"
#include "dealer.h"

class CFR {

    private:
        CardBuckets card_buckets;
        ActionTree action_tree;
        Dealer dealer;

        std::mt19937 rng;
        int iters_per_discount;

        InfoSets infosets;
        InfoKey get_InfoKey(const ActionTree& at, const Dealer& cur_dealer);
        double traverse_helper(int player);
        void traverse(int player);

    public:

        CFR(CardBuckets& card_buckets, ActionTree& action_tree);
        void train(int num_iterations, int starting_iter);
    };