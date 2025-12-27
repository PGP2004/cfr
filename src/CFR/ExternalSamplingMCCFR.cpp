#include "CFR/ExternalSamplingMCCFR.h"
#include <memory>
#include <utility>
#include <iostream>

//types included for templating later,
#include "KuhnPoker/KuhnInfoSet.h"
#include "RockPaperScissors/RPSInfoSet.h"

using namespace std;

template <typename InfoSetT>
ExternalSamplingMCCFR<InfoSetT>::ExternalSamplingMCCFR(uint32_t seed, unique_ptr<GameState> init_game_state)
    : init_state(std::move(init_game_state)), rng(seed) {}


template <typename InfoSetT>    
string ExternalSamplingMCCFR<InfoSetT>::get_InfoSetID(int player, const GameState& state){

    string output = "";

    output += state.get_hand(player);
    output += "|";

    for (const Action& action : state.get_action_history()){
        output += action.description();
        output += ","; 
    }

    return output;
}

template <typename InfoSetT>  
InfoSetT& ExternalSamplingMCCFR<InfoSetT>::get_InfoSet(int player, const GameState& state) {
    string id = get_InfoSetID(player, state);

    unordered_map<string, InfoSetT>& mp = infoset_dict[player];
    pair< typename unordered_map<string, InfoSetT>::iterator, bool> res = mp.try_emplace(id, state);   
    typename unordered_map<string, InfoSetT>::iterator it = res.first;
    return it->second;
}

template <typename InfoSetT>  
double ExternalSamplingMCCFR<InfoSetT>::traverse(int player, const GameState&state, double pi_i, double pi_opp){

    if (state.is_terminal()) {
        string id = get_InfoSetID(player, state);
        return state.get_rewards(player);
    }

    if (state.is_chance_node()){
        unique_ptr<GameState> next_state = state.sample_chance_node(rng);
        return traverse(player, *next_state, pi_i, pi_opp);
    }

    int current_player = state.get_active_player();

    if (current_player != player){

        InfoSetT& infoset = get_InfoSet(1-player, state);
        auto [sampled_action, sample_prob] = infoset.sample_regret_action(rng);  //should i sample from my regret_sum or my strategy_sum;
        unique_ptr<GameState> next_state = state.next_game_state(sampled_action);
        return traverse(player, *next_state, pi_i, pi_opp * sample_prob);
    }

    if (current_player == player){

        InfoSetT& infoset = get_InfoSet(player, state);

        vector<pair<Action, double>> actions_with_probs = infoset.get_actions_with_probs();
        double node_util = 0.0;
        vector<pair<Action, double>> action_utils;
        
        for (const auto& [action, prob] : actions_with_probs){
            unique_ptr<GameState> next_state = state.next_game_state(action);
            double action_util = traverse(player, *next_state, pi_i * prob, pi_opp);

            node_util += prob*action_util;
            action_utils.push_back({action, action_util});
        }

        for (const auto& [action, util] : action_utils) {
            infoset.update_regret(action, pi_opp * (util - node_util));
        }

        infoset.update_average_strategy(pi_i);
        return node_util;
    }

    throw logic_error("In theory should not get here in traverse");
    return 0.0;
} 

template <typename InfoSetT> 
void ExternalSamplingMCCFR<InfoSetT>::train(int num_iterations){

    for (int i = 0; i < num_iterations; i++){
        traverse(0, *init_state, 1.0, 1.0);
        traverse(1, *init_state, 1.0, 1.0);
    }
}

template <typename InfoSetT> 
const array<unordered_map<string, InfoSetT>, 2>& ExternalSamplingMCCFR<InfoSetT>::view_infosets() const{
    return infoset_dict;
}

template class ExternalSamplingMCCFR<KuhnInfoSet>;
template class ExternalSamplingMCCFR<RPSInfoSet>;
