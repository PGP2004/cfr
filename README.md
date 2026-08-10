Work in Progress implementation of External Sampling CFR.

The code is split into four sections.

Training - This contains the ES CFR implementation, as well as a GameState class implementing heads up poker.
It also contains a InfoSet class used in the ES CFR algorithm.

Clustering - This is a ton of scripts used to cluster the river, turn and flop. I used the potential aware clustering scheme 
described in this [paper](https://www.cs.cmu.edu/~sandholm/potential-aware_imperfect-recall.aaai14.pdf). This sections needs to be cleaned up.

External - Used to map hands to hand-isomorphism classes. 

Uses this [implementation](https://github.com/kdub0/hand-isomorphism) of the algorithm described in this [paper](https://www.cs.cmu.edu/~kwaugh/publications/isomorphism13.pdf)

Analysis - Used to visualize my clusters. I hope to add a way to visualize the training run convergence by looking at the preflop ranges.
