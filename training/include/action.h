#pragma once

struct Action{
    int type;
    int amt;
    bool operator==(const Action&) const = default;
};