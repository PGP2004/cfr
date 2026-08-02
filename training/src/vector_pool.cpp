#include "vector_pool.h"
std::stack<std::vector<double>*> VectorPool::delta_pool;
std::stack<std::vector<double>*> VectorPool::probs_pool;
