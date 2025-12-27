#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <random>

using std::vector;
using std::string;
using std::mt19937;

struct Deck{

    private: 

    size_t top_idx;

    vector<string> deck;

    public:

    Deck(vector<string> cards);

    void shuffle_remaining();
    string deal();

    void shuffle_remaining(mt19937& rng);
    void reset(mt19937& rng);

    vector<string> read_top_cards(size_t n) const;

    size_t size() const ;

    size_t remaining() const;
};

