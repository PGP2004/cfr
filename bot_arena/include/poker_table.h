#pragma once
#include "logger.h"
#include "cfr.h"
#include "action_tree.h"
#include "dealer.h"
#include "unordered_map"
#include "poker_state.h"
#include "agent.h"

Action query_user_action(const PokerState& state, Logger& log);
std::array<double,2> human_vs_bot( PokerState root_state, std::mt19937& rng, Logger& log, int num_hands, Agent& bot);
std::array<double,2> bot_vs_bot(const PokerState root_state, std::mt19937& rng, int num_hands, std::array<Agent&,2> bots);