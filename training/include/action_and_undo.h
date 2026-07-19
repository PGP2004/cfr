
#pragma once
#include <vector>
#include <array>
#include <random>
#include <utility>   
#include <string>
#include <iostream>
#include <map>
#include <string>

#include <iostream>
#include <fstream>

struct Action{
    int type;
    int amt;
    bool operator==(const Action&) const = default;
};

struct ActionUndo {
    Action old_last_action{0, 0};
    int old_street = 0;
    int old_active_player = 0;
    int old_to_pay = 0;

    bool operator==(const ActionUndo&) const = default;
};

struct ChanceUndo {
    std::array<int, 2> old_pips{0, 0};
    std::array<int, 2> old_stacks{0, 0};
    int old_pot = 0;
    int old_active_player = 0;
    int old_street = 0;
    Action old_last_action{-1, -1};
    
    bool operator==(const ChanceUndo&) const = default;
    
};
