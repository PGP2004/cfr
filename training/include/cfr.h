#pragma once
#include <random>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>

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
        InfoSets infosets;

        std::vector<std::vector<double>> probs_scratch;
        std::vector<std::vector<double>> deltas_scratch;

        double traverse_helper(int player, int depth);

    public:
        CFR(CardBuckets buckets, ActionTree at);
        CFR(InfoSets isets, CardBuckets buckets, ActionTree at);

        InfoKey get_InfoKey(const ActionTree& at, const Dealer& d);
 
        void traverse(int player);
        void train(int num_iters, int iters_per_discount);


        const ActionTree& get_action_tree()const {return action_tree;}
        const InfoSets& get_infosets()const {return infosets;}

    };