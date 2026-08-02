#!/bin/bash
set -euo pipefail

./build/bin/make_river_strengths
./build/bin/make_turn_cdfs
./build/bin/make_turn_clusters
./build/bin/make_flop_pdfs
./build/bin/make_flop_clusters
./build/bin/make_flop_ev_sdev

echo "All done!"