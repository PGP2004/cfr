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

struct PreflopStrategy{
    std::vector<Action> actions;
    std::unordered_map<std::string, std::vector<double>> probs;
};

class CFR {

    private:
        CardBuckets card_buckets;
        ActionTree action_tree;
        Dealer dealer;

        std::mt19937 rng;
        InfoSets infosets;
        int iters_per_discount;

        std::vector<std::vector<double>> probs_scratch;
        std::vector<std::vector<double>> deltas_scratch;

        InfoKey get_InfoKey(const ActionTree& at, const Dealer& d);
        double traverse_helper(int player, int depth);
        void traverse(int player);

        void write_check_point(ISetsCkpt ck_pt);

    public:

        CFR(CardBuckets buckets, ActionTree at, int iters_per_discount);
        void write_isets_check_point(const ISetsCkpt& ck_pt){infosets.write_check_point(ck_pt);}
        void train(int num_iters);
        PreflopStrategy get_preflop_strategy();
    };