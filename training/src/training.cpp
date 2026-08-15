
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

#include "card_buckets.h"
#include "cfr.h"
#include "info_sets.h"
#include "action_tree.h"
#include "training.h"

namespace fs = std::filesystem;

CFR load_spec(CFRSpec spec) {
    CardBuckets buckets{spec.bucket_paths};
    ActionTree action_tree{GameState{}, spec.bet_sizes};

    if (spec.isets_paths) {
        InfoSets isets{*spec.isets_paths};
        return CFR{std::move(isets), std::move(buckets), std::move(action_tree)};
    }

    return CFR{std::move(buckets), std::move(action_tree)};
}

void set_up_directories(const LogParams& lp) {
    std::vector<fs::path> paths;

    if (lp.preflop_path){

        fs::path p = *lp.preflop_path;
        if (!lp.overwrite_preflop && fs::exists(p)){
            throw std::runtime_error("Cannot write to " + p.string() +" since the path already exists");
        }

        if (p.has_parent_path()) fs::create_directories(p.parent_path());
    }

    if (! lp.isets_paths) return;

    std::vector<fs::path> from_isets;
    from_isets.push_back(lp.isets_paths->regret_path);
    from_isets.push_back(lp.isets_paths->strategy_path);
    from_isets.push_back(lp.isets_paths->offset_path);
    from_isets.push_back(lp.isets_paths->iters_path);

    if (!lp.overwrite_isets){
        for (const auto& p : from_isets) {
            if (fs::exists(p)){
                throw std::runtime_error("Cannot write to " + p.string() +" since the path already exists");
            }
        }
    }

    for (const auto& path : paths) {
        if (path.has_parent_path())
            fs::create_directories(path.parent_path());
    }
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
            record(base + "s", make_card(hi, 0), make_card(lo, 0)); //suited
            record(base + "o", make_card(hi, 0), make_card(lo, 1)); //offsuit
        }

    if (!out) throw std::runtime_error("write failed: " + path);
}

void run_training(const CFRSpec& spec, const TrainParams& tp, const LogParams& lp){

    set_up_directories(lp);
    CFR cfr = load_spec(spec);

    cfr.train(tp.train_iters, tp.iters_per_discount, 
        tp.num_threads, tp.omp_chunk_sz, tp.base_seed);

    if (lp.preflop_path){
        write_preflop_csv(*lp.preflop_path, cfr);
    }

    if (lp.isets_paths){
        const InfoSets& isets = cfr.get_infosets();
        isets.write_ckpt(*lp.isets_paths);
    }
}