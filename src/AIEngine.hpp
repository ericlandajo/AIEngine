#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "IActionHandler.hpp"
#include "IVariableUpdater.hpp"
#include "aitoolkit/behtree.hpp"
#include "AIBrainTemplate_generated.h" 

namespace AIEngineData {
    namespace Core {
        struct Node;
        struct AIBrainTemplate;
    }
}

struct Blackboard {
    IActionHandler* actionHandler = nullptr;
    IVariableUpdater* variableUpdater = nullptr;
    float deltaTime = 0.0f;
};

class AIEngine {
public:
    AIEngine(IActionHandler* actionHandler, IVariableUpdater* variableUpdater);
    ~AIEngine() = default;

    bool LoadFromFile(const std::string& filepath);
    
    void Update(float deltaTime); 

private:
    std::unique_ptr<aitoolkit::bt::node<Blackboard>> BuildBTNode(uint32_t nodeId);

    Blackboard m_blackboard;
    std::vector<char> m_fileBuffer;
    
    const AIEngineData::Core::AIBrainTemplate* m_brain = nullptr;
    std::unordered_map<uint32_t, const AIEngineData::Core::Node*> m_nodeMap;

    std::unique_ptr<aitoolkit::bt::node<Blackboard>> m_btRoot;

    std::unique_ptr<aitoolkit::bt::node<Blackboard>> BuildConditionNode(const AIEngineData::Core::Condition* conditionData);

    uint32_t m_currentStateId = 0; 
    bool m_isFSM = false;

    void UpdateBT();
    void UpdateFSM();

    bool EvaluateCondition(const AIEngineData::Core::Condition* conditionData) const;
};