#include "logger.h"
#include "poker_state.h"
#include "dealer.h"
#include "action.h"
#include <algorithm>
#include <iostream>

void Logger::log_state(const PokerState& state) {
    int me  = state.active_player;
    int opp = 1 - me;

    lines.push_back("Pot : " + std::to_string(state.pot));
    lines.push_back("You | PIP : " + std::to_string(state.pips[me])
        + " | Stack : "  + std::to_string(state.stacks[me]));
    lines.push_back("Opp | PIP : " + std::to_string(state.pips[opp])
         + " | Stack : "  + std::to_string(state.stacks[opp]));
}

void Logger::log_dealer(const Dealer& d, int player, int street) {
    lines.push_back("Hand : " + card_string(d.cards[player][0]) + " "
                              + card_string(d.cards[player][1]));

    std::vector<int> board_cards_per_st = {0, 3, 4, 5, 5};
    int n = board_cards_per_st[street];
    if (n > 0) {
        std::string board = "Board :";
        for (int i = 0; i < n; ++i)
            board += " " + card_string(d.cards[player][2 + i]);
        lines.push_back(board);
    }
}

void Logger::log_action(const std::string& name, const Action& action) {
    lines.push_back(name + " " + action.to_string());
    rule();
}

void Logger::log_user_options(const std::vector<Action>& actions) {
    lines.push_back("Choose Your Action:");
    for (size_t i = 0; i < actions.size(); ++i)
        lines.push_back(std::to_string(i) + " - " + actions[i].to_string());
}

void Logger::log_showdown(const Dealer& d, const ActionTree& action_tree, size_t node_idx, int human) {
    int opp = 1 - human;

    log_dealer(d, human, action_tree.street(node_idx));

    if (action_tree.is_folded(node_idx)){
        lines.push_back("Opp Hand : (folded)");
    }
    else{
        lines.push_back("Opp Hand : " + card_string(d.cards[opp][0]) + " "
            + card_string(d.cards[opp][1]));
    }
    rule();

    double reward = d.get_reward(node_idx, human, action_tree);
    if (reward > 0) lines.push_back(std::format("You win  {:.2f}", reward));
    else if (reward < 0) lines.push_back(std::format("You lose {:.2f}", -reward));
    else lines.push_back("Split pot");
}

std::string Logger::render() const {
    size_t width = 0;
    for (const auto& line : lines)
        if (line != RULE)
            width = std::max(width, line.size());

    const std::string bar = "+" + std::string(width + 2, '-') + "+\n";

    std::string out = bar;
    for (const auto& line : lines) {
        if (line == RULE) out += bar;
        else out += "| " + line + std::string(width - line.size(), ' ') + " |\n";
    }
    out += bar;
    return out;
}

void Logger::display() const {
    std::cout << "\033[2J\033[3J\033[H" << render() << std::flush;
}