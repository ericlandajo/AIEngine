// =====================================================================
//  Test de fum: valida que la infraestructura de tests compila,
//  enllaça contra AIEngineCore i s'executa.
// =====================================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "AIEngine.hpp"

// Handlers mínims només per poder construir l'AIEngine.
namespace {
    struct NullActionHandler : public IActionHandler {
        aitoolkit::bt::execution_state ExecuteAction(const std::string&,
                                                     const ActionParameters&) override {
            return aitoolkit::bt::execution_state::success;
        }
    };

    struct NullVariableUpdater : public IVariableUpdater {
        bool GetBoolVariable(const std::string&) override { return false; }
        float GetFloatVariable(const std::string&) override { return 0.0f; }
        int GetIntVariable(const std::string&) override { return 0; }
        std::string GetStringVariable(const std::string&) override { return ""; }
    };
}

TEST_CASE("La infraestructura de tests funciona") {
    CHECK(1 + 1 == 2);
}

TEST_CASE("El nucli s'enllaça i es pot instanciar un AIEngine") {
    NullActionHandler actions;
    NullVariableUpdater variables;
    AIEngine engine(&actions, &variables);

    // Carregar un fitxer inexistent ha de fallar de forma controlada,
    // sense llançar cap excepció ni provocar un crash.
    // (Cobreix el flux alternatiu 2a del CU-05.)
    CHECK_FALSE(engine.LoadFromFile("aquest_fitxer_no_existeix.bin"));
}