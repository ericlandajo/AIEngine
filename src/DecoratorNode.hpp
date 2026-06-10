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
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

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
    if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

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
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

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
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

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
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

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
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

        auto state = this->m_children.front()->evaluate(bb);
        
        if (state == aitoolkit::bt::execution_state::success) {
            return aitoolkit::bt::execution_state::failure;
        } else if (state == aitoolkit::bt::execution_state::failure) {
            return aitoolkit::bt::execution_state::success;
        }
        
        return state;
    }
};

template <class T>
class CooldownNode final : public aitoolkit::bt::node<T> {
public:
    CooldownNode(float cooldownTime, std::unique_ptr<aitoolkit::bt::node<T>> child)
        : m_cooldownTime(cooldownTime), m_elapsedTime(0.0f), m_onCooldown(false) {
        this->m_children.push_back(std::move(child));
    }

    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

        if (m_onCooldown) {
            m_elapsedTime += bb.deltaTime;
            
            if (m_elapsedTime >= m_cooldownTime) {
                m_onCooldown = false;
                m_elapsedTime = 0.0f;
            } else {
                return aitoolkit::bt::execution_state::failure;
            }
        }

        auto state = this->m_children.front()->evaluate(bb);

        if (state == aitoolkit::bt::execution_state::success) {
            m_onCooldown = true;
            m_elapsedTime = 0.0f;
        }

        return state;
    }

private:
    float m_cooldownTime;
    mutable float m_elapsedTime;
    mutable bool m_onCooldown;
};

template <class T>
class TimeoutNode final : public aitoolkit::bt::node<T> {
public:
    TimeoutNode(float timeout, std::unique_ptr<aitoolkit::bt::node<T>> child)
        : m_timeout(timeout), m_elapsedTime(0.0f) {
        this->m_children.push_back(std::move(child));
    }

    virtual aitoolkit::bt::execution_state evaluate(T& bb) const override {
        if (this->m_children.empty() || !this->m_children.front()) 
            return aitoolkit::bt::execution_state::failure;

        m_elapsedTime += bb.deltaTime;

        if (m_elapsedTime >= m_timeout) {
            m_elapsedTime = 0.0f; 
            return aitoolkit::bt::execution_state::failure;
        }

        auto state = this->m_children.front()->evaluate(bb);

        if (state != aitoolkit::bt::execution_state::running) {
            m_elapsedTime = 0.0f; 
        }

        return state;
    }

private:
    float m_timeout;
    mutable float m_elapsedTime;
};