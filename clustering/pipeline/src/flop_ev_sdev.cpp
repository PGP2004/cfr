#include "pipeline.h"
#include "matrix_loader.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

void cdfs_to_pdfs(size_t num_centers, size_t num_buckets, std::vector<int>& cdfs) {
    //write over the cdfs to get pdfs;
    for (size_t c = 0; c < num_centers; ++c) {
        for (size_t i = num_buckets - 1; i >= 1; --i) {
            size_t idx = num_buckets * c + i;
            cdfs[idx] = cdfs[idx] - cdfs[idx - 1];
        }
    }
}

std::pair<int, int> get_ev_and_sdev(size_t num_buckets, std::vector<int>& multiset,
                               const std::vector<int>& centers, std::vector<float>& buff) {

    buff.resize(num_buckets);
    for (size_t i = 0; i < num_buckets; ++i) buff[i] = 0.0;

    //num buckets is the number of buckets over which we chop up the [0,100] strength interval.
    //Bucket sizes are uniform.
    //the multiset is a sparse representation of a distribution over the centers
    //Aggregate the distribution over turns into just a distributino over strengths

    float total = 0.0;
    for (int c : multiset) {
        size_t ctr_idx = static_cast<size_t>(c) * num_buckets;
        for (size_t i = 0; i < num_buckets; ++i) {
            buff[i] += static_cast<float>(centers[ctr_idx + i]);
            total += static_cast<float>(centers[ctr_idx + i]);
        }
    }

    float ev = 0;
    float sdev = 0;
    //get the std_dev by first computing E(X^2) then subtracting E(X)^2 and taking sqrt root.

    for (size_t i = 0; i < num_buckets; ++i) {
        float prob = buff[i] / total;
        float wt = (100.0 / static_cast<float>(num_buckets)) * (i + 0.5);
        ev += wt * prob;
        sdev += wt * wt * prob;
    }

    sdev += -(ev * ev);
    sdev = sqrt(sdev);

    std::pair<int, int> output = {static_cast<int>(ev), static_cast<int>(sdev)};
    return output;
}

void run_flop_ev_sdev(const PipelineConfig& cfg) {
    const size_t num_centers = cfg.turn_clusters;
    const size_t num_buckets = cfg.turn_buckets;

    auto [centers, centers_header] = load_matrix_and_header<int>(cfg.art.turn_cdf_centers.string());
    if (centers_header.num_rows != num_centers || centers_header.num_cols != num_buckets)
        throw std::runtime_error("turn_cdf_centers shape does not match config: " + centers_header.to_string());

    cdfs_to_pdfs(num_centers, num_buckets, centers);

    auto [multisets, multisets_header] = load_matrix_and_header<int>(cfg.art.flop_multisets.string());
    size_t num_flops = static_cast<size_t>(multisets_header.num_rows);
    size_t multiset_size = static_cast<size_t>(multisets_header.num_cols);

    std::vector<int> multiset_buff(multiset_size, 0);
    std::vector<float> prob_buff(num_buckets, 0.0);
    std::vector<int> output(2 * num_flops, 0);

    for (size_t i = 0; i < num_flops; ++i) {
        size_t idx = multiset_size * i;
        for (size_t j = 0; j < multiset_size; ++j) {
            multiset_buff[j] = multisets[idx + j];
        }

        auto [ev, sdev] = get_ev_and_sdev(num_buckets, multiset_buff, centers, prob_buff);
        size_t o_idx = 2 * i;
        output[o_idx] = ev;
        output[o_idx + 1] = sdev;
    }

    MatrixHeader output_header{num_flops, 2, sizeof(int)};
    write_matrix_and_header<int>(cfg.art.flop_ev_sdev.string(), output_header, output);
}