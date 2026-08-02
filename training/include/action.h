#pragma once
#include <map>
#include <string>
#include <iostream>

struct Action{
    int type;
    int amt;
    bool operator==(const Action&) const = default;


    void print() const {
        // type: fold=0, check=1, call=2, raise=3
        static const std::map<int, std::string> type_name = {
            {-1, "placeholder"},
            {0, "fold"},
            {1, "check"},
            {2, "call"},
            {3, "raise"},
        };

        auto it = type_name.find(type);
        const std::string name = (it == type_name.end() ? "UNKNOWN" : it->second);

        std::cout << "Action(type=" << name << ", amt=" << amt << ")"; // flush
    }
};