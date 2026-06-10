#pragma once

#include <aitoolkit/behtree.hpp>
#include <vector>
#include <memory>

template <class T>
class ParallelNode final : public aitoolkit::bt::node<T> {
private:
    size_t m_success_threshold;
    size_t m_failure_threshold;

public:
    ParallelNode(size_t success_threshold, size_t failure_threshold, std::vector<std::unique_ptr<aitoolkit::bt::node<T>>> children)
        : m_success_threshold(success_threshold), m_failure_threshold(failure_threshold) {
        this->m_children = std::move(children);
    }

    virtual aitoolkit::bt::execution_state evaluate(T& blackboard) const override {
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

        size_t successes = 0;
        size_t failures = 0;

        for (auto& child : this->m_children) {
            if (!child) {
                failures++;
                continue; 
            }

            auto state = child->evaluate(blackboard);
            if (state == aitoolkit::bt::execution_state::success) {
                successes++;
            } else if (state == aitoolkit::bt::execution_state::failure) {
                failures++;
            }
        }

        if (failures >= m_failure_threshold) {
            return aitoolkit::bt::execution_state::failure;
        }
        if (successes >= m_success_threshold) {
            return aitoolkit::bt::execution_state::success;
        }

        return aitoolkit::bt::execution_state::running;
    }
};