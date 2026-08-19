#include "poker_table.h"
#include "action_tree.h"
#include "poker_state.h"
#include "evaluator.h"
#include <iostream>
#include <tuple>

Action query_user_action(const PokerState& state, Logger& log) {

    log.log_cards(state);
    log.rule();
    log.log_dealer(state);
    log.rule();
    const std::vector<Action> options = log.log_user_options(state);
    log.display();
    log.clear();

    std::string in;
    int choice = -1;
    while (std::getline(std::cin, in)) {
        try { choice = std::stoi(in); } catch (...) { choice = -1; }
        if (choice >= 0 && choice < (int)options.size()) break;
        std::cout << "invalid, retry: " << std::flush;
    }

    Action action = options[choice];
    if (action.type == 3) {
        auto [min_raise, max_raise] = state.get_raise_bounds();
        std::cout << "raise to [" << min_raise << ", " << max_raise << "]: " << std::flush;
        while (std::getline(std::cin, in)) {
            try { action.amt = std::stoi(in); } catch (...) { action.amt = -1; }
            if (action.amt >= min_raise && action.amt <= max_raise) break;
            std::cout << "invalid, retry: " << std::flush;
        }
    }

    return action;
}

std::array<double,2> human_vs_bot(PokerState& root_state, std::mt19937& rng,  Logger& log,
    int num_hands, Agent& bot) {

    std::array<double,2> rewards{0, 0};
    int human_seat = 0;

    for (int h = 0; h < num_hands; ++h) {

        PokerState state{root_state};
        bot.reset();
        human_seat = 1 - human_seat;
        log.clear();

        while (!state.is_terminal()) {

            if  (state.is_chance()){
                state = state.apply_chance(rng);
                continue;
            }

            if (state.is_terminal()) break;

            const int player = state.active_player;

            Action action;

            if (player == human_seat) {
                action = query_user_action(state, log);
                log.log_action("You", action);
            } else {
                action = bot.get_action(state);
                log.log_action("Opp", action);
            }

            bot.update_on_action(state, action);
            state = state.apply_action(action);
        }

        log.log_showdown(state, human_seat);
        log.rule();
        log.push("press enter for next hand");
        log.display();
        log.clear();

        std::string dummy;
        std::getline(std::cin, dummy);

        rewards[0] += state.get_reward(human_seat);
        rewards[1] += state.get_reward(1 - human_seat);
    }

    return rewards;
}


std::array<double,2> bots_vs_bot( const PokerState& root_state, std::mt19937& rng,
    int num_hands, std::array<Agent*,2> bots) {

    std::array<double,2> rewards{0, 0};

    for (int h = 0; h < num_hands; ++h) {

        PokerState state{root_state};
        bots[0]->reset();
        bots[1]->reset();

        const int b0_seat = h % 2;

        while (!state.is_terminal()) {

            if (state.is_chance()){
                state = state.apply_chance(rng);
                continue;
            }

            const int actor = (state.active_player == b0_seat) ? 0 : 1;
            const Action action = bots[actor]->get_action(state);

            bots[0]->update_on_action(state, action);
            bots[1]->update_on_action(state, action);
            state = state.apply_action(action);
        }

        rewards[0] += state.get_reward(b0_seat);
        rewards[1] += state.get_reward(1 - b0_seat);
    }

    return rewards;
}