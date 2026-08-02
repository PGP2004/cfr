#include <cstdint>
#include <vector>
#include "action_tree.h"
#include "game_state.h"

std::vector<Action> ActionTree::get_legal_actions(const GameState& state) {
    int pot = state.get_pot();
    int my_pip = state.get_pip(state.get_active_player());

    std::vector<std::pair<std::string, Action>> candidates = {
        {"fold", {0, 0}},
        {"check", {1, 0}},
        {"call", {2, 0}},
        {"pot", {3, my_pip + pot}},
    };

    std::vector<Action> action_vec;
    for (const auto& cand : candidates) {
        if (state.is_legal_action(cand.second)) {
            action_vec.push_back(cand.second);
        }
    }
    return action_vec;
}

void ActionTree::undo_action(){
    cur_idx = nodes[cur_idx].parent_idx;
}

void ActionTree::apply_action(size_t action_idx){
    if (action_idx >= nodes[cur_idx].child_idxs.size()){
        throw std::out_of_range("the idx is out of range");
    }

    cur_idx = nodes[cur_idx].child_idxs[action_idx];
    return;
}

ActionTree::ActionTree(const GameState& root_state) {

    std::mt19937 rng(0);

    nodes.push_back(ActionNode{0, 0, root_state.get_street(), {}, {},
                               root_state.get_active_player()});
    root_idx = 0;

    std::vector<std::pair<GameState, size_t>> stack;  // (state, tree node idx)
    stack.push_back({root_state, root_idx});

    while (!stack.empty()) {
        auto [state, node_idx] = std::move(stack.back());
        stack.pop_back();

        if (state.is_terminal_node()) continue;

        if (state.is_chance_node()) {
            stack.push_back({state.apply_chance(rng), node_idx});
            continue;
        }

        size_t street_idx = state.get_street();

        for (const Action& action : get_legal_actions(state)) {
            GameState child = state.apply_action(action);

            size_t child_idx = nodes.size();
            nodes[node_idx].child_idxs.push_back(child_idx);
            nodes[node_idx].edges.push_back(action);
            nodes.push_back(ActionNode{child_idx, node_idx, street_idx, {}, {},
                                       child.get_active_player()});

            stack.push_back({std::move(child), child_idx});
        }
    }
}