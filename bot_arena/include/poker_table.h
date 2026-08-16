#pragma once
#include "logger.h"
#include "cfr.h"
#include "action_tree.h"
#include "dealer.h"
#include "unordered_map"
#include "poker_state.h"

class PokerTable{
private:
   const ActionTree& action_tree;
   Dealer dealer;
   std::mt19937 rng;

public:
   PokerTable(const ActionTree& action_tree, Dealer dealer, std::mt19937 rng);

   std::pair<Action, size_t> query_user_action(const std::vector<Action>& actions,
      const PokerState& state, const Dealer& dealer, Logger& log);

   std::array<double,2>play_bot(PokerState init_state,
      int num_hands, const CFR& bot, Logger& log);

   std::array<double,2> bot_duel(int num_hands, const std::array<CFR, 2>& bots);

};