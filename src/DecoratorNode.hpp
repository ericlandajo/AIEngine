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