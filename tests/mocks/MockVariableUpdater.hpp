#pragma once
// =====================================================================
//  Doble de prova d'IVariableUpdater. Permet:
//    - Programar el valor de cada variable del món del joc.
//    - Registrar quines variables s'han consultat i quantes vegades.
//
//  El registre de consultes és el que permet validar el requisit de
//  consulta sota demanda: es pot comprovar que el motor NO consulta
//  variables que no necessita en l'estat actual.
// =====================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "IVariableUpdater.hpp"

class MockVariableUpdater : public IVariableUpdater {
public:
    // -----------------------------------------------------------------
    //  Configuració (abans d'executar)
    // -----------------------------------------------------------------
    void SetBool(const std::string& name, bool value)          { m_bools[name] = value; }
    void SetFloat(const std::string& name, float value)        { m_floats[name] = value; }
    void SetInt(const std::string& name, int value)            { m_ints[name] = value; }
    void SetString(const std::string& name, std::string value) { m_strings[name] = std::move(value); }

    // -----------------------------------------------------------------
    //  Interfície sota prova
    //  Una variable no configurada retorna el valor per defecte del seu
    //  tipus, imitant un joc que encara no l'hagi establerta.
    // -----------------------------------------------------------------
    bool GetBoolVariable(const std::string& name) override {
        Record(name);
        auto it = m_bools.find(name);
        return it != m_bools.end() ? it->second : false;
    }

    float GetFloatVariable(const std::string& name) override {
        Record(name);
        auto it = m_floats.find(name);
        return it != m_floats.end() ? it->second : 0.0f;
    }

    int GetIntVariable(const std::string& name) override {
        Record(name);
        auto it = m_ints.find(name);
        return it != m_ints.end() ? it->second : 0;
    }

    std::string GetStringVariable(const std::string& name) override {
        Record(name);
        auto it = m_strings.find(name);
        return it != m_strings.end() ? it->second : std::string();
    }

    // -----------------------------------------------------------------
    //  Consultes (per a les assercions)
    // -----------------------------------------------------------------
    bool WasQueried(const std::string& name) const {
        return QueryCount(name) > 0;
    }

    size_t QueryCount(const std::string& name) const {
        auto it = m_queryCounts.find(name);
        return it != m_queryCounts.end() ? it->second : 0;
    }

    size_t TotalQueries() const { return m_queryOrder.size(); }

    // Ordre en què s'han consultat les variables (amb repeticions).
    const std::vector<std::string>& QueryOrder() const { return m_queryOrder; }

    void Reset() {
        m_queryCounts.clear();
        m_queryOrder.clear();
    }

    // Neteja també els valors configurats.
    void ResetAll() {
        Reset();
        m_bools.clear();
        m_floats.clear();
        m_ints.clear();
        m_strings.clear();
    }

private:
    void Record(const std::string& name) {
        m_queryCounts[name]++;
        m_queryOrder.push_back(name);
    }

    std::unordered_map<std::string, bool>        m_bools;
    std::unordered_map<std::string, float>       m_floats;
    std::unordered_map<std::string, int>         m_ints;
    std::unordered_map<std::string, std::string> m_strings;

    std::unordered_map<std::string, size_t> m_queryCounts;
    std::vector<std::string>                m_queryOrder;
};