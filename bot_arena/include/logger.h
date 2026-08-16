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
    void push(std::string line){ lines.push_back(line); }
    void clear(){ lines.clear(); }
    bool empty()const { return lines.empty(); }

    void log_state(const PokerState& state);
    void log_dealer(const Dealer& d, int player, int street);
    void log_action(const std::string& name, const Action& action);
    void log_user_options(const std::vector<Action>& actions);
    void log_showdown(const Dealer& d, const ActionTree& action_tree, size_t node_idx, int human);

    std::string render() const;
    void display() const; 

};