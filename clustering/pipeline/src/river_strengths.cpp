#include "pipeline.h"
#include "indexer.h"
#include "dataloader.h"
#include "evaluator.h"

#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int get_strength(std::array<uint8_t, 7>& board) {

    uint32_t board_strength = evaluate(board);
    int deck_size = 52;
    int shared_cards = 5;

    std::array<uint8_t, 7> opp_board;
    for (uint8_t i = 0; i < shared_cards; i++) {
        opp_board[i] = board[i + 2];
    }

    std::vector<bool> missing(deck_size);
    for (int card : board) missing[card] = true;

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
    uint8_t strength = static_cast<int>(100*win_rate);
    return strength;
}

void run_river_strengths(const PipelineConfig& cfg) {
    if (fs::exists(cfg.art.river_strengths)){
        throw std::runtime_error("write path already exists: " + cfg.art.river_strengths.string());
    }

    std::array<uint8_t, 2> cards_per_round = {2, 5};
    Indexer indexer(cards_per_round.size(), cards_per_round.data());

    const uint32_t round = 1;
    const hand_index_t total = hand_indexer_size(&indexer.h, round);

    std::ofstream out(cfg.art.river_strengths, std::ios::binary);
    if (!out) throw std::runtime_error("cant open the write path: " + cfg.art.river_strengths.string());

    DataHeader header{static_cast<uint64_t>(total), 1, sizeof(int)};
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    std::array<uint8_t, 7> cards;
    for (hand_index_t i = 0; i < total; ++i) {
        hand_unindex(&indexer.h, round, i, cards.data());
        int strength = get_strength(cards);
        out.write(reinterpret_cast<const char*>(&strength), sizeof(strength));
    }

    if (!out) throw std::runtime_error("write failed partway through: " + cfg.art.river_strengths.string());
}