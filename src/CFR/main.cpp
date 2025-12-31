#include <algorithm>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "BruteAbstraction.h"
#include "CFR/ExternalSamplingMCCFR.h"

#include "KuhnPoker/KuhnGameState.h"
#include "KuhnPoker/KuhnInfoSet.h"

#include "RockPaperScissors/RPSGameState.h"
#include "RockPaperScissors/RPSInfoSet.h"

#include "TexasHoldEm/HoldEmGameState.h"
#include "TexasHoldEm/HoldEmInfoSet.h"

using namespace std;

template <typename InfoSetT, typename AbstractionT>
static void print_infosets(const ExternalSamplingMCCFR<InfoSetT, AbstractionT>& solver) {
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

template <class GameStateT, class InfoSetT, class AbstractionT>
static void run_game(int epochs,
                     int iters_per_epoch) {

    bool print_each_epoch = true;
    int print_every = 10;

    unique_ptr<GameState> init_state = make_unique<GameStateT>();
    ExternalSamplingMCCFR<InfoSetT, AbstractionT> solver(12345u, std::move(init_state));

    for (int epoch = 0; epoch < epochs; ++epoch) {
        solver.train(iters_per_epoch);

        // if (print_each_epoch && (epoch % print_every == 0)) {
        //     cout << "Epoch " << epoch << "\n";
        //     print_infosets(solver);
        // }
    }

    // cout << " STRATEGY AT THE END:\n";
    // print_infosets(solver);
}

int main() {
    try {
        using clock = std::chrono::steady_clock;

        int epochs = 1;
        int iters_per_epoch = 5000;

        auto t0 = clock::now();
        run_game<HoldEmGameState, HoldEmInfoSet, BruteAbstraction>(epochs, iters_per_epoch);
        auto t1 = clock::now();

        double seconds = std::chrono::duration<double>(t1 - t0).count();
        cout << fixed << setprecision(6) << "Elapsed: " << seconds << "\n";

    } 
        catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
