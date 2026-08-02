    #include "emd_k_means.h"
    #include <string>
    #include <algorithm>
    #include <random>
    #include <limits>
    #include <vector>
    #include <span>

    using namespace std;

namespace emd{

    void fill_emd_cache(const Params& params, const Center& ctr, EMDCache& emd_cache){

        vector<size_t> idx_scratch(params.center_support);
        vector<float> dist_scratch(params.center_support);

        emd_cache.ordered_clusters.resize(params.num_verts * params.center_support);
        emd_cache.sorted_distances.resize(params.num_verts * params.center_support);

        for (size_t i = 0; i < params.num_verts; ++i){
            for (size_t j = 0; j < params.center_support; ++j){

                size_t atom = static_cast<size_t>(ctr.verts[j]);
                dist_scratch[j] = params.weight_matrix[i*params.num_verts+atom];
                idx_scratch[j] = j;
            }

            sort(idx_scratch.begin(), idx_scratch.end(), 
                [&](size_t a, size_t b){return dist_scratch[a] < dist_scratch[b];});
            
            for (size_t j = 0; j < params.center_support; ++j){

                emd_cache.ordered_clusters[i*params.center_support + j] = idx_scratch[j];
                emd_cache.sorted_distances[i*params.center_support + j] = dist_scratch[idx_scratch[j]];               
            }
        }
    }


    float approx_EMD(const Params& params, const Center& ctr, span<const int> multiset,  const EMDCache& emd_cache, EMDScratch& emd_scratch) {

        if (multiset.size() != params.multiset_size) throw runtime_error("Something got cooked");

        float temp = 1.0f / static_cast<float>(multiset.size());
        emd_scratch.targets.assign(multiset.size(), temp);
        emd_scratch.done.assign(multiset.size(), false);
        emd_scratch.mean_remaining.assign(ctr.wts.begin(), ctr.wts.end());

        float total_cost = 0;

        for (size_t i = 0; i < params.center_support; ++i) {

            for (size_t j = 0; j < params.multiset_size; ++j) {

                if (emd_scratch.done[j]) continue;

                int ground_cluster = multiset[j];
                size_t oc_idx = ground_cluster * params.center_support + i;
                int mean_cluster = emd_cache.ordered_clusters[oc_idx];

                float amt_rem = emd_scratch.mean_remaining[mean_cluster];
                if (amt_rem == 0) continue;

                float d = emd_cache.sorted_distances[oc_idx];

                if (amt_rem < emd_scratch.targets[j]) {
                    total_cost += d * amt_rem;
                    emd_scratch.targets[j] -= amt_rem;
                    emd_scratch.mean_remaining[mean_cluster] = 0;
                } 

                else {
                    total_cost += emd_scratch.targets[j] * d;
                    emd_scratch.mean_remaining[mean_cluster] -= emd_scratch.targets[j];
                    emd_scratch.targets[j] = 0;
                    emd_scratch.done[j] = true;
                }
            }
        }
        return total_cost;
    }

    void update_assignments_and_counts(const Params& params, ClusterBuffer& c_buff, 
        const vector<int>& multisets, EMDCache& emd_cache) {

        c_buff.assignments.assign(params.num_multisets, 0);
        c_buff.min_dists.assign(params.num_multisets, numeric_limits<float>::max());

        for (size_t ctr = 0; ctr <c_buff.centers.size(); ++ctr) {
            fill_emd_cache(params,c_buff.centers[ctr], emd_cache);   

            #pragma omp parallel
            {
                EMDScratch local_scratch;              
                #pragma omp for schedule(static)
                for (size_t multiset = 0; multiset < params.num_multisets; ++multiset) {
                    span<const int> multiset_span(&multisets[multiset * params.multiset_size], params.multiset_size);
                    float dist = approx_EMD(params, c_buff.centers[ctr], multiset_span, emd_cache, local_scratch);
                    if (dist < c_buff.min_dists[multiset]) {
                        c_buff.min_dists[multiset] = dist;
                        c_buff.assignments[multiset] = static_cast<int>(ctr);
                    }
                }
            }
        }

        c_buff.counts.assign(params.num_clusters, 0);
        for (size_t multiset = 0; multiset < params.num_multisets; ++multiset)
            c_buff.counts[c_buff.assignments[multiset]] += 1;
    }

