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
#include "game_state.h"
#include "info_sets.h"
#include "action_tree.h"


struct Trainer{

    BucketingPaths bp;

    GameState game_state;
    std::vector<std::vector<float>> bet_sizes;

    std::filesystem::path runs_folder;
    std::filesystem::path run_name;

    std::vector<int> preflop_ckpts;
    int max_iters;
    bool store_infosets;
};

void run_training(const Trainer& trainer);



