#include "AIEngine.hpp"
#include <iostream>
#include <fstream>
#include "AIBrainTemplate_generated.h" 
#include "ParallelNode.hpp"
#include "DecoratorNode.hpp"

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

    flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t*>(m_fileBuffer.data()), m_fileBuffer.size());
    if (!AIEngineData::Core::VerifyAIBrainTemplateBuffer(verifier)) {
        std::cerr << "[ERROR] El fitxer no conté un cervell d'IA vàlid." << std::endl;
        return false;
    }

    m_brain = AIEngineData::Core::GetAIBrainTemplate(m_fileBuffer.data());
    if (m_brain == nullptr) return false;

    m_nodeMap.clear();
    if (m_brain->nodes() != nullptr) {
        for (const auto* node : *(m_brain->nodes())) {
            m_nodeMap[node->id()] = node;
        }
    }

    if (m_brain->type() == AIEngineData::Core::BrainType_BehaviorTree) {
        m_isFSM = false;
        std::unordered_set<uint32_t> path;
        m_btRoot = BuildBTNode(m_brain->root_node_id(), path);
        if (!m_btRoot) {
            std::cerr << "-> ERROR en construir l'arbre." << std::endl;
            return false;
        }
        std::cout << "-> Arbre de comportament construït amb èxit!" << std::endl;
    } 
    else if (m_brain->type() == AIEngineData::Core::BrainType_FiniteStateMachine) {
        m_isFSM = true;
        m_currentStateId = m_brain->root_node_id(); // El root és l'estat inicial
        if (m_nodeMap.find(m_currentStateId) == m_nodeMap.end()) {
            std::cerr << "-> ERROR: l'estat inicial no existeix." << std::endl;
            return false;
        }
        std::cout << "-> Màquina d'Estats construïda amb èxit! Estat inicial: "
                  << m_currentStateId << std::endl;
    }

    return true;
}

