#include "indexer.h"
#include "dataloader.h"
#include "evaluator.h"

#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;
using namespace std;

void get_strength_cdf(const array<uint8_t, 6>& cards, uint8_t num_buckets,
                      const vector<uint8_t>& strengths, hand_indexer_t& river_indexer,
                      array<bool, 52>& missing, vector<uint8_t>& cdf) {
    //fills the cdf vector passed as arg

    const int strength_max = 100;
    const int deck_size = 52;

    array<uint8_t, 7> sim_board;
    for (size_t i = 0; i < cards.size(); ++i)
        sim_board[i] = cards[i];

    missing.fill(false);
    for (uint8_t card : cards)
        missing[card] = true;

    //Note the naming is a bit confusing here: first I use the cdf to store the unnormalized pdf,
    //then later I cumsum to turn it into a (unnormalized) cdf 

    fill(cdf.begin(), cdf.end(), uint8_t{0});

    for (uint8_t c1 = 0; c1 < deck_size; ++c1) {
        if (missing[c1]) continue;
        sim_board[6] = c1;

        hand_index_t idx = hand_index_last(&river_indexer, sim_board.data());
        uint8_t strength = strengths[idx];

        uint8_t bucket = static_cast<uint8_t>((strength * num_buckets) / (strength_max + 1));
        ++cdf[bucket];
    }

    for (size_t i = 1; i < cdf.size(); ++i)
        cdf[i] = static_cast<uint8_t>(cdf[i] + cdf[i - 1]);
}

void write_turn_strength_cdf(const string& river_strength_path, const string& write_path,
                             uint8_t num_buckets) {

    if (fs::exists(write_path))
        throw runtime_error("write path already exists");

    auto [strengths, river_header] = load_matrix_and_header<uint8_t>(river_strength_path);

    array<uint8_t, 2> river_cpr = {2, 5};
    Indexer river_indexer(river_cpr.size(), river_cpr.data()); 
    
    array<uint8_t, 2> turn_cpr  = {2, 4};
    Indexer turn_indexer(turn_cpr.size(), turn_cpr.data());

    const hand_index_t total_turns = hand_indexer_size(&turn_indexer.h, 1);   
    //napkin math: for the turn, total_turns is around 13 million.
    //the number of buckets should always be < 100 so the product above should not overflow a uint64_t

    ofstream out(write_path, ios::binary);
    if (!out) throw runtime_error("cant open the path:  " + write_path);    
   
    DataHeader turn_cdf_header{static_cast<uint64_t>(total_turns), num_buckets, sizeof(uint8_t)};

    cout << "The turn cdfs header is: " << endl;
    cout << turn_cdf_header.to_string() << endl;
    // num_cols=buckets  num_rows=total_turns
    out.write(reinterpret_cast<const char*>(&turn_cdf_header), sizeof(turn_cdf_header));    

    array<bool, 52> missing;
    vector<uint8_t> cdf(num_buckets);
    array<uint8_t, 6> cards;
    
    for (hand_index_t i = 0; i < total_turns; ++i) {
        hand_unindex(&turn_indexer.h, 1, i, cards.data());                           
        get_strength_cdf(cards, num_buckets, strengths, river_indexer.h, missing, cdf);
        out.write(reinterpret_cast<const char*>(cdf.data()), num_buckets);
    }
}


int main(int , char** argv) {
    cout << "Started" << endl;
    fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
    fs::path root = exe.parent_path().parent_path().parent_path();                           
    fs::path storage = root / "storage";

    uint8_t num_buckets = 20;

    write_turn_strength_cdf((storage / "river_strengths").string(),
                            (storage / "turn_cdfs").string(),
                            num_buckets);
    cout << "Finished" << endl;
}
