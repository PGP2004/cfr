#include "L1_k_means.h"
#include <string>
#include <algorithm>
#include <random>
#include <iostream>
#include <climits>
#include <vector>
#include <span>

using namespace std;
namespace L1{

void update_assignments_and_counts(const ClusteringParams& params, ClusterBuffer& c_buff, const vector<int>& pts) {

    c_buff.assignments.assign(params.num_pts, 0);
    c_buff.counts.assign(params.num_clusters, 0);

    for (size_t pt_idx = 0; pt_idx < params.num_pts; ++pt_idx) {

        int best_dist = INT_MAX;
        int best_center = 0;

        for (size_t center_idx = 0; center_idx < params.num_clusters; ++center_idx) {

            span<const int>pt_span(&pts[pt_idx * params.dim], params.dim);
            span<const int>ctr_span(&c_buff.centers[center_idx * params.dim], params.dim);
            int d = L1_dist(pt_span, ctr_span);

            if (d < best_dist) { 
                best_dist = d; 
                best_center = static_cast<int>(center_idx); 
            }
        }
        c_buff.counts[best_center] += 1;
        c_buff.assignments[pt_idx] = best_center;
    }
}

void update_grouped(const ClusteringParams& params, ClusterBuffer& c_buff, const vector<int>& pts) {
    //Reshaping data into grouped makes it nice to use the L1 dist functions ince the data it wants to use is all contiguos in the array

    vector<size_t> offsets(params.num_clusters * params.dim, 0);
    size_t running = 0;

    for (size_t center_idx = 0; center_idx < params.num_clusters; ++center_idx) {
        for (size_t dim = 0; dim <params.dim; ++dim) {
            offsets[params.dim * center_idx + dim] = running;
            running += c_buff.counts[center_idx];
        }
    }

    c_buff.grouped.resize(params.num_pts*params.dim);   

    for (size_t pt_idx = 0; pt_idx < params.num_pts; ++pt_idx) {
        int assigned_center = c_buff.assignments[pt_idx];
        for (size_t dim_idx = 0; dim_idx < params.dim; ++dim_idx) {
            size_t gpd_idx = offsets[params.dim * assigned_center + dim_idx];
            c_buff.grouped[gpd_idx] = pts[pt_idx * params.dim + dim_idx];
            ++offsets[params.dim * assigned_center + dim_idx];
        }
    }
}

vector<bool> update_centers(const ClusteringParams& params, ClusterBuffer& c_buff) {
    //updates c_buff.centers using L1 centroid (this is the coordinate wise median)

    c_buff.centers.resize(params.num_clusters * params.dim);
    size_t running = 0;

    vector<bool> center_reseeded(params.num_clusters);
    for (size_t ctr = 0; ctr < params.num_clusters; ++ctr) {
        for (size_t dim = 0; dim < params.dim; ++dim) {
            size_t block_len = c_buff.counts[ctr];
            auto it = c_buff.grouped.begin() + running;
            running += block_len;      

            if (c_buff.counts[ctr] == 0){
                center_reseeded[ctr] = true;
                continue;
            } 

            nth_element(it, it + block_len / 2, it + block_len); 
            c_buff.centers[params.dim * ctr + dim] = *(it + block_len / 2);
        }
    }

    return center_reseeded;
}

void reinit_centers(const ClusteringParams& params, ClusterBuffer& c_buff, const vector<int>& pts, const vector<bool>& reinit) {

    uniform_int_distribution<size_t> pick(0, params.num_pts - 1);

    for (size_t c = 0; c < reinit.size(); ++c) {
        if (!reinit[c]) continue;
        size_t r = pick(params.rng);                      
        for (size_t j = 0; j < params.dim; ++j)
            c_buff.centers[c * params.dim + j] = pts[r * params.dim + j];
    }
}  

bool clustering_step(const ClusteringParams& params, ClusterBuffer& c_buff, const vector<int>& pts){

    c_buff.prev_assignments.swap(c_buff.assignments);              

    update_assignments_and_counts(params, c_buff ,pts); 
    update_grouped(params, c_buff, pts);
    vector<bool> reinit= update_centers(params, c_buff);

    if (find(reinit.begin(), reinit.end(), true) != reinit.end()){
        reinit_centers(params, c_buff, pts, reinit);
    }
    return c_buff.assignments != c_buff.prev_assignments;
}

void init_centers(const ClusteringParams& params, ClusterBuffer& c_buff, const vector<int>&pts){
    //heuristic initialization of c_buff.centers with distance caching
    c_buff.centers.resize(params.num_clusters*params.dim);

    uniform_int_distribution<size_t> upto(0, params.num_pts -1 );
    size_t first_center = upto(params.rng);

    for (size_t i = 0; i < params.dim; ++i){
        c_buff.centers[i] = pts[first_center*params.dim + i];
    }
    vector<int> min_d_cache(params.num_pts, INT_MAX); 
    uint64_t total = 0;

    for (size_t c = 1; c < params.num_clusters; ++c){
        //c is the center we are trying to sample
        total = 0;
        for(size_t p = 0; p < params.num_pts; ++p){
            //p is the pt idx

            span<const int>pt_span(&pts[p * params.dim], params.dim);
            span<const int>ctr_span(&c_buff.centers[(c-1) * params.dim], params.dim);
            int dist = L1_dist(pt_span, ctr_span);
           
            if (dist < min_d_cache[p]) min_d_cache[p] = dist;
            total += min_d_cache[p];
        }

        uniform_int_distribution<uint64_t> upto(0, total);
        uint64_t target = upto(params.rng);
        size_t chosen = 0;

        uint64_t cum_sum = 0;
        for (size_t p = 0; p < params.num_pts; ++p){
            cum_sum += min_d_cache[p];
            if(cum_sum >= target){
                chosen = p;
                break;
            }
        }

        for (size_t i = 0; i < params.dim; ++i){
            c_buff.centers[c*params.dim+i] = pts[chosen*params.dim+i];
        }
    }
}

pair<vector<int>,vector<int>> l1_k_means(const ClusteringParams& params, const vector<int>& pts){
    if (pts.size() != params.dim* params.num_pts) throw runtime_error("pt size doesnt match param specs");

    ClusterBuffer c_buff;
    c_buff.assignments.resize(params.num_pts);
    c_buff.prev_assignments.assign(params.num_pts, -1);
    c_buff.grouped.resize(params.num_pts*params.dim);
    c_buff.counts.resize(params.num_clusters);
    c_buff.centers.resize(params.num_clusters);

    init_centers(params, c_buff, pts);

    for (size_t iter = 0; iter < params.max_iters; ++iter) {
        bool changed = clustering_step(params, c_buff,  pts);
        if (!changed) break;    
    }

    return {std::move(c_buff.assignments), std::move(c_buff.centers),};
}
}