#pragma once
#include <string>
#include <filesystem> 
#include <toml.hpp>

#include "card_buckets.h"
#include "cfr.h"
#include "info_sets.h"

struct TrainParams {            
    size_t train_iters;
    size_t iters_per_discount;
    size_t num_threads = 1;
    size_t omp_chunk_sz = 1;
    uint32_t base_seed = 0;
};

struct ReportParams{
    std::optional<std::filesystem::path> preflop_path;
    std::optional<ISetsPaths> isets_paths;
    bool overwrite_isets = false; //gives permission to overwrite isets
    bool overwrite_preflop = false; //gives permission to overwrite preflops
};

struct CFRSpec{
    std::optional<ISetsPaths> isets_paths;
    BucketPaths bucket_paths;
    std::vector<std::vector<float>> bet_sizes;
    int starting_stack;
    int big_blind;
    int small_blind;
};

CFR load_spec(CFRSpec spec);

void write_preflop_csv(const std::string& path, const CFR& cfr);

void generate_report(const ReportParams& report, const CFR& cfr);

void run_training(const CFRSpec& spec, const TrainParams& tp);

TrainParams load_train_config(const std::filesystem::path& run_toml_path);

ReportParams load_report_config(const std::filesystem::path& report_toml_path, const std::filesystem::path& root);

CFRSpec load_cfr_config(const std::filesystem::path& cfr_toml_path, const std::filesystem::path& root);
