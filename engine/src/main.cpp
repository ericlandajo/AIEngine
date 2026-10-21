#include <iostream>
#include "AIEngine.hpp"

class DummyActionHandler : public IActionHandler {
public:
    virtual aitoolkit::bt::execution_state ExecuteAction(const std::string& actionName, const ActionParameters& params) override {
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
        return aitoolkit::bt::execution_state::success;
    }
};

class DummyVariableUpdater : public IVariableUpdater {
public:
    bool GetBoolVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta pel Bool: " << varName << " -> Retornem true" << std::endl;
        return true;
    }

    float GetFloatVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta pel Float: " << varName << " -> Retornem 3.14" << std::endl;
        return 3.14f; 
    }

    int GetIntVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta per l'Int: " << varName << " -> Retornem 10" << std::endl;
        return 10;
    }

    std::string GetStringVariable(const std::string& varName) override {
        std::cout << "  [JOC] L'Engine pregunta per la String: " << varName << " -> Retornem 'Hello, world!'" << std::endl;
        return "Hello, world";
    }
};

int main(int argc, char** argv) {
    // Ruta del cervell: primer argument, o el de les proves per defecte.
    const std::string brainPath = (argc > 1) ? argv[1] : "tests/fixtures/test_brain.bin";
    const int frames = 5;

    std::cout << "--- INICIANT MOTOR D'IA ---" << std::endl;

    DummyActionHandler actionHandler;
    DummyVariableUpdater variableUpdater;

    AIEngine engine(&actionHandler, &variableUpdater);

    if (!engine.LoadFromFile(brainPath)) {
        std::cerr << "No s'ha pogut carregar el cervell: " << brainPath << std::endl;
        return 1;
    }

    // deltaTime fals de 0.016 s (60 FPS) per a les proves de consola
    for (int i = 1; i <= frames; ++i) {
        std::cout << "--- FOTOGRAMA " << i << " ---" << std::endl;
        engine.Update(0.016f);
    }

    std::cout << "--- FINALITZANT MOTOR D'IA ---" << std::endl;
    return 0;
}