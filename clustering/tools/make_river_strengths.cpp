#include <array>
#include "indexer.h"
#include "dataloader.h"
#include "evaluator.h"

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}

namespace fs = std::filesystem;
using namespace std;

int get_strength(std::array<uint8_t, 7>& board) {
    uint32_t board_strength = evaluate(board);

    int deck_size = 52;
    int shared_cards = 5;

    array<uint8_t, 7> opp_board;
    for (uint8_t i = 0; i < shared_cards; i++) {
        opp_board[i] = board[i + 2];
    }

    vector<bool> missing(deck_size);
    for (int card : board) {
        missing[card] = true;
    }

    double score = 0.0;
    double num_opps = 0;
    for (int c1 = 0; c1 < deck_size; ++c1) {
        if (missing[c1]) continue;
        for (int c2 = c1 + 1; c2 < deck_size; ++c2) {
            if (missing[c2]) continue;
            opp_board[shared_cards] = c1;
            opp_board[shared_cards + 1] = c2;
            uint32_t opp_strength = evaluate(opp_board);
            num_opps++;

            if (board_strength > opp_strength) score += 1.0;
            else if (board_strength == opp_strength) score += 0.5;
        }
    }

    double win_rate = score / num_opps;
    uint8_t strength = static_cast<uint8_t>(100*win_rate);
    return strength;
}

void write_strengths(const string& write_path) {
    array<uint8_t, 2> cards_per_round = {2, 5};
    Indexer indexer(cards_per_round.size(), cards_per_round.data());

    const uint32_t round = 1;
    const hand_index_t total = hand_indexer_size(&indexer.h, round);

    if (fs::exists(write_path))
        throw runtime_error("write path already exists");

    ofstream out(write_path, ios::binary);
    
    if (!out) throw runtime_error("cant open the write path");

    DataHeader header{static_cast<uint64_t>(total), 1 , sizeof(uint8_t)};
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    array<uint8_t, 7> cards;

    for (hand_index_t i = 0; i < total; ++i) {
        hand_unindex(&indexer.h, round, i, cards.data());
        uint8_t strength = get_strength(cards);
        out.write(reinterpret_cast<const char*>(&strength), 1);
    }
}

int main(int, char** argv) { 
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path();         
    fs::path storage = root / "storage";

    write_strengths((storage / "river_strengths").string());

    cout << "Finished" << endl;
}
