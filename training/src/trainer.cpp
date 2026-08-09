
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

void write_preflop_csv(const std::string& path, const PreflopStrategy& preflop_strat) {

    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot open " + path);

    out << "hand";
    for (const Action& action : preflop_strat.actions){
        out << ',' << action.to_string();
    }

    out << '\n';
    out << std::fixed << std::setprecision(6);

    for (const auto& [hand, probs] : preflop_strat.probs) {
        out << hand;
        for (double p : probs) out << ',' << p;
        out << '\n';
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

    std::pair<fs::path, fs::path> new_paths = make_run_dirs(trainer);
    fs::path preflop_path = new_paths.first;
    fs::path infosets_path = new_paths.second;

    ActionTree at{trainer.game_state, trainer.bet_sizes};
    CardBuckets buckets(trainer.bp);
    CFR cfr{std::move(buckets), std::move(at)};

    int cur_iter = 0;

    for (int ckpt : trainer.preflop_ckpts) {
        if (ckpt > trainer.max_iters) break;

        cfr.train(ckpt - cur_iter, cur_iter);
        cur_iter = ckpt;

        std::string csv_name = "iter_"  + fmt_iters(cur_iter) + ".csv";
        std::string csv_path = (preflop_path / csv_name).string();
        
        write_preflop_csv(csv_path, cfr.get_preflop_strategy());
    }
}