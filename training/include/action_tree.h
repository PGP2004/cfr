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
    std::vector<Action> get_legal_actions(const GameState& state);
    
public:
    std::vector<ActionNode> nodes;
    size_t root_idx;
    size_t cur_idx;

    ActionTree(const GameState& root_state);
    void undo_action();
    void apply_action(size_t action_idx);

    int get_player() const{
        return nodes[cur_idx].active_player;
    };

    int get_street() const {
        return nodes[cur_idx].street_idx;
    };

    bool is_terminal() const {
        return nodes[cur_idx].street_idx == 8;
    }

    bool is_root_node() const{
        return root_idx == cur_idx;
    }
};
