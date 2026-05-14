#include "AIEngine.h"
#include <iostream>

AIEngine::AIEngine(IActionHandler* actionHandler, IVariableUpdater* variableUpdater)
    : m_actionHandler(actionHandler), m_variableUpdater(variableUpdater) {
}

bool AIEngine::LoadFromFile(const std::string& filepath) {
    std::cout << "Carregant arxiu: " << filepath << std::endl;
    // TODO: Llegir el fitxer binari amb FlatBuffers
    return true;
}

void AIEngine::Update() {
    // TODO: Avaluar l'arbre d'aitoolkit
}