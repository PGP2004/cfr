
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <toml.hpp>

#include "card_buckets.h"
#include "cfr.h"
#include "info_sets.h"
#include "action_tree.h"
#include "training.h"

namespace fs = std::filesystem;

CFR load_spec(CFRSpec spec) {
    CardBuckets buckets{spec.bucket_paths};
    PokerState init_state{spec.starting_stack, spec.big_blind, spec.small_blind}; 
    ActionTree action_tree{init_state, spec.bet_sizes};

    if (spec.isets_paths) {
        InfoSets isets{*spec.isets_paths};
        return CFR{std::move(isets), std::move(buckets), std::move(action_tree)};
    }

    return CFR{std::move(buckets), std::move(action_tree)};
}

void set_up_directories(const TrainLog& lp) {

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

    for (const auto& p : from_isets) {
        if (p.has_parent_path())
            fs::create_directories(p.parent_path());
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

void run_training(const CFRSpec& spec, const TrainParams& tp, const TrainLog& lp){

    set_up_directories(lp);
    CFR cfr = load_spec(spec);

    cfr.train(tp.train_iters, tp.iters_per_discount, 
        tp.num_threads, tp.omp_chunk_sz, tp.base_seed);

    if (lp.preflop_path){

        if (lp.overwrite_preflop)fs::remove(*lp.preflop_path);
        write_preflop_csv(*lp.preflop_path, cfr);
    }

    if (lp.isets_paths){
        if (lp.overwrite_isets) (*lp.isets_paths).remove();
        const InfoSets& isets = cfr.get_infosets();
        isets.write_ckpt(*lp.isets_paths);
    }
}

/// ---------------------------- TOML Setup Code! ------------------------------------

static std::vector<float> toml_float_vec(const toml::array& a) {
    std::vector<float> out;
    for (const toml::node& v : a) out.push_back(v.value<float>().value());
    return out;
}

CFRSpec load_cfr_config(const std::filesystem::path& cfr_toml_path, const std::filesystem::path& root){

    toml::table toml = toml::parse_file(cfr_toml_path.string());
    CFRSpec spec;
    spec.bucket_paths = {
        .flop_path = root/toml["buckets"]["flop"].value<std::string>().value(),
        .turn_path = root/toml["buckets"]["turn"].value<std::string>().value(),
        .river_path = root/toml["buckets"]["river"].value<std::string>().value()
    };

    spec.starting_stack = toml["game_values"]["starting_stacks"].value<int>().value();
    spec.big_blind = toml["game_values"]["big_blind"].value<int>().value();
    spec.small_blind = toml["game_values"]["small_blind"].value<int>().value();

    spec.bet_sizes = {
        toml_float_vec(*toml["bet_sizes"]["preflop"].as_array()),
        toml_float_vec(*toml["bet_sizes"]["flop"].as_array()),
        toml_float_vec(*toml["bet_sizes"]["turn"].as_array()),
        toml_float_vec(*toml["bet_sizes"]["river"].as_array())
    };

    if (toml["load_isets"]["enabled"].value_or(false)) {
        spec.isets_paths = ISetsPaths{
            .regret_path = (root/toml["load_isets"]["regret"].value<std::string>().value()).string(),
            .strategy_path = (root/toml["load_isets"]["strategy"].value<std::string>().value()).string(),
            .offset_path = (root/toml["load_isets"]["offset"].value<std::string>().value()).string(),
            .iters_path = (root/toml["load_isets"]["iters"].value<std::string>().value()).string()
        };
    }

    return spec;
}

std::pair<TrainParams, TrainLog> load_run_config(const std::filesystem::path& run_toml_path, 
    const std::filesystem::path& root) {
    toml::table toml = toml::parse_file(run_toml_path.string());
    TrainLog log;
    TrainParams train;

    train.train_iters = toml["train"]["train_iters"].value<size_t>().value();
    train.iters_per_discount = toml["train"]["iters_per_discount"].value<size_t>().value();
    train.num_threads = toml["train"]["num_threads"].value<size_t>().value();
    train.omp_chunk_sz = toml["train"]["omp_chunk_sz"].value<size_t>().value();
    train.base_seed= toml["train"]["base_seed"].value<uint32_t>().value();

    if (toml["save_isets"]["enabled"].value_or(false)) {
        log.isets_paths = ISetsPaths{
            .regret_path = (root/toml["save_isets"]["regret"].value<std::string>().value()).string(),
            .strategy_path = (root/toml["save_isets"]["strategy"].value<std::string>().value()).string(),
            .offset_path = (root/toml["save_isets"]["offset"].value<std::string>().value()).string(),
            .iters_path = (root/toml["save_isets"]["iters"].value<std::string>().value()).string()
        };
    }
    log.overwrite_isets = toml["save_isets"]["overwrite"].value_or(false);

    if (toml["save_preflop"]["enabled"].value_or(false)) {
        log.preflop_path = root / toml["save_preflop"]["path"].value<std::string>().value();
    }
    log.overwrite_preflop = toml["save_preflop"]["overwrite"].value_or(false);

    return {train, log};
}