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

    public:
        CardBuckets card_buckets;
        ActionTree action_tree;
        Dealer dealer;

        std::mt19937 rng;
        InfoSets infosets;

        std::vector<std::vector<double>> probs_scratch;
        std::vector<std::vector<double>> deltas_scratch;

        InfoKey get_InfoKey(const ActionTree& at, const Dealer& d);
        double traverse_helper(int player, int depth);
        void traverse(int player);

        void write_check_point(ISetsCkpt ck_pt);

        CFR(CardBuckets buckets, ActionTree at);
        void write_isets_check_point(const ISetsCkpt& ck_pt){infosets.write_check_point(ck_pt);}
        void train(int num_iters, int iters_per_discount);

    };