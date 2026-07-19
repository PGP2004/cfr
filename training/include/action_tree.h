#pragma once
#include "game_state.h"
#include <random>
#include <vector>
#include <cstddef>

struct ActionNode{
    size_t node_idx;
    size_t parent_idx;
    size_t street_idx;

    std::vector<int> child_idxs;
    std::vector<Action> edges;

    int active_player;
};

class ActionTree{

private: 
    void dfs_tree(GameState& state, std::mt19937& rng);

public:

    std::vector<ActionNode> nodes;
    size_t root_idx;
    size_t cur_idx;

    ActionTree(GameState& root_state);

    std::vector<Action> get_legal_actions(GameState& state);

    void undo_action(){
        cur_idx = nodes[cur_idx].parent_idx;
    }

    void apply_action(size_t action_idx){
        if (action_idx >= nodes[cur_idx].child_idxs.size()){
             throw std::out_of_range("the idx is out of range");
        }

        cur_idx = nodes[cur_idx].child_idxs[action_idx];
        return;
    }
};