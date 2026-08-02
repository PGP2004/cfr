#pragma once
#include "game_state.h"
#include <random>
#include <vector>
#include <cstddef>
#include <array>

//TODO: Add something that 

struct PublicState{
    int street_idx;
    int active_player;
    std::array<double,2> payoffs;
    bool folded;
    std::vector<Action> edge_labels;
};

struct TreeNode{
    size_t node_idx;
    size_t parent_idx;
    std::vector<size_t> child_idxs;
};

class ActionTree{

private:
    std::vector<Action> discretize_actions(const GameState& state);
    std::vector<Action> get_legal_actions(const GameState& state);
    PublicState get_public_state(const GameState& state);
    
public:
    std::vector<TreeNode> nodes;
    std::vector<PublicState> pub_states;

    size_t root_idx;
    size_t cur_idx;

    ActionTree(const GameState& root_state);
    void undo_action();
    void apply_action(size_t action_idx);

    int active_player() const{
        return pub_states[cur_idx].active_player;
    };

    int street() const {
        return pub_states[cur_idx].street_idx;
    };

    bool is_terminal() 
        const {return nodes[cur_idx].child_idxs.size() == 0;
    }

    bool folded() const{
        return pub_states[cur_idx].folded;
    }

    bool is_root_node() const{
        return root_idx == cur_idx;
    }

    double get_payoff(int player) const{
        return pub_states[cur_idx].payoffs[player];
    }

    void restart() {
        cur_idx = root_idx;
    }

};
