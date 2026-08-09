#pragma once
#include "matrix_loader.h"
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>

template <class T>
static size_t count_clusters(const std::vector<T>& assign) {
    if (assign.size() == 0) throw std::runtime_error("The assignment is empty. Cannot get max");
    return std::ranges::max(assign) + size_t{1};
}

struct BucketingPaths{
    std::filesystem::path flop_path;
    std::filesystem::path turn_path;
    std::filesystem::path river_path;
};

struct CardBuckets {
    std::vector<int> preflop_clusters;
    std::vector<int> flop_clusters;
    std::vector<int> turn_clusters;
    std::vector<int> river_clusters;
    std::vector<size_t> cluster_counts;

    CardBuckets() = default; 

    CardBuckets(const BucketingPaths& bp) {
        set_clusters(bp);
    }

    void set_clusters(const BucketingPaths& bp){

        int num_preflops = 169;
        preflop_clusters.resize(num_preflops);
    
        for (int i = 0; i < num_preflops; ++i){
            preflop_clusters[i] = static_cast<int>(i);
        }

        auto [fc, flop_header] = load_matrix_and_header<int>(bp.flop_path.string());
        flop_clusters = std::move(fc);

        auto[tc, turn_header] = load_matrix_and_header<int>(bp.turn_path.string());
        turn_clusters = std::move(tc);

        auto [rc, river_header] = load_matrix_and_header<int>(bp.river_path.string());
        river_clusters = std::move(rc);

        cluster_counts.clear();
        cluster_counts.push_back(preflop_clusters.size());   
        cluster_counts.push_back(count_clusters(flop_clusters));
        cluster_counts.push_back(count_clusters(turn_clusters));
        cluster_counts.push_back(count_clusters(river_clusters));
    }

    int cluster_of(int street, int hand_id) const {
        if (street == 0) return preflop_clusters[hand_id];
        if (street == 1) return flop_clusters[hand_id];
        if (street == 2) return turn_clusters[hand_id];
        if (street == 3) return river_clusters[hand_id];
        throw std::runtime_error("Street does not exist");
    }
};

