#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>
#include "dataloader.h"
#include <algorithm>

//TODO: Add the clustering results to the infokey // and j rework this class hella

template <class T>
static size_t count_clusters(const std::vector<T>& assign) {
    if (assign.size() == 0) throw std::runtime_error("The assignment is empty. Cannot get max");
    return std::ranges::max(assign) + size_t{1};
}

struct Abstraction {
    
    std::vector<uint8_t> preflop_clusters;
    std::vector<uint16_t> flop_clusters;
    std::vector<uint16_t> turn_clusters;
    std::vector<uint8_t> river_clusters;
    std::vector<size_t> cluster_counts;

    Abstraction() = default; 

    Abstraction(const std::string& flop_path,
                const std::string& turn_path,
                const std::string& river_path) {
        set_clusters(flop_path, turn_path, river_path);
    }

    void set_clusters(const std::string& flop_path, const std::string&turn_path, const std::string& river_path){

        size_t num_preflops = 169;
        preflop_clusters.resize(num_preflops);
    
        for (size_t i = 0; i < num_preflops; ++i){
            preflop_clusters[i] = static_cast<int>(i);
        }

        auto [fc, flop_header] = load_matrix_and_header<uint16_t>(flop_path);

        flop_clusters = std::move(fc);

        auto[tc, turn_header] = load_matrix_and_header<uint16_t>(turn_path);
        turn_clusters = std::move(tc);

        auto [rc, river_header] = load_matrix_and_header<uint8_t>(river_path);
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

