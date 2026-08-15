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
    size_t omp_chunk_sz = 64 ;
    uint32_t base_seed = 0;
};

struct LogParams{
    std::optional<std::filesystem::path> preflop_path;
    std::optional<ISetsPaths> isets_paths;
    bool overwrite_isets = false; //gives permission to overwrite isets
    bool overwrite_preflop = false; //gives permission to overwrite preflops
};

struct CFRSpec{
    std::optional<ISetsPaths> isets_paths;
    BucketPaths bucket_paths;
    std::vector<std::vector<float>> bet_sizes;
};

CFR load_spec(CFRSpec spec);
void write_preflop_csv(const std::string& path, const CFR& cfr);
void run_training(const CFRSpec& spec, const TrainParams& tp, const LogParams& lp);

struct Config {
    TrainParams train;
    LogParams log;
    CFRSpec spec;
};

Config load_config(const std::filesystem::path& cfg_path, const std::filesystem::path& root);