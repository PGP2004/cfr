#pragma once
#include <vector>
#include <stack>
#include <stdexcept>


class VectorPool {
private:
    static std::stack<std::vector<double>*> delta_pool;
    static std::stack<std::vector<double>*> probs_pool;

public:

    struct DeltaBuffer {
        std::vector<double>* buf;
        DeltaBuffer() {
            if (VectorPool::delta_pool.empty()) throw std::logic_error("ran outa deltas");
            else {
                buf = VectorPool::delta_pool.top();
                VectorPool::delta_pool.pop();
            }
            buf->clear();
        }
        
        ~DeltaBuffer() {
            if (buf) VectorPool::delta_pool.push(buf);
        }

        DeltaBuffer(const DeltaBuffer&) = delete;
        DeltaBuffer& operator=(const DeltaBuffer&) = delete;
        std::vector<double>& get() {return *buf; }
    };

    struct ProbsBuffer {
        std::vector<double>* buf;
        ProbsBuffer() {
            if (VectorPool::probs_pool.empty()) throw std::logic_error("ran outa probs");
            else {
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
        std::vector<double>& get() { return *buf; }
    };


      static void preallocate(size_t vector_size, size_t count = 100) {
        for (size_t i = 0; i < count; i++) {
           
            auto* delta_buf = new std::vector<double>();
            delta_buf->reserve(vector_size);
            delta_pool.push(delta_buf);

            auto* probs_buf = new std::vector<double>();
            probs_buf->reserve(vector_size);
            probs_pool.push(probs_buf);
        }
    }

};