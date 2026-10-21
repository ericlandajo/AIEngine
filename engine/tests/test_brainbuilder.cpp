// =====================================================================
//  Valida que el BrainBuilder genera fitxers .bin que el motor és
//  capaç de carregar i interpretar.
// =====================================================================

#include <doctest/doctest.h>

#include "helpers/BrainBuilder.hpp"
#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "AIEngine.hpp"

using namespace test;
using State = aitoolkit::bt::execution_state;
namespace Core = AIEngineData::Core;

TEST_CASE("BrainBuilder genera un arbre mínim que el motor pot carregar") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddAction(1, "Patrol")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater variables;
    AIEngine engine(&actions, &variables);

    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.WasCalled("Patrol"));
}

TEST_CASE("BrainBuilder trasllada els paràmetres de l'acció fins al handler") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddAction(1, "MoveTo", { Param::Float("speed", 3.5f),
                                  Param::Int("priority", 2) })
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater variables;
    AIEngine engine(&actions, &variables);

    REQUIRE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    auto params = actions.LastParamsOf("MoveTo");
    REQUIRE(params.count("speed") == 1);
    CHECK(params["speed"].float_val == doctest::Approx(3.5f));
    REQUIRE(params.count("priority") == 1);
    CHECK(params["priority"].int_val == 2);
}

TEST_CASE("BrainBuilder construeix arbres amb nodes compostos") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2, 3 })
        .AddAction(2, "Primera")
        .AddAction(3, "Segona")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater variables;
    AIEngine engine(&actions, &variables);

    REQUIRE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    std::vector<std::string> esperat{ "Primera", "Segona" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("BrainBuilder construeix màquines d'estats amb transicions") {
    auto brain = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "Patrol")
        .AddState(2, "Chase")
        .AddTransition(1, 2, Cond::Bool("playerVisible", Core::Operator_Equal, true))
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater variables;
    AIEngine engine(&actions, &variables);

    REQUIRE(engine.LoadFromFile(brain.Path()));

    // Sense veure el jugador, es manté patrullant
    variables.SetBool("playerVisible", false);
    engine.Update(0.016f);
    CHECK(actions.WasCalled("Patrol"));
    CHECK_FALSE(actions.WasCalled("Chase"));

    // En veure'l, la transició es dispara
    variables.SetBool("playerVisible", true);
    engine.Update(0.016f);
    CHECK(actions.WasCalled("Chase"));
}

TEST_CASE("El fitxer temporal s'esborra en destruir-se el TempBrain") {
    std::string path;
    {
        auto brain = BrainBuilder()
            .AsBehaviorTree()
            .AddAction(1, "Patrol")
            .SetRoot(1)
            .Build();
        path = brain.Path();

        std::ifstream existeix(path, std::ios::binary);
        CHECK(existeix.good());
    }

    std::ifstream jaNoExisteix(path, std::ios::binary);
    CHECK_FALSE(jaNoExisteix.good());
}