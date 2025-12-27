#pragma once

#include <vector>
#include <string>

using std::vector;
using std::string;


struct Action{

private: 

    string action_type;

    int bet_amount;

    string action_description;
    
public:

    Action(string input_type, int amount = 0);

    const string& type() const; 

    int amount() const;

    const string& description() const;

    bool operator==(const Action& other) const; 
};

