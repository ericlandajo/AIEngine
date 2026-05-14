#pragma once
#include <string>
#include "IActionHandler.h"
#include "IVariableUpdater.h"

class AIEngine {
public:
    AIEngine(IActionHandler* actionHandler, IVariableUpdater* variableUpdater);
    ~AIEngine() = default;

    bool LoadFromFile(const std::string& filepath);

    void Update();

private:
    IActionHandler* m_actionHandler;
    IVariableUpdater* m_variableUpdater;

    // Aquí es guardarà el punter a l'arrel de l'arbre d'aitoolkit
};