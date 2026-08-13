#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <chrono>
#include <filesystem> 

#include "card_buckets.h"
#include "cfr.h"

#include "info_sets.h"
#include "action_tree.h"

struct LogParams {
    std::filesystem::path folder;     
    std::string run_name;            
    std::string preflop_subdir;     
    std::vector<int> preflop_ckpts;
    bool store_infosets;
};

struct CFRSpec{
    std::optional<ISetsPaths> isets_paths;
    BucketPaths bucket_paths;
    std::vector<std::vector<float>> bet_sizes;
};

CFR load_cfr(CFRSpec spec);
void write_preflop_csv(const std::string& path, const CFR& cfr);
void run_training(CFRSpec spec, const LogParams& log_params, int iters_per_discount);