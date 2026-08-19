#include "logger.h"
#include "poker_state.h"
#include "dealer.h"
#include "action.h"
#include <algorithm>
#include <iostream>

void Logger::log_cards(const PokerState& state) {
    int me  = state.active_player;
    int opp = 1 - me;

    lines.push_back("Pot : " + std::to_string(state.pot));
    lines.push_back("You | PIP : " + std::to_string(state.pips[me])
        + " | Stack : "  + std::to_string(state.stacks[me]));
    lines.push_back("Opp | PIP : " + std::to_string(state.pips[opp])
         + " | Stack : "  + std::to_string(state.stacks[opp]));
}

void Logger::log_dealer(const PokerState& state) {
    const Dealer& d = state.get_dealer();
    int player = state.active_player;
    int street = state.get_street();

    std::vector<uint8_t> cards = state.get_cards(player);

    if (cards.size() >= 2){
        lines.push_back("Hand : " + card_string(cards[0]) + " "
                + card_string(cards[1]));
    }
    
    if (cards.size() > 2) {
        std::string board = "Board :";
        for (int i = 0; i < cards.size(); ++i)
            board += " " + card_string(d.cards[player][2 + i]);
        lines.push_back(board);
    }
}

void Logger::log_action(const std::string& name, const Action& action) {
    lines.push_back(name + " " + action.to_string());
    rule();
}


std::vector<Action> Logger::log_user_options(const PokerState& state){
    auto [min_raise, max_raise] = state.get_raise_bounds();

    std::vector<Action> options;
    for (const Action& a : {Action{0,0}, Action{1,0}, Action{2,0}})
        if (state.is_legal_action(a)) options.push_back(a);
    if (min_raise <= max_raise) options.push_back({3, min_raise});

    lines.push_back("Choose Your Action:");
    for (size_t i = 0; i < options.size(); ++i) {
        std::string label;
        if (options[i].type == 3)
            label = "raise to " + std::to_string(min_raise) + " - " + std::to_string(max_raise);
        else
            label = options[i].to_string();
        lines.push_back(std::to_string(i) + " - " + label);
    }
    return options;
}

void Logger::log_showdown(const PokerState& state, int player) {
    int opp = 1 - player;

    log_dealer(state);

    if (state.player_folded()){
        lines.push_back("Opp Hand : (folded)");
    }

    else{
        std::vector<uint8_t> opp_cards = state.get_cards(opp);
        lines.push_back("Opp Hand : " + card_string(opp_cards[0]) + " "
            + card_string(opp_cards[1]));
    }
    rule();

    double reward = state.get_reward(player);
    if (reward > 0) lines.push_back(std::format("Chips won this hand  : {:.2f}", reward));
    else if (reward < 0) lines.push_back(std::format("Chips lost this hand : {:.2f}", reward < 0 ? -reward : reward));
    else lines.push_back("Split pot : 0.00");
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