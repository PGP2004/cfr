#include <cstdint>
#include <vector>
#include "action_tree.h"
#include "game_state.h"

std::vector<Action> ActionTree::discretize_actions(const GameState& state){
    int pot = state.get_pot();
    int my_pip = state.get_pip(state.get_active_player());

    std::vector<std::pair<std::string, Action>> candidates = {
        {"fold", {0, 0}}, {"check", {1, 0}},
        {"call", {2, 0}}, {"pot", {3, my_pip + pot}}
    };

    std::vector<Action> output;
    for (const auto& cand : candidates) output.push_back(cand.second);
    return output;
}

std::vector<Action> ActionTree::get_legal_actions(const GameState& state){

    std::vector<Action> candidates = discretize_actions(state);
    std::vector<Action> legal_actions;

    for (const Action& cand : candidates) {
        if (state.is_legal_action(cand)) {
            legal_actions.push_back(cand);
        }
    }

    return legal_actions;
}

PublicState ActionTree::get_public_state(const GameState& state){
    PublicState pub_state{
        .street_idx = state.get_street(),
        .active_player = state.get_active_player(),
        .payoffs = {state.get_win_payoff(0),state.get_win_payoff(1)},
        .folded = state.player_folded()
    };

    //TODO: I need to find a way to get rewards here!
    //this function is wrong as is!

    return pub_state;
}

void ActionTree::undo_action(){
    cur_idx = nodes[cur_idx].parent_idx;
}

void ActionTree::apply_action(size_t action_idx){
    if (action_idx >= nodes[cur_idx].child_idxs.size()){
        throw std::out_of_range("the idx is out of range");
    }

    cur_idx = nodes[cur_idx].child_idxs[action_idx];
}

ActionTree::ActionTree(const GameState& root_state) {
    std::mt19937 rng(0);

    root_idx = 0;
    cur_idx  = 0;

    nodes.push_back(TreeNode{0, 0, {}});
    pub_states.push_back(get_public_state(root_state));

    std::vector<std::pair<GameState, size_t>> stack;  // (state, node idx)
    stack.push_back({root_state, root_idx});

    while (!stack.empty()) {
        auto [state, node_idx] = std::move(stack.back());
        stack.pop_back();

        if (state.is_terminal_node()) continue;

        if (state.is_chance_node()) {
            stack.push_back({state.apply_chance(rng), node_idx});
            continue;
        }

        for (const Action& action : get_legal_actions(state)) {
            GameState child = state.apply_action(action);

            size_t child_idx = nodes.size();
            nodes[node_idx].child_idxs.push_back(child_idx);
            pub_states[node_idx].edge_labels.push_back(action);

            nodes.push_back(TreeNode{child_idx, node_idx, {}});
            pub_states.push_back(get_public_state(child));

            stack.push_back({std::move(child), child_idx});
        }
    }
}