std::unique_ptr<aitoolkit::bt::node<Blackboard>> AIEngine::BuildBTNode(
        uint32_t nodeId, std::unordered_set<uint32_t>& path) {

    auto it = m_nodeMap.find(nodeId);
    if (it == m_nodeMap.end()) return nullptr;

    // Si el node ja és al camí que estem recorrent, el graf conté un
    // cicle. Es descarta la branca per evitar una recursió infinita.
    if (!path.insert(nodeId).second) {
        std::cerr << "[ERROR] Cicle detectat a l'arbre, node id: " << nodeId << std::endl;
        return nullptr;
    }

    // Treu el node del camí en sortir, sigui per quin punt sigui
    struct PathGuard {
        std::unordered_set<uint32_t>& p;
        uint32_t id;
        ~PathGuard() { p.erase(id); }
    } guard{ path, nodeId };

    const auto* nodeData = it->second;

    switch (nodeData->type()) {
        case AIEngineData::Core::NodeType_Sequence: {
            std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> children;
            if (nodeData->children_ids() != nullptr) {
                for (uint32_t child_id : *(nodeData->children_ids())) {
                    auto childNode = BuildBTNode(child_id, path);
                    if (childNode) children.push_back(std::move(childNode));
                }
            }
            if (children.empty()) {
                std::cerr << "[ERROR] Node compost sense fills vàlids, id: " << nodeId << std::endl;
                return nullptr;
            }
            return std::make_unique<aitoolkit::bt::seq<Blackboard>>(std::move(children));
        }

    case AIEngineData::Core::NodeType_Action: {
            std::string actionName = nodeData->action_name() ? nodeData->action_name()->c_str() : "";

            ActionParameters params;
            if (nodeData->action_params() != nullptr) {
                for (const auto* paramData : *(nodeData->action_params())) {
                    if (!paramData->name()) continue;
                    
                    ActionParameter p;
                    p.type = static_cast<ActionParameter::Type>(paramData->type());
                    p.int_val = paramData->int_val();
                    p.float_val = paramData->float_val();
                    p.bool_val = paramData->bool_val();
                    if (paramData->string_val()) p.string_val = paramData->string_val()->c_str();
                    
                    params[paramData->name()->c_str()] = p;
                }
            }
            return std::make_unique<aitoolkit::bt::task<Blackboard>>(
                [actionName, params](Blackboard& bb) {
                    if (bb.actionHandler) {
                        return bb.actionHandler->ExecuteAction(actionName, params);
                    }
                    return aitoolkit::bt::execution_state::failure;
                }
            );
        }

        case AIEngineData::Core::NodeType_ConditionNode: {
            return BuildConditionNode(nodeData->condition());
        }

        case AIEngineData::Core::NodeType_Selector: {
            std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> children;
            if (nodeData->children_ids() != nullptr) {
                for (uint32_t child_id : *(nodeData->children_ids())) {
                    auto childNode = BuildBTNode(child_id, path);
                    if (childNode) children.push_back(std::move(childNode));
                }
            }
            if (children.empty()) {
                std::cerr << "[ERROR] Node compost sense fills vàlids, id: " << nodeId << std::endl;
                return nullptr;
            }
            return std::make_unique<aitoolkit::bt::sel<Blackboard>>(std::move(children));
        }

        case AIEngineData::Core::NodeType_Parallel: {
            std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> children;
            if (nodeData->children_ids() != nullptr) {
                for (uint32_t child_id : *(nodeData->children_ids())) {
                    auto childNode = BuildBTNode(child_id, path);
                    if (childNode) children.push_back(std::move(childNode));
                }
            }
            if (children.empty()) {
                std::cerr << "[ERROR] Node compost sense fills vàlids, id: " << nodeId << std::endl;
                return nullptr;
            }
            size_t success_threshold = 1;
            size_t failure_threshold = children.size();

            return std::make_unique<ParallelNode<Blackboard>>(
                success_threshold, 
                failure_threshold, 
                std::move(children)
            );
        }

        case AIEngineData::Core::NodeType_Decorator: {
            if (nodeData->children_ids() == nullptr || nodeData->children_ids()->size() == 0) {
                std::cerr << "[ERROR] S'ha intentat crear un Decorator sense fill!" << std::endl;
                return nullptr;
            }
            
            auto childNode = BuildBTNode(nodeData->children_ids()->Get(0), path);
            if (!childNode) return nullptr;

            std::string decType = nodeData->action_name() ? nodeData->action_name()->str() : "Inverter";

            if (decType == "Inverter") {
                return std::make_unique<InverterNode<Blackboard>>(std::move(childNode));
            } 
            else if (decType == "ForceSuccess") {
                return std::make_unique<ForceSuccessNode<Blackboard>>(std::move(childNode));
            } 
            else if (decType == "ForceFailure") {
                return std::make_unique<ForceFailureNode<Blackboard>>(std::move(childNode));
            } 
            else if (decType == "SuccessIsFailure") {
                return std::make_unique<SuccessIsFailureNode<Blackboard>>(std::move(childNode));
            } 
            else if (decType == "Repeater") {
                int count = 3;
                if (nodeData->action_params() && nodeData->action_params()->size() > 0) {
                    count = nodeData->action_params()->Get(0)->int_val();
                }
                return std::make_unique<RepeaterNode<Blackboard>>(count, std::move(childNode));
            } 
            else if (decType == "RetryUntilSuccess") {
                return std::make_unique<RetryUntilSuccessNode<Blackboard>>(std::move(childNode));
            } 
            else if (decType == "ConditionCheck") {
                std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>> checkSeq;
                checkSeq.push_back(BuildConditionNode(nodeData->condition()));
                checkSeq.push_back(std::move(childNode));
                return std::make_unique<aitoolkit::bt::seq<Blackboard>>(std::move(checkSeq));
            }
            else if (decType == "Cooldown") {
                float time = 1.0f;
                if (nodeData->action_params() && nodeData->action_params()->size() > 0) {
                    time = nodeData->action_params()->Get(0)->float_val();
                }
                return std::make_unique<CooldownNode<Blackboard>>(time, std::move(childNode));
            }
            else if (decType == "Timeout") {
                float time = 1.0f;
                if (nodeData->action_params() && nodeData->action_params()->size() > 0) {
                    time = nodeData->action_params()->Get(0)->float_val();
                }
                return std::make_unique<TimeoutNode<Blackboard>>(time, std::move(childNode));
            }
            else {
                std::cerr << "[WARNING] Decorator desconegut o no suportat: " << decType << std::endl;
                return childNode; 
            }
        }

        default:
            std::cerr << "Tipus de node no implementat encara." << std::endl;
            return nullptr;
    }
}

