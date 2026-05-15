#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "IActionHandler.hpp"
#include "IVariableUpdater.hpp"

#include "aitoolkit/behtree.hpp"

namespace AIEngineData {
    namespace Core {
        struct Node;
        struct AIBrainTemplate;
    }
}

struct Blackboard {
    IActionHandler* actionHandler = nullptr;
    IVariableUpdater* variableUpdater = nullptr;
};

class AIEngine {
public:
    AIEngine(IActionHandler* actionHandler, IVariableUpdater* variableUpdater);
    ~AIEngine() = default;

    bool LoadFromFile(const std::string& filepath);
    
    void Update();

private:
    std::unique_ptr<aitoolkit::bt::node<Blackboard>> BuildBTNode(uint32_t nodeId);

    Blackboard m_blackboard;
    std::vector<char> m_fileBuffer;
    
    const AIEngineData::Core::AIBrainTemplate* m_brain = nullptr;
    std::unordered_map<uint32_t, const AIEngineData::Core::Node*> m_nodeMap;

    std::unique_ptr<aitoolkit::bt::node<Blackboard>> m_btRoot;
};