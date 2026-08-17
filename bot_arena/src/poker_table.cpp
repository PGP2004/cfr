#include "poker_table.h"
#include "action_tree.h"
#include "evaluator.h"
#include <iostream>
#include <tuple>

PokerTable::PokerTable(const ActionTree& action_tree, Dealer dealer, std::mt19937 rng):
    action_tree(action_tree), dealer(dealer), rng(rng) {}

std::pair<Action, size_t> PokerTable::query_user_action(const std::vector<Action>& actions,
    const PokerState& state, const Dealer& dealer, Logger& log) {

    log.log_state(state);
    log.rule();
    log.log_dealer(dealer, state.active_player, state.get_street());
    log.rule();
    log.log_user_options(actions);

    log.display();
    log.clear();

    std::string in;
    int choice = 0;
    while (std::getline(std::cin, in)) {
        try {
            choice = std::stoi(in);
            if (choice >= 0 && choice < (int)actions.size()) break;
        } catch (...) {}
        std::cout << "invalid, retry: " << std::flush;
    }

    return {actions[choice], static_cast<size_t>(choice)};
}

std::array<double,2> PokerTable::play_bot(PokerState init_state,const CFR& bot, 
    Logger& log, int num_hands, int rng_seed) {

    rng.seed(rng_seed);
    std::array<double,2> rewards{0, 0};
    int human = 0;

    PokerState state{init_state};

    for (int h = 0; h < num_hands; ++h) {

        dealer.deal(rng);
        state = init_state;
        size_t node_idx = action_tree.root_idx;
        human = 1 - human;
        log.clear();

        while (!action_tree.is_terminal(node_idx)) {

            int player = action_tree.active_player(node_idx);
            Action action;
            size_t action_idx;

            while (state.is_chance_node())
                state = state.apply_chance();

            if (player == human) {
                const std::vector<Action> options = action_tree.get_actions(node_idx);
                std::tie(action, action_idx) = query_user_action(options, state, dealer, log);
                log.log_action("You", action);
            } else {
                std::tie(action, action_idx) = bot.sample_strategy(node_idx, dealer, rng);
                log.log_action("Opp", action);
            }

            node_idx = action_tree.apply_action(node_idx, action_idx);
            state = state.apply_action(action);
        }

        log.log_showdown(dealer, action_tree, node_idx, human);
        log.rule();
        log.push("press enter for next hand");
        log.display();
        log.clear();

        std::cout << "--------------" <<std::endl;
        std::cout << "rewward for human: " << dealer.get_reward(node_idx, human, action_tree) << std::endl;
        std::cout << "rewward for bot: " << dealer.get_reward(node_idx, 1-human, action_tree) << std::endl;

        std::string dummy;
        std::getline(std::cin, dummy);

        rewards[0] += dealer.get_reward(node_idx, human, action_tree);
        rewards[1] += dealer.get_reward(node_idx, 1-human, action_tree);
    }

    return rewards;
}

std::array<double,2> PokerTable::bot_duel(const std::array<CFR,2>& bots,
    int num_hands, int rng_seed) {
    rng.seed(rng_seed);                      // make runs comparable
    std::array<double,2> rewards{0, 0};
    int p0_bot = 0;
    int cur_bot;


    for (int h = 0; h < num_hands; ++h) {
        dealer.deal(rng);
        size_t node_idx = action_tree.root_idx;
        p0_bot = 1 - p0_bot;

        while (!action_tree.is_terminal(node_idx)) {
            if (action_tree.active_player(node_idx) == 0){
                cur_bot = p0_bot;
            }
            else{
                cur_bot = 1-p0_bot;
            }

            auto [ action, action_idx] = bots[cur_bot].sample_strategy(node_idx, dealer, rng);
            node_idx = action_tree.apply_action(node_idx, action_idx);
        }

        rewards[p0_bot] += dealer.get_reward(node_idx, 0, action_tree);
        rewards[1 - p0_bot] += dealer.get_reward(node_idx, 1, action_tree);
    }
    return rewards;
}