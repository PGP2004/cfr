#include <cstdint>
#include <vector>
#include <iostream>
#include <unordered_set>
#include "action_tree.h"
#include "poker_state.h"

int ActionTree::raise_to_x_pot(double x, const PokerState& state) const {

    int me = state.active_player;
    int opp = 1 - me;
    int my_pip = state.pips[me]; 
    int opp_pip = state.pips[opp];

    int to_call = opp_pip - my_pip;
    int pot_after = state.pot + to_call;

    int cur_bet = std::max(my_pip, opp_pip);
    int raise_to  = cur_bet + static_cast<int>(std::llround(x * pot_after));

    int min_raise_to = cur_bet + std::max(state.big_blind, to_call);
    int max_raise_to = std::min(state.pips[0] + state.stacks[0], state.pips[1] + state.stacks[1]);

    if (raise_to < min_raise_to) raise_to = min_raise_to;
    if (raise_to >= max_raise_to) raise_to = max_raise_to;
    return raise_to;
}

std::vector<Action> ActionTree::get_actions(const PokerState& state){

    std::vector<Action> output = {
        {0, 0}, //fold
        {1, 0}, //check
        {2, 0}, //call
    };

    std::unordered_set<int> seen;
    int street = state.get_street();

    for (float x : bet_sizes[street]){

        int chip_amt = raise_to_x_pot(x, state);

        if (seen.insert(chip_amt).second) {
            //prevent multiple min/max raises getting passed along
            output.push_back({3, chip_amt});
        }
    }

    return output;
}

std::vector<Action> ActionTree::get_legal_actions(const PokerState& state){
    std::vector<Action> legal_actions;

    for (const Action& cand : get_actions(state)) {
        if (state.is_legal_action(cand)) legal_actions.push_back(cand);
    }

    return legal_actions;
}

PublicState ActionTree::get_public_state(const PokerState& state){

    PublicState pub_state{
        .street_idx = state.get_street(),
        .active_player = state.active_player,
        .payoffs = {state.get_payoff(0), state.get_payoff(1)},
        .folded = state.player_folded(),
        .edge_labels = {}
    };

    return pub_state;
}

size_t ActionTree::max_branching() const {

    std::vector<size_t> q;

    //vector of nodes w format (node_idx, node_depth)
    q.push_back(root_idx); 
    size_t max_branching = 0;

    while (q.size() != 0){
        size_t v = q[q.size()-1];
        q.pop_back();
        size_t node_branching = nodes[v].child_idxs.size();
        max_branching = std::max(max_branching, node_branching);

        for (size_t child_idx : nodes[v].child_idxs){
            q.push_back(child_idx);
        }
    }

    return max_branching;
}

size_t ActionTree::depth() const {

    std::vector<std::array<size_t, 2>> q;

    //vector of nodes w format (node_idx, node_depth)
    q.push_back({root_idx, 1}); 

    size_t max_depth = 0;

    while (q.size() != 0){
        std::array<size_t, 2> v = q[q.size()-1];
        q.pop_back();
        
        max_depth = std::max(max_depth, v[1]);
        for (size_t child_idx : nodes[v[0]].child_idxs){
            q.push_back({child_idx, v[1]+1});
        }
    }

    return max_depth;
}

ActionTree::ActionTree(const PokerState& root_state, const std::vector<std::vector<float>>& bet_szs):
    bet_sizes(bet_szs){

    std::mt19937 rng(0);
    root_idx = 0;

    nodes.push_back(TreeNode{0, 0, {}});
    pub_states.push_back(get_public_state(root_state));

    std::vector<std::pair<PokerState, size_t>> stack;  // (state, node idx)
    stack.push_back({root_state, root_idx});

    while (!stack.empty()) {
        auto [state, node_idx] = std::move(stack.back());
        stack.pop_back();

        if (state.is_terminal_node()) continue;

        if (state.is_chance_node()) {
            PokerState post_chance = state.apply_chance();
            pub_states[node_idx] = get_public_state(post_chance);
            stack.push_back({std::move(post_chance), node_idx});
            continue;
        }

        for (const Action& action : get_legal_actions(state)) {

            PokerState child = state.apply_action(action);
            size_t child_idx = nodes.size();

            nodes[node_idx].child_idxs.push_back(child_idx);
            pub_states[node_idx].edge_labels.push_back(action);

            nodes.push_back(TreeNode{child_idx, node_idx, {}});
            pub_states.push_back(get_public_state(child));

            stack.push_back({std::move(child), child_idx});
        }
    }

}