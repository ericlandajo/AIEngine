#include <iostream>
#include "AIEngine.hpp"

class DummyActionHandler : public IActionHandler {
public:
    bool ExecuteAction(const std::string& actionName) override {
        std::cout << "  [JOC] Executant acció: " << actionName << std::endl;
        return true; 
    }
};

class DummyVariableUpdater : public IVariableUpdater {
public:
    bool GetCondition(const std::string& conditionName) override {
        return true; 
    }
};

int main() {
    std::cout << "--- INICIANT MOTOR D'IA ---" << std::endl;

    DummyActionHandler actionHandler;
    DummyVariableUpdater variableUpdater;

    AIEngine engine(&actionHandler, &variableUpdater);

    engine.LoadFromFile("schema/test_brain.bin");

    std::cout << "--- EXECUTANT UPDATE ---" << std::endl;
    engine.Update();

    std::cout << "--- FINALITZANT MOTOR D'IA ---" << std::endl;
    return 0;
}