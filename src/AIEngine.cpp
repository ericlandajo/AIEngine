#include "AIEngine.h"
#include <iostream>
#include <fstream>
#include "AIBrainTemplate_generated.h" 

AIEngine::AIEngine(IActionHandler* actionHandler, IVariableUpdater* variableUpdater)
    : m_actionHandler(actionHandler), m_variableUpdater(variableUpdater) {
}

bool AIEngine::LoadFromFile(const std::string& filepath) {
    std::cout << "Intentant carregar arxiu: " << filepath << std::endl;

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "ERROR: No s'ha pogut obrir el fitxer " << filepath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    m_fileBuffer.resize(size);
    if (!file.read(m_fileBuffer.data(), size)) {
        std::cerr << "ERROR: No s'ha pogut llegir el contingut de " << filepath << std::endl;
        return false;
    }

    auto aiBrain = AIEngineData::Core::GetAIBrainTemplate(m_fileBuffer.data());
    
    if (aiBrain == nullptr) {
        std::cerr << "ERROR: El fitxer no es un AIBrainTemplate valid." << std::endl;
        return false;
    }

    std::cout << "Exit! S'ha llegit el cervell correctament." << std::endl;
    
    if (aiBrain->type() == AIEngineData::Core::BrainType_BehaviorTree) {
        std::cout << "Tipus de cervell: Behavior Tree" << std::endl;
    } else {
        std::cout << "Tipus de cervell: FSM" << std::endl;
    }

    std::cout << "Node arrel ID: " << aiBrain->root_node_id() << std::endl;
    
    if (aiBrain->nodes() != nullptr) {
        std::cout << "Total de nodes trobats: " << aiBrain->nodes()->size() << std::endl;
    }

    return true;
}

void AIEngine::Update() {
    // TODO: Avaluar l'arbre
}