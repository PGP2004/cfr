
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <chrono>

#include "card_buckets.h"
#include "cfr.h"
#include "game_state.h"
#include "info_sets.h"
#include "action_tree.h"
#include "trainer.h"

namespace fs = std::filesystem;

static std::string fmt_iters(int n) {
    if (n % 1'000'000'000 == 0 && n >= 1'000'000'000)
        return std::to_string(n / 1'000'000'000) + "B";
    if (n % 1'000'000 == 0 && n >= 1'000'000)
        return std::to_string(n / 1'000'000) + "M";
    if (n % 1'000 == 0 && n >= 1'000)
        return std::to_string(n / 1'000) + "k";
    return std::to_string(n);
}


//clauded need to do more carefully
void write_preflop_csv(const std::string& path, const CFR& cfr) {
    static const std::array<uint8_t, 1> cpr = {2};
    static Indexer idx{1, cpr.data()};
    static const std::array<std::string, 13> rank = {
        "2","3","4","5","6","7","8","9","T","J","Q","K","A"};

    const ActionTree& at = cfr.get_action_tree();
    const InfoSets& isets = cfr.get_infosets();
    const size_t root_idx = at.root_idx;
    const std::vector<Action>& actions = at.pub_states[root_idx].edge_labels;

    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot open " + path);

    out << "hand";
    for (const Action& a : actions){
        out << ',' << a.to_string();
    }
    out << '\n';

    std::vector<double> strat;
    auto record = [&](const std::string& name, uint8_t c0, uint8_t c1) {
        uint8_t cards[2] = {c0, c1};
        InfoKey key{root_idx, hand_index_last(&idx.h, cards), actions.size()};
        isets.get_strategy(key, strat);
        out << name;
        for (double p : strat) out << ',' << p;
        out << '\n';
    };

    //pocket pairs!
    for (uint8_t r = 0; r < rank.size(); ++r){
        record(rank[r] + rank[r], make_card(r, 0), make_card(r, 1));
    }

    //non-pocket pairs
    for (uint8_t hi = 1; hi < rank.size(); ++hi)
        for (uint8_t lo = 0; lo < hi; ++lo) {
            const std::string base = rank[hi] + rank[lo];
            //suited
            record(base + "s", make_card(hi, 0), make_card(lo, 0));
            //offsuit
            record(base + "o", make_card(hi, 0), make_card(lo, 1));
        }

    if (!out) throw std::runtime_error("write failed: " + path);
}

std::pair<fs::path, fs::path> make_run_dirs(const Trainer& trainer) {

    fs::path preflop_path = trainer.runs_folder / trainer.run_name / "preflop";
    fs::path infosets_path = trainer.runs_folder / trainer.run_name / "infosets";

    std::pair<fs::path, fs::path> output{preflop_path, infosets_path};

    if (trainer.preflop_ckpts.size() > 0){
        if (fs::exists(preflop_path)) throw std::runtime_error("run folder already exists: " + preflop_path.string());
        fs::create_directories(preflop_path);
    }

    if (trainer.store_infosets){
        if (fs::exists(preflop_path)) throw std::runtime_error("run folder already exists: " + preflop_path.string());
        fs::create_directories(preflop_path);
    }

    return output;
}


void run_training(const Trainer& trainer) {

    int iters_per_discount = 10'000;

    std::pair<fs::path, fs::path> new_paths = make_run_dirs(trainer);
    fs::path preflop_path = new_paths.first;
    fs::path infosets_path = new_paths.second;

    ActionTree at{trainer.game_state, trainer.bet_sizes};
    CardBuckets buckets(trainer.bp);
    CFR cfr{std::move(buckets), std::move(at)};

    int cur_iter = 0;

    for (int ckpt : trainer.preflop_ckpts) {
        if (ckpt > trainer.max_iters) break;

        cfr.train(ckpt - cur_iter, iters_per_discount);
        cur_iter = ckpt;

        std::string csv_name = "iter_"  + fmt_iters(cur_iter) + ".csv";
        std::string csv_path = (preflop_path / csv_name).string();
        
        write_preflop_csv(csv_path, cfr);
    }
}