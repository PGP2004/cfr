#pragma once
#include <vector>
#include <stack>
using std::vector;
using std::pair;
using std::stack;


// Object pool for vector buffers to avoid allocations in hot path
class VectorPool {
private:

    static stack<vector<double>*> delta_pool;
    static stack<vector<double>*> probs_pool;

public:
   
    struct DeltaBuffer {
        vector<double>* buf;
        DeltaBuffer() {
            if (VectorPool::delta_pool.empty()) {
                throw std::logic_error("ran outa deltas");
            } else {
                buf = VectorPool::delta_pool.top();
                VectorPool::delta_pool.pop();
            }
            buf->clear();
        }
        
        ~DeltaBuffer() {
            if (buf) {
                VectorPool::delta_pool.push(buf);
            }
        }

        DeltaBuffer(const DeltaBuffer&) = delete;
        DeltaBuffer& operator=(const DeltaBuffer&) = delete;
        vector<double>& get() {return *buf; }
    };

    struct ProbsBuffer {
        vector<double>* buf;
        ProbsBuffer() {
            if (VectorPool::probs_pool.empty()) {
                throw std::logic_error("ran outa probs");
            } else {
                buf = VectorPool::probs_pool.top();
                VectorPool::probs_pool.pop();
            }
            buf->clear();
        }
        ~ProbsBuffer() {
            if (buf) {
                VectorPool::probs_pool.push(buf);
            }
        }

        ProbsBuffer(const ProbsBuffer&) = delete;
        ProbsBuffer& operator=(const ProbsBuffer&) = delete;
        vector<double>& get() { return *buf; }
    };


      static void preallocate(size_t vector_size, size_t count = 100) {
        for (size_t i = 0; i < count; i++) {
           
            auto* delta_buf = new vector<double>();
            delta_buf->reserve(vector_size);
            delta_pool.push(delta_buf);

            auto* probs_buf = new vector<double>();
            probs_buf->reserve(vector_size);
            probs_pool.push(probs_buf);
        }
    }

};