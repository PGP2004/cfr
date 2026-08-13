
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

    int b = 1'000'000'000; 
    int m = 1'000'000;
    int k = 1'00;

    if (n % b == 0 && n >= b) return std::to_string(n / b) + "B";
    if (n % m == 0 && n >= m) return std::to_string(n / m) + "M";
    if (n % k == 0 && n >= k) return std::to_string(n / 1'000) + "k";

    return std::to_string(n);
}

CFR load_ckpt(CFRSpec spec) {
    CardBuckets buckets{spec.bucket_paths};
    ActionTree action_tree{GameState{}, spec.bet_sizes};

    if (spec.isets_paths) {
        InfoSets isets{*spec.isets_paths};
        return CFR{std::move(isets), std::move(buckets), std::move(action_tree)};
    }

    return CFR{std::move(buckets), std::move(action_tree)};
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


std::pair<fs::path, fs::path> set_up_directories(const LogParams& log_params) {

    fs::path run_root = log_params.folder / log_params.run_name;
    fs::path preflop_path = run_root / "preflop";
    fs::path infosets_path = run_root / "infosets";

    if (fs::exists(run_root))
        throw std::runtime_error("run folder already exists: " + run_root.string());

    if (!log_params.preflop_ckpts.empty())
        fs::create_directories(preflop_path);

}


void run_training(CFRSpec spec, const LogParams& log_params, int iters_per_discount){

    set_up_directories(log_params);
    CFR cfr = load_cfr(spec);

    int cur_iter = 0;

    //finish this loading function!
}