#include "AIEngine.hpp"
#include <iostream>
#include <fstream>
#include "AIBrainTemplate_generated.h" 

AIEngine::AIEngine(IActionHandler* actionHandler, IVariableUpdater* variableUpdater) {
    m_blackboard.actionHandler = actionHandler;
    m_blackboard.variableUpdater = variableUpdater;
}

bool AIEngine::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    m_fileBuffer.resize(size);
    if (!file.read(m_fileBuffer.data(), size)) return false;

    m_brain = AIEngineData::Core::GetAIBrainTemplate(m_fileBuffer.data());
    if (m_brain == nullptr) return false;

    m_nodeMap.clear();
    if (m_brain->nodes() != nullptr) {
        for (const auto* node : *(m_brain->nodes())) {
            m_nodeMap[node->id()] = node;
        }
    }

    if (m_brain->type() == AIEngineData::Core::BrainType_BehaviorTree) {
        m_btRoot = BuildBTNode(m_brain->root_node_id());
        if (m_btRoot) {
            std::cout << "-> Arbre de comportament construït amb èxit!" << std::endl;
        } else {
            std::cerr << "-> ERROR en construir l'arbre." << std::endl;
        }
    }

    return true;
}

std::unique_ptr<aitoolkit::bt::node<Blackboard>> AIEngine::BuildBTNode(uint32_t nodeId) {
    auto it = m_nodeMap.find(nodeId);
    if (it == m_nodeMap.end()) return nullptr;

    const auto* nodeData = it->second;

    switch (nodeData->type()) {
        case AIEngineData::Core::NodeType_Sequence: {
            std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> children;
            if (nodeData->children_ids() != nullptr) {
                for (uint32_t child_id : *(nodeData->children_ids())) {
                    auto childNode = BuildBTNode(child_id);
                    if (childNode) children.push_back(std::move(childNode));
                }
            }
            return std::make_unique<aitoolkit::bt::seq<Blackboard>>(std::move(children));
        }

        case AIEngineData::Core::NodeType_Action: {
            std::string actionName = nodeData->action_name() ? nodeData->action_name()->c_str() : "Unknown";
            
            return std::make_unique<aitoolkit::bt::task<Blackboard>>(
                [actionName](const Blackboard& bb) {
                    bool result = bb.actionHandler->ExecuteAction(actionName);
                    return result ? aitoolkit::bt::execution_state::success : aitoolkit::bt::execution_state::failure;
                }
            );
        }

        case AIEngineData::Core::NodeType_Selector: {
            std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> children;
            if (nodeData->children_ids() != nullptr) {
                for (uint32_t child_id : *(nodeData->children_ids())) {
                    auto childNode = BuildBTNode(child_id);
                    if (childNode) children.push_back(std::move(childNode));
                }
            }
            return std::make_unique<aitoolkit::bt::sel<Blackboard>>(std::move(children));
        }

        case AIEngineData::Core::NodeType_ConditionNode: {
            auto conditionData = nodeData->condition(); 
            
            if (!conditionData || !conditionData->variable()) {
                return std::make_unique<aitoolkit::bt::check<Blackboard>>(
                    [](const Blackboard& bb) { return false; }
                );
            }

            std::string varName = conditionData->variable()->str();
            auto op = conditionData->op(); 
            auto dataType = conditionData->compare_type();
            
            int expectedInt = conditionData->int_val();
            float expectedFloat = conditionData->float_val();
            bool expectedBool = conditionData->bool_val();
            std::string expectedString = conditionData->string_val() ? conditionData->string_val()->str() : "";

            return std::make_unique<aitoolkit::bt::check<Blackboard>>(
                [varName, op, dataType, expectedInt, expectedFloat, expectedBool, expectedString](const Blackboard& bb) {
                    
                    if (dataType == AIEngineData::Core::DataType_Float) {
                        float currentVal = bb.variableUpdater->GetFloatVariable(varName);
                        switch (op) {
                            case AIEngineData::Core::Operator_Equal: return currentVal == expectedFloat;
                            case AIEngineData::Core::Operator_NotEqual: return currentVal != expectedFloat;
                            case AIEngineData::Core::Operator_Greater: return currentVal > expectedFloat;
                            case AIEngineData::Core::Operator_Less: return currentVal < expectedFloat;
                            case AIEngineData::Core::Operator_GreaterOrEqual: return currentVal >= expectedFloat;
                            case AIEngineData::Core::Operator_LessOrEqual: return currentVal <= expectedFloat;
                            default: return false;
                        }
                    }
                    if (dataType == AIEngineData::Core::DataType_Int) {
                        int currentVal = bb.variableUpdater->GetIntVariable(varName);
                        switch (op) {
                            case AIEngineData::Core::Operator_Equal: return currentVal == expectedInt;
                            case AIEngineData::Core::Operator_NotEqual: return currentVal != expectedInt;
                            case AIEngineData::Core::Operator_Greater: return currentVal > expectedInt;
                            case AIEngineData::Core::Operator_Less: return currentVal < expectedInt;
                            case AIEngineData::Core::Operator_GreaterOrEqual: return currentVal >= expectedInt;
                            case AIEngineData::Core::Operator_LessOrEqual: return currentVal <= expectedInt;
                            default: return false;
                        }
                    }
                    if (dataType == AIEngineData::Core::DataType_String) {
                        std::string currentVal = bb.variableUpdater->GetStringVariable(varName);
                        switch (op) {
                            case AIEngineData::Core::Operator_Equal: return currentVal == expectedString;
                            case AIEngineData::Core::Operator_NotEqual: return currentVal != expectedString;
                            case AIEngineData::Core::Operator_Greater: return currentVal > expectedString;
                            case AIEngineData::Core::Operator_Less: return currentVal < expectedString;
                            case AIEngineData::Core::Operator_GreaterOrEqual: return currentVal >= expectedString;
                            case AIEngineData::Core::Operator_LessOrEqual: return currentVal <= expectedString;
                            default: return false;
                        }
                    } 
                    else if (dataType == AIEngineData::Core::DataType_Bool) {
                        bool currentVal = bb.variableUpdater->GetBoolVariable(varName);
                        switch (op) {
                            case AIEngineData::Core::Operator_Equal: return currentVal == expectedBool;
                            case AIEngineData::Core::Operator_NotEqual: return currentVal != expectedBool;
                            default: return false;
                        }
                    }
                    return false;
                }
            );
        }

        default:
            std::cerr << "Tipus de node no implementat encara." << std::endl;
            return nullptr;
    }
}

void AIEngine::Update() {
    if (m_btRoot) {
        m_btRoot->evaluate(m_blackboard);
    }
}