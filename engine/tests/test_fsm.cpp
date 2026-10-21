// =====================================================================
//  Tests de la lògica de màquines d'estats finits. A cada tick, el
//  motor avalua primer les transicions que surten de l'estat actual i,
//  tot seguit, executa l'acció de l'estat en què ha quedat.
//
//  Ordre d'execució dins d'un mateix Update():
//    1. Recórrer les arestes amb source_id == estat actual
//    2. La primera transició amb condició certa canvia l'estat i atura
//       la cerca (break)
//    3. Executar l'acció de l'estat resultant
//
//  Això vol dir que, en el tick en què es dispara una transició, ja
//  s'executa l'acció del NOU estat, no la de l'anterior.
// =====================================================================

#include <doctest/doctest.h>

#include "helpers/BrainBuilder.hpp"
#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "AIEngine.hpp"

using namespace test;
namespace Core = AIEngineData::Core;

// =====================================================================
//  Estat inicial
// =====================================================================

TEST_CASE("La FSM comença a l'estat marcat com a arrel") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.WasCalled("Patrol"));
    CHECK_FALSE(actions.WasCalled("Chase"));
}

TEST_CASE("L'estat inicial pot ser qualsevol node, no necessàriament el primer") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Idle")
        .SetRoot(2)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.WasCalled("Idle"));
    CHECK_FALSE(actions.WasCalled("Patrol"));
}

// =====================================================================
//  Execució de l'acció de l'estat
// =====================================================================

TEST_CASE("L'acció de l'estat actual s'executa a cada tick") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);
    engine.Update(0.016f);
    engine.Update(0.016f);

    CHECK(actions.CallCount("Patrol") == 3);
}

TEST_CASE("Els paràmetres de l'estat arriben al handler") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol", { Param::Float("speed", 2.5f) })
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    auto params = actions.LastParamsOf("Patrol");
    REQUIRE(params.count("speed") == 1);
    CHECK(params["speed"].float_val == doctest::Approx(2.5f));
}

TEST_CASE("Un estat sense acció no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

// =====================================================================
//  Transicions
// =====================================================================

TEST_CASE("La transició no es dispara mentre la condició no es compleix") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);
    engine.Update(0.016f);

    CHECK(actions.CallCount("Patrol") == 2);
    CHECK_FALSE(actions.WasCalled("Chase"));
}

TEST_CASE("La transició es dispara quan la condició es compleix") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);                    // encara patrullant
    vars.SetBool("playerVisible", true);
    engine.Update(0.016f);                    // transició i acció nova

    std::vector<std::string> esperat{ "Patrol", "Chase" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("En disparar-se la transició s'executa ja l'acció del nou estat") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    // Ja al primer tick es transiciona i s'executa Chase, no Patrol:
    // les transicions s'avaluen abans d'executar l'acció de l'estat.
    engine.Update(0.016f);

    CHECK(actions.WasCalled("Chase"));
    CHECK_FALSE(actions.WasCalled("Patrol"));
}

TEST_CASE("L'estat es manté un cop transicionat, encara que la condició deixi de complir-se") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);                 // passa a Chase
    vars.SetBool("playerVisible", false);
    engine.Update(0.016f);                 // sense transició de tornada, s'hi queda

    CHECK(actions.CallCount("Chase") == 2);
}

TEST_CASE("Una transició de tornada permet recuperar l'estat anterior") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        .AddTransition(2, 1, Cond::Bool("playerVisible", Core::Operator_Equal, false))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);                  // Patrol
    vars.SetBool("playerVisible", true);
    engine.Update(0.016f);                  // Chase
    vars.SetBool("playerVisible", false);
    engine.Update(0.016f);                  // torna a Patrol

    std::vector<std::string> esperat{ "Patrol", "Chase", "Patrol" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("Només es dispara una transició per tick, encara que n'hi hagi d'encadenades") {
    // Cadena A -> B -> C amb totes les condicions certes alhora.
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "A")
        .AddState(2, "B")
        .AddState(3, "C")
        .AddTransition(1, 2, Cond::Bool("go", Core::Operator_Equal, true))
        .AddTransition(2, 3, Cond::Bool("go", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("go", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);   // A -> B
    engine.Update(0.016f);   // B -> C

    std::vector<std::string> esperat{ "B", "C" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("Si diverses transicions es compleixen alhora, guanya la primera declarada") {
    // Dues sortides des de l'estat 1, totes dues amb condició certa.
    // El motor s'atura a la primera que troba (break), de manera que
    // l'ordre de declaració de les arestes determina la prioritat.
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Inici")
        .AddState(2, "Primera")
        .AddState(3, "Segona")
        .AddTransition(1, 2, Cond::Bool("go", Core::Operator_Equal, true))
        .AddTransition(1, 3, Cond::Bool("go", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("go", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.WasCalled("Primera"));
    CHECK_FALSE(actions.WasCalled("Segona"));
}

TEST_CASE("Les transicions d'altres estats no afecten l'estat actual") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddState(3, "Flee")
        // Transició que surt de l'estat 2, no del 1
        .AddTransition(2, 3, Cond::Bool("go", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("go", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.WasCalled("Patrol"));
    CHECK_FALSE(actions.WasCalled("Flee"));
}

// =====================================================================
//  Condicions de transició amb els diferents tipus
// =====================================================================

TEST_CASE("Les transicions admeten condicions numèriques") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Fight")
        .AddState(2, "Flee")
        .AddTransition(1, 2, Cond::Float("health", Core::Operator_Less, 20.0f))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetFloat("health", 100.0f);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);            // salut alta: segueix lluitant
    vars.SetFloat("health", 15.0f);
    engine.Update(0.016f);            // salut baixa: fuig

    std::vector<std::string> esperat{ "Fight", "Flee" };
    CHECK(actions.CallSequence() == esperat);
}

// =====================================================================
//  Consulta sota demanda a les FSM
// =====================================================================

TEST_CASE("La FSM només consulta les variables de les transicions de l'estat actual") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddState(3, "Flee")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        // Aquesta surt de l'estat 2, que no és l'actual
        .AddTransition(2, 3, Cond::Float("health", Core::Operator_Less, 20.0f))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetFloat("health", 5.0f);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(vars.WasQueried("playerVisible"));
    // "health" només intervé en una transició que surt d'un altre estat
    CHECK_FALSE(vars.WasQueried("health"));
}

// =====================================================================
//  Robustesa
// =====================================================================

TEST_CASE("Una FSM sense transicions es manté indefinidament al mateix estat") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Idle")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    for (int i = 0; i < 5; ++i) engine.Update(0.016f);

    CHECK(actions.CallCount("Idle") == 5);
}

TEST_CASE("Una transició cap a un estat inexistent no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddTransition(1, 99, Cond::Bool("go", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("go", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    // L'estat destí no existeix al mapa de nodes: el motor no ha de
    // petar, simplement no troba cap acció a executar.
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Un estat arrel inexistent no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .SetRoot(99)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);
    engine.LoadFromFile(brain.Path());

    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}