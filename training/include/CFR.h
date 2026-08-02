
#pragma once

#include <random>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>

#include "game_state.h"
#include "info_sets.h"

class CFR {

    private:
        GameState state;
        Abstraction abs;
        ActionTree action_tree;
        std::mt19937 rng;
        int iters_per_discount;

        InfoSets infosets;
        InfoKey get_InfoKey(const GameState& state, const ActionTree& at);
        double traverse(int player, GameState& state, ActionTree& at);

    public:

        CFR(GameState init_game_state, Abstraction& abstraction, ActionTree& action_tree);
        void train(int num_iterations, int starting_iter);
        double get_action_prob(int player, InfoKey info_key, Action action);

    };