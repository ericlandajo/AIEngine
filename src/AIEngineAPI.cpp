#include "AIEngineAPI.h"
#include "AIEngine.hpp"
#include <iostream>

class UnityActionHandler : public IActionHandler {
private:
    ActionCallback m_actionCb;
public:
    UnityActionHandler(ActionCallback cb) : m_actionCb(cb) {}

    aitoolkit::bt::execution_state ExecuteAction(const std::string& actionName, const ActionParameters& params) override {
        if (m_actionCb) {
            int actionResult = m_actionCb(actionName.c_str());
            
            switch (actionResult) {
                case 1:  return aitoolkit::bt::execution_state::success;
                case 2:  return aitoolkit::bt::execution_state::running;
                case 0:
                default: return aitoolkit::bt::execution_state::failure;
            }
        }
        return aitoolkit::bt::execution_state::failure;
    }
};

class UnityVariableUpdater : public IVariableUpdater {
private:
    GetFloatVarCallback m_floatCb;
    GetBoolVarCallback m_boolCb;
    GetIntVarCallback m_intCb;
    GetStringVarCallback m_stringCb;
public:
    UnityVariableUpdater(GetFloatVarCallback fCb, GetBoolVarCallback bCb, GetIntVarCallback iCb) 
        : m_floatCb(fCb), m_boolCb(bCb), m_intCb(iCb) {}

    float GetFloatVariable(const std::string& varName) override {
        if (m_floatCb) return m_floatCb(varName.c_str());
        return 0.0f;
    }

    bool GetBoolVariable(const std::string& varName) override {
        if (m_boolCb) return m_boolCb(varName.c_str());
        return false;
    }

    int GetIntVariable(const std::string& varName) override {
        if (m_intCb) return m_intCb(varName.c_str());
        return 0;
    }

    std::string GetStringVariable(const std::string& varName) override {
        if (m_stringCb) {
            const char* unityStr = m_stringCb(varName.c_str());
            if (unityStr != nullptr) {
                return std::string(unityStr);
            }
        }
        return "";
    }
};

struct EngineContainer {
    UnityActionHandler* actionHandler;
    UnityVariableUpdater* variableUpdater;
    AIEngine* engine;
};

extern "C" {

void* AIEngine_Create(ActionCallback actionCb, GetFloatVarCallback floatCb, GetBoolVarCallback boolCb, GetIntVarCallback intCb, GetStringVarCallback stringCb) {
        EngineContainer* container = new EngineContainer();
        
        container->actionHandler = new UnityActionHandler(actionCb);
        container->variableUpdater = new UnityVariableUpdater(floatCb, boolCb, intCb, stringCb);
        container->engine = new AIEngine(container->actionHandler, container->variableUpdater);
        
        return container;
    }

    void AIEngine_Destroy(void* enginePtr) {
        if (!enginePtr) return;
        EngineContainer* container = static_cast<EngineContainer*>(enginePtr);
        
        delete container->engine;
        delete container->variableUpdater;
        delete container->actionHandler;
        delete container;
    }

    bool AIEngine_LoadBrain(void* enginePtr, const char* filePath) {
        if (!enginePtr) return false;
        EngineContainer* container = static_cast<EngineContainer*>(enginePtr);
        return container->engine->LoadFromFile(filePath);
    }

    void AIEngine_Update(void* enginePtr, float deltaTime) {
        if (!enginePtr) return;
        EngineContainer* container = static_cast<EngineContainer*>(enginePtr);
        container->engine->Update(deltaTime);
    }
}