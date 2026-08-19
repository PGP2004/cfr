#pragma once
#include <string>
#include <vector>
struct Action;
class Dealer;
class PokerState;
class ActionTree;

struct Logger {

private:
    static constexpr const char* RULE = "RULE";

public:
    std::vector<std::string> lines;

    void rule(){ lines.push_back(RULE); }
    void push( std::string line){ lines.push_back(line); }
    void clear(){ lines.clear(); }
    bool empty()const { return lines.empty(); }

    void log_cards(const PokerState& state);
    void log_dealer(const PokerState& state);
    void log_action(const std::string& name, const Action& action);
    std::vector<Action> log_user_options(const PokerState& state);
    void log_showdown(const PokerState& state, int player);

    std::string render() const;
    void display() const; 

};