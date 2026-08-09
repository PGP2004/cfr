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
        int iters_per_discount;

        InfoSets infosets;
        InfoKey get_InfoKey(const ActionTree& at, const Dealer& d);
        double traverse_helper(int player);
        void traverse(int player);

        void write_check_point(CheckPoint ck_pt);

    public:

        CFR(CardBuckets& card_buckets, ActionTree& action_tree);
        CFR(const CheckPoint& ck_pt, CardBuckets& card_buckets, ActionTree& action_tree);
        void write_isets_check_point(const CheckPoint& ck_pt){infosets.write_check_point(ck_pt);}
        void train(int num_iterations, int starting_iter);

        double get_prob(InfoKey ikey, Action a);
        std::unordered_map<std::string, double> preflop_probs(Action action);
    };