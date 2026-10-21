#pragma once
// =====================================================================
//  Node fals que retorna una seqüència d'estats programada i compta
//  quantes vegades l'han avaluat. Permet provar decoradors i nodes
//  compostos de manera aïllada, sense dependre del comportament d'un
//  node real ni de cap fitxer .bin.
//
//  Els comptadors són `mutable` perquè evaluate() és const a la
//  interfície de aitoolkit.
// =====================================================================

#include <vector>
#include <memory>

#include <aitoolkit/behtree.hpp>
#include "AIEngine.hpp"   // Blackboard

class MockNode final : public aitoolkit::bt::node<Blackboard> {
public:
    using State = aitoolkit::bt::execution_state;

    // Retorna sempre el mateix estat.
    explicit MockNode(State state = State::success)
        : m_states{ state } {}

    // Retorna una seqüència: 1a avaluació states[0], 2a states[1]...
    // Quan s'exhaureix, es repeteix l'últim valor.
    explicit MockNode(std::vector<State> states)
        : m_states(std::move(states)) {
        if (m_states.empty()) m_states.push_back(State::success);
    }

    State evaluate(Blackboard& bb) const override {
        m_evalCount++;
        m_lastDeltaTime = bb.deltaTime;

        State state = m_states[m_index];
        if (m_index + 1 < m_states.size()) m_index++;
        return state;
    }

    // -----------------------------------------------------------------
    //  Consultes (per a les assercions)
    // -----------------------------------------------------------------
    size_t EvalCount() const { return m_evalCount; }
    bool WasEvaluated() const { return m_evalCount > 0; }
    float LastDeltaTime() const { return m_lastDeltaTime; }

    void Reset() const {
        m_evalCount = 0;
        m_index = 0;
    }

private:
    std::vector<State> m_states;
    mutable size_t m_index = 0;
    mutable size_t m_evalCount = 0;
    mutable float m_lastDeltaTime = 0.0f;
};


// =====================================================================
//  Helpers
//
//  Els decoradors i nodes compostos prenen els fills per unique_ptr,
//  de manera que en perds la propietat en construir-los. Aquests
//  helpers retornen el unique_ptr per moure'l i, alhora, deixen un
//  punter cru per poder inspeccionar el mock després.
//
//  Ús típic:
//      MockNode* child = nullptr;
//      auto decorator = InverterNode<Blackboard>(MakeMock(State::success, &child));
//      ...
//      CHECK(child->EvalCount() == 1);
// =====================================================================

inline std::unique_ptr<aitoolkit::bt::node<Blackboard>>
MakeMock(aitoolkit::bt::execution_state state, MockNode** out = nullptr) {
    auto node = std::make_unique<MockNode>(state);
    if (out) *out = node.get();
    return node;
}

inline std::unique_ptr<aitoolkit::bt::node<Blackboard>>
MakeMock(std::vector<aitoolkit::bt::execution_state> states, MockNode** out = nullptr) {
    auto node = std::make_unique<MockNode>(std::move(states));
    if (out) *out = node.get();
    return node;
}

// Construeix el vector de fills per a un node compost (seq, sel,
// ParallelNode) a partir d'una llista de unique_ptr ja creats:
//      auto children = MakeChildren(MakeMock(success), MakeMock(failure));
template <typename... Nodes>
std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>>
MakeChildren(Nodes&&... nodes) {
    std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> children;
    children.reserve(sizeof...(nodes));
    (children.push_back(std::forward<Nodes>(nodes)), ...);
    return children;
}


// =====================================================================
//  Blackboard llest per a proves, amb un deltaTime injectable.
//  El deltaTime controlat és el que permet provar CooldownNode i
//  TimeoutNode de forma determinista, sense esperes reals.
// =====================================================================
inline Blackboard MakeBlackboard(IActionHandler* actions = nullptr,
                                 IVariableUpdater* variables = nullptr,
                                 float deltaTime = 0.0f) {
    Blackboard bb;
    bb.actionHandler = actions;
    bb.variableUpdater = variables;
    bb.deltaTime = deltaTime;
    return bb;
}