    void update_grouped(const Params& params, ClusterBuffer& c_buff, const vector<int>& multisets) {
        // groups multisets by cluster assignment into contiguous blocks in c_buff.grouped

        vector<size_t> offsets(params.num_clusters, 0);

        for (size_t ctr = 1; ctr < params.num_clusters; ++ctr) {
            offsets[ctr] = offsets[ctr-1] + c_buff.counts[ctr-1] * params.multiset_size;
        }

        c_buff.grouped.resize(params.num_multisets * params.multiset_size);

        for (size_t multiset = 0; multiset < params.num_multisets; ++multiset) {
            int ass_ctr = c_buff.assignments[multiset];
            size_t gpd_idx = offsets[ass_ctr];
            size_t multisets_idx = multiset * params.multiset_size;
            for (size_t i = 0; i < params.multiset_size; ++i) {
                c_buff.grouped[gpd_idx + i] = multisets[multisets_idx + i];
            }
            offsets[ass_ctr] += params.multiset_size;
        }   
    }

    void clipped_dense_center(const Params& params, Center& ctr, const vector<int>& dense_rep){

        ctr.wts.resize(params.center_support);
        ctr.verts.resize(params.center_support);

        vector<size_t> idx(dense_rep.size());
        for (size_t i = 0; i < dense_rep.size(); ++i) idx[i] = i;
        nth_element(idx.begin(), idx.begin() + params.center_support, idx.end(),
                    [&](size_t a, size_t b){ return dense_rep[a] > dense_rep[b]; });

        float cum_sum = 0;
        for (size_t i = 0; i < params.center_support; ++i){
            ctr.verts[i] = idx[i];
            cum_sum += static_cast<float>(dense_rep[idx[i]]);
        }

        for (size_t i = 0; i < params.center_support; ++i){
            size_t sr_idx = idx[i];
            ctr.wts[i] = static_cast<float>(dense_rep[sr_idx])/cum_sum;
        }

    }

    vector<bool> update_centers(const Params& params, ClusterBuffer& c_buff) {
        

        vector<bool> reinit(c_buff.counts.size());
        size_t running = 0;

        vector<int> dense_rep(params.num_verts);

        for (size_t ctr = 0; ctr <c_buff.centers.size(); ++ctr){

            dense_rep.assign(params.num_verts, 0);

            if (c_buff.counts[ctr] == 0){
                reinit[ctr] = true;
                continue;
            } 

            size_t end = running + c_buff.counts[ctr]*params.multiset_size;
            for (size_t i = running; i < end; ++i){
                dense_rep[c_buff.grouped[i]] += 1;
            }

            running = end;
            clipped_dense_center(params, c_buff.centers[ctr], dense_rep);
        }
        return reinit;
    }


    void reinit_centers(const Params& params, ClusterBuffer& c_buff,
        const vector<int>& multisets, const vector<bool>& reinit) {

        //fully randomized reinitialize. Should prolly do better at some point

        size_t num_multisets = multisets.size() / params.multiset_size;  
        uniform_int_distribution<size_t> pick(0, num_multisets - 1);

        for (size_t ctr = 0; ctr < params.num_clusters; ++ctr) { 

            if (!reinit[ctr]) continue;

           c_buff.centers[ctr].verts.assign(params.center_support, 0);
           c_buff.centers[ctr].wts.assign(params.center_support, 0.0);

            size_t multiset = pick(params.rng);                      
            for (size_t j = 0; j < params.multiset_size; ++j){
               c_buff.centers[ctr].verts[j] = multisets[multiset * params.multiset_size + j];
               c_buff.centers[ctr].verts[j] = 1.0/static_cast<float>(params.multiset_size);
            }
        }
    }  

