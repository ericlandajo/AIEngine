#pragma once
// =====================================================================
//  Doble de prova d'IActionHandler. Permet:
//    - Programar què retorna cada acció (success / failure / running),
//      inclús una seqüència de resultats diferents per a crides
//      successives (útil per a accions de durada llarga).
//    - Registrar totes les crides rebudes, amb el seu nom i paràmetres,
//      per poder-hi fer assercions després.
// =====================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "IActionHandler.hpp"

class MockActionHandler : public IActionHandler {
public:
    using State = aitoolkit::bt::execution_state;

    // Registre d'una crida rebuda
    struct Call {
        std::string name;
        ActionParameters params;
    };

    // -----------------------------------------------------------------
    //  Configuració (abans d'executar)
    // -----------------------------------------------------------------

    // Resultat fix per a una acció concreta.
    void SetResult(const std::string& actionName, State state) {
        m_results[actionName] = { state };
        m_resultIndex[actionName] = 0;
    }

    // Seqüència de resultats: la 1a crida retorna states[0], la 2a
    // states[1], etc. Quan s'exhaureix, es repeteix l'últim valor.
    // Exemple: {running, running, success} per a una acció que triga
    // tres frames a completar-se.
    void SetResultSequence(const std::string& actionName,
                           std::vector<State> states) {
        m_results[actionName] = std::move(states);
        m_resultIndex[actionName] = 0;
    }

    // Resultat per a qualsevol acció no configurada explícitament.
    void SetDefaultResult(State state) { m_defaultResult = state; }

    // -----------------------------------------------------------------
    //  Interfície sota prova
    // -----------------------------------------------------------------

    State ExecuteAction(const std::string& actionName,
                        const ActionParameters& params) override {
        m_calls.push_back({ actionName, params });

        auto it = m_results.find(actionName);
        if (it == m_results.end() || it->second.empty()) {
            return m_defaultResult;
        }

        size_t& idx = m_resultIndex[actionName];
        State state = it->second[idx];
        if (idx + 1 < it->second.size()) idx++;   // l'últim valor es manté
        return state;
    }

    // -----------------------------------------------------------------
    //  Consultes (per a les assercions)
    // -----------------------------------------------------------------

    const std::vector<Call>& Calls() const { return m_calls; }

    size_t TotalCalls() const { return m_calls.size(); }

    bool WasCalled(const std::string& actionName) const {
        return CallCount(actionName) > 0;
    }

    size_t CallCount(const std::string& actionName) const {
        return std::count_if(m_calls.begin(), m_calls.end(),
            [&](const Call& c) { return c.name == actionName; });
    }

    // Nom de l'acció cridada en posició `index` (per verificar ordre).
    std::string CallAt(size_t index) const {
        return index < m_calls.size() ? m_calls[index].name : "";
    }

    // Seqüència completa de noms, útil per comparar d'un sol cop:
    //   CHECK(mock.CallSequence() == std::vector<std::string>{"A", "B"});
    std::vector<std::string> CallSequence() const {
        std::vector<std::string> names;
        names.reserve(m_calls.size());
        for (const auto& c : m_calls) names.push_back(c.name);
        return names;
    }

    // Paràmetres de l'última crida a una acció concreta.
    // Retorna un mapa buit si no s'ha cridat mai.
    ActionParameters LastParamsOf(const std::string& actionName) const {
        for (auto it = m_calls.rbegin(); it != m_calls.rend(); ++it) {
            if (it->name == actionName) return it->params;
        }
        return {};
    }

    void Reset() {
        m_calls.clear();
        m_resultIndex.clear();
        for (auto& kv : m_results) m_resultIndex[kv.first] = 0;
    }

private:
    std::vector<Call> m_calls;
    std::unordered_map<std::string, std::vector<State>> m_results;
    std::unordered_map<std::string, size_t> m_resultIndex;
    State m_defaultResult = State::success;
};