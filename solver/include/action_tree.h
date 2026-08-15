#pragma once
#include "game_state.h"
#include <random>
#include <vector>
#include <cstddef>
#include <array>

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

    int raise_to_x_pot(double x, int player, int pot,
        std::array<int,2> pips, std::array<int,2> stacks) const;

    std::vector<Action> get_actions(const GameState& state);
    std::vector<Action> get_legal_actions(const GameState& state);

    PublicState get_public_state(const GameState& state);
    
public:

    size_t root_idx;
    std::vector<TreeNode> nodes;
    std::vector<PublicState> pub_states;

    //array of bet sizes per street with bet sizes encoded as floats where (0.333 = 1/3 pot bet)
    std::vector<std::vector<float>> bet_sizes; 

    ActionTree(const GameState& root_state, const std::vector<std::vector<float>>& bet_szs);

    size_t apply_action(size_t node_idx, size_t action_idx){
        if (action_idx >= nodes[node_idx].child_idxs.size()){
            throw std::out_of_range("the idx is out of range");
        }
        return nodes[node_idx].child_idxs[action_idx];
    }

    int active_player(size_t node_idx) const {return pub_states[node_idx].active_player;};
    int num_children(size_t node_idx) const {return nodes[node_idx].child_idxs.size();};

    int street(size_t node_idx) const { return pub_states[node_idx].street_idx;};

    bool is_terminal(size_t node_idx) const {return nodes[node_idx].child_idxs.size() == 0;}

    bool folded(size_t node_idx) const{return pub_states[node_idx].folded;}

    bool is_root_node(size_t node_idx) const{return root_idx == node_idx;}

    double get_payoff(size_t node_idx, int player) const{ return pub_states[node_idx].payoffs[player];}

    size_t depth() const;

    size_t max_branching() const;

};
