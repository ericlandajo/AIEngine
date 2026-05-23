#include <iostream>
#include "AIEngine.hpp"

class DummyActionHandler : public IActionHandler {
public:
    bool ExecuteAction(const std::string& actionName, const ActionParameters& params) override {
        std::cout << "  [JOC] Executant acció: " << actionName;
        
        if (!params.empty()) {
            std::cout << " | Params: { ";
            for (const auto& [name, param] : params) {
                std::cout << name << ": ";
                switch (param.type) {
                    case ActionParameter::Type::Int: std::cout << param.int_val; break;
                    case ActionParameter::Type::Float: std::cout << param.float_val; break;
                    case ActionParameter::Type::Bool: std::cout << (param.bool_val ? "true" : "false"); break;
                    case ActionParameter::Type::String: std::cout << "\"" << param.string_val << "\""; break;
                }
                std::cout << " ";
            }
            std::cout << "}";
        }
        std::cout << std::endl;
        return true; 
    }
};

class DummyVariableUpdater : public IVariableUpdater {
public:
    bool GetBoolVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta pel Bool: " << varName << " -> Retornem true" << std::endl;
        return true; // Return per defecte per proves
    }

    float GetFloatVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta pel Float: " << varName << " -> Retornem 3.14" << std::endl;
        return 3.14f; // Return per defecte per proves
    }

    int GetIntVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta per l'Int: " << varName << " -> Retornem 10" << std::endl;
        return 10; // Return per defecte per proves
    }

    std::string GetStringVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta per la String: " << varName << " -> Retornem 'Hello, world!'" << std::endl;
        return "Hello, world"; // Return per defecte per proves
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