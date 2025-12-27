#include "Deck.h"
#include <random>
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

Deck::Deck(vector<string> cards){
    deck = cards;
    top_idx = 0;
}

void Deck::shuffle_remaining(mt19937& rng) {
    shuffle(deck.begin() + top_idx, deck.end(), rng);
}

void Deck::reset(mt19937& rng){
    top_idx = 0;
    shuffle_remaining(rng);
}

string Deck::deal(){

    if (top_idx >= deck.size()){
       throw out_of_range("No more cards to deal");
    }

    string output = deck[top_idx];
    top_idx += 1;

    return output;
}

vector<string> Deck::read_top_cards(size_t n) const{

   
    if (n + top_idx > deck.size()){
        throw out_of_range("Not enough cards to read");
    }

    vector<string> output;

    for (size_t i = top_idx; i < top_idx + n; i++){
        output.push_back(deck[i]);
    }

    return output;
}

size_t Deck::size() const{
    return deck.size();
}

size_t Deck:: remaining() const{
    return deck.size() - top_idx;
}