std::unique_ptr<aitoolkit::bt::node<Blackboard>> AIEngine::BuildConditionNode(const AIEngineData::Core::Condition* conditionData) {
    return std::make_unique<aitoolkit::bt::check<Blackboard>>(
        [this, conditionData](const Blackboard& bb) {
            return this->EvaluateCondition(conditionData);
        }
    );
}

bool AIEngine::EvaluateCondition(const AIEngineData::Core::Condition* conditionData) const {
    if (!conditionData || !conditionData->variable()) return false;

    std::string varName = conditionData->variable()->str();
    auto op = conditionData->op(); 
    auto dataType = conditionData->compare_type();
    
    if (dataType == AIEngineData::Core::DataType_Float) {
        float currentVal = m_blackboard.variableUpdater->GetFloatVariable(varName);
        float expectedFloat = conditionData->float_val();
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
    else if (dataType == AIEngineData::Core::DataType_Bool) {
        bool currentVal = m_blackboard.variableUpdater->GetBoolVariable(varName);
        bool expectedBool = conditionData->bool_val();
        switch (op) {
            case AIEngineData::Core::Operator_Equal: return currentVal == expectedBool;
            case AIEngineData::Core::Operator_NotEqual: return currentVal != expectedBool;
            default: return false;
        }
    }
    else if (dataType == AIEngineData::Core::DataType_Int) {
        int currentVal = m_blackboard.variableUpdater->GetIntVariable(varName);
        int expectedInt = conditionData->int_val();
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
    else if (dataType == AIEngineData::Core::DataType_String) {
        std::string currentVal = m_blackboard.variableUpdater->GetStringVariable(varName);
        std::string expectedString = conditionData->string_val() ? conditionData->string_val()->str() : "";
        switch (op) {
            case AIEngineData::Core::Operator_Equal: return currentVal == expectedString;
            case AIEngineData::Core::Operator_NotEqual: return currentVal != expectedString;
            default: return false; 
        }
    }

    return false;
}

void AIEngine::Update(float deltaTime) {
    m_blackboard.deltaTime = deltaTime;
    if (m_isFSM) {
        UpdateFSM();
    } else {
        UpdateBT();
    }
}

void AIEngine::UpdateBT() {
    if (m_btRoot) {
        m_btRoot->evaluate(m_blackboard);
    }
}

void AIEngine::UpdateFSM() {
    if (!m_brain) return;

    if (m_brain->edges() != nullptr) {
        for (const auto* edge : *(m_brain->edges())) {
            
            if (edge->source_id() == m_currentStateId) {
                
                if (EvaluateCondition(edge->condition())) {
                    
                    m_currentStateId = edge->target_id();
                    std::cout << "  [FSM] Transició disparada! Nou Estat ID: " << m_currentStateId << std::endl;
                    
                    break; 
                }
            }
        }
    }

    auto it = m_nodeMap.find(m_currentStateId);
    if (it != m_nodeMap.end()) {
        const auto* stateNode = it->second;
        
        if (stateNode->type() == AIEngineData::Core::NodeType_State) {
            std::string actionName = stateNode->action_name() ? stateNode->action_name()->c_str() : "";
            
            if (!actionName.empty()) {
                // Preparem el diccionari de paràmetres
                ActionParameters params;
                
                // Si el node té paràmetres al FlatBuffer, els llegim
                if (stateNode->action_params() != nullptr) {
                    for (const auto* paramData : *(stateNode->action_params())) {
                        if (!paramData->name()) continue;
                        
                        ActionParameter p;
                        p.type = static_cast<ActionParameter::Type>(paramData->type());
                        p.int_val = paramData->int_val();
                        p.float_val = paramData->float_val();
                        p.bool_val = paramData->bool_val();
                        if (paramData->string_val()) p.string_val = paramData->string_val()->c_str();
                        
                        params[paramData->name()->c_str()] = p;
                    }
                }
                
                // Enviem l'acció amb els paràmetres inclosos!
                m_blackboard.actionHandler->ExecuteAction(actionName, params);
            }
        }
    }
}