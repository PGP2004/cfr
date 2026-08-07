#pragma once

struct Action{
    int type;
    int amt;
    bool operator==(const Action&) const = default;
    // type: fold=0, check=1, call=2, raise=3, second argument is the amount
};