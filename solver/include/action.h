#pragma once
#include <string>
#include <stdexcept>

struct Action{
    int type;
    int amt;
    bool operator==(const Action&) const = default;
    
    std::string to_string() const {
        switch (type) {
            case 0: return "fold";
            case 1: return "check";
            case 2: return "call";
            case 3: return "raise to " + std::to_string(amt);
            default: return "unknown";
        }
    }
    // type: fold=0, check=1, call=2, raise=3, second argument is the amount
    // (3,x) = raise to x
};