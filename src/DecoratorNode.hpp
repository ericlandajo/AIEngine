#pragma once

#include <aitoolkit/behtree.hpp>
#include <memory>

template <class T>
class ForceSuccessNode final : public aitoolkit::bt::node<T> {
public:
    ForceSuccessNode(std::unique_ptr<aitoolkit::bt::node<T>> child) {
        this->m_children.push_back(std::move(child));
    }
    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        auto state = this->m_children.front()->evaluate(bb);
        if (state == aitoolkit::bt::execution_state::running) 
            return aitoolkit::bt::execution_state::running;
        return aitoolkit::bt::execution_state::success;
    }
};

template <class T>
class ForceFailureNode final : public aitoolkit::bt::node<T> {
public:
    ForceFailureNode(std::unique_ptr<aitoolkit::bt::node<T>> child) {
        this->m_children.push_back(std::move(child));
    }
    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        auto state = this->m_children.front()->evaluate(bb);
        if (state == aitoolkit::bt::execution_state::running) 
            return aitoolkit::bt::execution_state::running;
        return aitoolkit::bt::execution_state::failure;
    }
};

template <class T>
class SuccessIsFailureNode final : public aitoolkit::bt::node<T> {
public:
    SuccessIsFailureNode(std::unique_ptr<aitoolkit::bt::node<T>> child) {
        this->m_children.push_back(std::move(child));
    }
    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        auto state = this->m_children.front()->evaluate(bb);
        if (state == aitoolkit::bt::execution_state::success) 
            return aitoolkit::bt::execution_state::failure;
        return state;
    }
};

template <class T>
class RepeaterNode final : public aitoolkit::bt::node<T> {
private:
    int m_max_iterations;
    mutable int m_current_iteration = 0;
public:
    RepeaterNode(int max_iterations, std::unique_ptr<aitoolkit::bt::node<T>> child) 
        : m_max_iterations(max_iterations) {
        this->m_children.push_back(std::move(child));
    }
    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        if (m_current_iteration >= m_max_iterations) {
            m_current_iteration = 0; 
            return aitoolkit::bt::execution_state::success; 
        }
        
        auto state = this->m_children.front()->evaluate(bb);
        
        if (state != aitoolkit::bt::execution_state::running) {
            m_current_iteration++;
            if (m_current_iteration >= m_max_iterations) {
                m_current_iteration = 0;
                return aitoolkit::bt::execution_state::success;
            }
        }
        return aitoolkit::bt::execution_state::running; 
    }
};

template <class T>
class RetryUntilSuccessNode final : public aitoolkit::bt::node<T> {
public:
    RetryUntilSuccessNode(std::unique_ptr<aitoolkit::bt::node<T>> child) {
        this->m_children.push_back(std::move(child));
    }
    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        auto state = this->m_children.front()->evaluate(bb);
        if (state == aitoolkit::bt::execution_state::success) {
            return aitoolkit::bt::execution_state::success;
        }
        return aitoolkit::bt::execution_state::running;
    }
};

template <class T>
class InverterNode final : public aitoolkit::bt::node<T> {
public:
    InverterNode(std::unique_ptr<aitoolkit::bt::node<T>> child) {
        this->m_children.push_back(std::move(child));
    }
    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        auto state = this->m_children.front()->evaluate(bb);
        
        if (state == aitoolkit::bt::execution_state::success) {
            return aitoolkit::bt::execution_state::failure;
        } else if (state == aitoolkit::bt::execution_state::failure) {
            return aitoolkit::bt::execution_state::success;
        }
        
        return state;
    }
};