    void init_centers(const Params& params, ClusterBuffer& c_buff,
        const vector<int>&multisets, EMDCache& emd_cache){
        //heuristic initialization ofc_buff.centers 
        
        c_buff.centers.resize(params.num_clusters);

        uniform_int_distribution<size_t> upto(0, params.num_multisets -1 );
        size_t first_center = upto(params.rng);


        for (size_t i = 0; i < params.num_clusters; ++i){
           c_buff.centers[i].verts.assign(params.center_support, 0);
           c_buff.centers[i].wts.assign(params.center_support, 0.0);
        }

        for (size_t j = 0; j < params.multiset_size; ++j){
           c_buff.centers[0].verts[j] = multisets[first_center * params.multiset_size + j];
           c_buff.centers[0].wts[j] = 1.0/static_cast<float>(params.multiset_size);
        }

        c_buff.min_dists.assign(params.num_multisets, numeric_limits<float>::max());

        //min_dist here actually holds distance squared. kinda funky but nicer this way
        for (size_t ctr = 1; ctr < params.num_clusters; ++ctr){
            float total = 0.0;

            fill_emd_cache(params,c_buff.centers[ctr-1], emd_cache);
            #pragma omp parallel
            {
                EMDScratch local_scratch;                     
                #pragma omp for reduction(+:total) schedule(static)
                for (size_t multiset = 0; multiset < params.num_multisets; ++multiset) {
                    span<const int> multiset_span(&multisets[multiset * params.multiset_size], params.multiset_size);
                    float dist = approx_EMD(params, c_buff.centers[ctr-1], multiset_span, emd_cache, local_scratch);
                    dist = dist * dist;
                    if (dist < c_buff.min_dists[multiset]) c_buff.min_dists[multiset] = dist;
                    total += c_buff.min_dists[multiset];
                }
            }

            uniform_real_distribution<float> pick_dist(0.0f, total);
            float target = pick_dist(params.rng);

            size_t chosen = 0;

            float cum_sum = 0;
            for (size_t multiset = 0; multiset < params.num_multisets; ++multiset){
                cum_sum += c_buff.min_dists[multiset];
                if(cum_sum >= target){
                    chosen = multiset;
                    break;
                }
            }

            for (size_t i = 0; i < params.multiset_size; ++i){
               c_buff.centers[ctr].verts[i] = multisets[chosen * params.multiset_size + i];
               c_buff.centers[ctr].wts[i] = 1.0/static_cast<float>(params.multiset_size);
            }
        }

    }


    bool clustering_step(const Params& params, ClusterBuffer& c_buff, 
        const vector<int>& multisets,  EMDCache& emd_cache) {

        c_buff.prev_assignments.swap(c_buff.assignments);   
        
        update_assignments_and_counts(params, c_buff, multisets, emd_cache); 
        update_grouped(params, c_buff, multisets);
        vector<bool> reinit = update_centers(params, c_buff);
       
        if (find(reinit.begin(), reinit.end(), true) != reinit.end()){
            reinit_centers(params, c_buff, multisets, reinit);
        }

        return c_buff.assignments != c_buff.prev_assignments;
    }

    pair<vector<int>, vector<Center>> emd_k_means(const Params& params, const vector<int>& multisets) {
    

        if (multisets.size() != params.multiset_size*params.num_multisets){
            throw runtime_error("multiset size does not match params");
        }

        ClusterBuffer c_buff;
        c_buff.assignments.assign(params.num_multisets, 0);
        c_buff.prev_assignments.assign(params.num_multisets, 0);
        c_buff.min_dists.assign(params.num_multisets, 0.0);
        c_buff.grouped.assign(params.num_multisets*params.multiset_size, 0);
        c_buff.counts.assign(params.num_clusters, 0);

        EMDCache emd_cache;
        EMDScratch emd_scratch;

        init_centers(params, c_buff, multisets,emd_cache);

        for (size_t iter = 0; iter < params.max_iters; ++iter) {
            bool changed = clustering_step(params, c_buff, multisets, emd_cache);   
            if (!changed) break;    
        }

        return {std::move(c_buff.assignments), std::move(c_buff.centers)};
    }
}
