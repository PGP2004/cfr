#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <chrono>

#include "KuhnPoker/KuhnGameState.h"
#include "KuhnPoker/KuhnInfoSet.h"

#include "RockPaperScissors/RPSGameState.h"
#include "RockPaperScissors/RPSInfoSet.h"

#include "CFR/ExternalSamplingMCCFR.h"



using namespace std;

template <typename InfoSetT>
static void print_infosets(const ExternalSamplingMCCFR<InfoSetT>& solver) {
    const auto& all = solver.view_infosets();

    cout << fixed << setprecision(4);

    for (int p = 0; p < 2; ++p) {
        cout << "=== Player " << p << " infosets ===\n";

        vector<string> ids;
        ids.reserve(all[p].size());
        for (const auto& kv : all[p]) ids.push_back(kv.first);
        sort(ids.begin(), ids.end());

        for (const string& infoset_id : ids) {
            const InfoSetT& iset = all[p].at(infoset_id);

            unordered_map<string, double> strat = iset.get_average_strategy();

            cout << infoset_id << "  ";

            vector<pair<string,double>> items(strat.begin(), strat.end());
            sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b){ return a.first < b.first; });

            bool first = true;
            for (const auto& [action, prob] : items) {
                if (!first) cout << ", ";
                first = false;
                cout << action << ":" << prob;
            }
            cout << "\n";
        }

        cout << "\n";
    }
}

static void run_kuhn() {
    unique_ptr<GameState> init_state = make_unique<KuhnGameState>();
    ExternalSamplingMCCFR<KuhnInfoSet> solver(12345u, std::move(init_state));

    int num_epochs = 5;
    int iters_per_epoch = 50000;

    for (int epoch = 0; epoch < num_epochs; ++epoch) {
        solver.train(iters_per_epoch);

        if (epoch % 10 == 0) {
            // cout << "Epoch " << epoch << "\n";
            // print_infosets(solver);
        }
    }

    cout << "STRATEGY AT THE END:\n";
    print_infosets(solver);
}

static void run_rps() {
    unique_ptr<GameState> init_state = make_unique<RPSGameState>();
    ExternalSamplingMCCFR<RPSInfoSet> solver(12345u, std::move(init_state));

    int num_epochs = 1;
    int iters_per_epoch = 50000;

    for (int epoch = 0; epoch < num_epochs; ++epoch) {
        solver.train(iters_per_epoch);

        // if (epoch % 10 == 0) {
        //     cout << "Epoch " << epoch << "\n";
        //     print_infosets(solver);
        // }
    }

    cout << "STRATEGY AT THE END:\n";
    print_infosets(solver);
}

int main() {
    try {
        using clock = std::chrono::steady_clock;

        auto t0 = clock::now();
        cout << "start timeing:" << endl;
            
        // run_rps();
        run_kuhn();

        auto t1 = clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();
        cout << fixed << setprecision(6) << "Elapsed: " << seconds << " s\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}