// =====================================================================
//  Valida el comportament dels mateixos dobles de prova. Si un mock té
//  un error, tots els tests que hi confiïn donarien resultats falsos.
// =====================================================================

#include <doctest/doctest.h>

#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "mocks/MockNode.hpp"
#include "DecoratorNode.hpp"

using State = aitoolkit::bt::execution_state;

// =====================================================================
//  MockActionHandler
// =====================================================================

TEST_CASE("MockActionHandler registra les crides rebudes") {
    MockActionHandler actions;
    ActionParameters noParams;

    actions.ExecuteAction("Patrol", noParams);
    actions.ExecuteAction("Chase", noParams);
    actions.ExecuteAction("Patrol", noParams);

    CHECK(actions.TotalCalls() == 3);
    CHECK(actions.CallCount("Patrol") == 2);
    CHECK(actions.CallCount("Chase") == 1);
    CHECK(actions.WasCalled("Patrol"));
    CHECK_FALSE(actions.WasCalled("Flee"));
}

TEST_CASE("MockActionHandler conserva l'ordre de les crides") {
    MockActionHandler actions;
    ActionParameters noParams;

    actions.ExecuteAction("Primera", noParams);
    actions.ExecuteAction("Segona", noParams);

    CHECK(actions.CallAt(0) == "Primera");
    CHECK(actions.CallAt(1) == "Segona");

    std::vector<std::string> esperat{ "Primera", "Segona" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("MockActionHandler retorna el resultat programat") {
    MockActionHandler actions;
    ActionParameters noParams;

    actions.SetResult("Fallida", State::failure);

    CHECK(actions.ExecuteAction("Fallida", noParams) == State::failure);
    // Una acció no configurada retorna el valor per defecte
    CHECK(actions.ExecuteAction("Altra", noParams) == State::success);

    actions.SetDefaultResult(State::running);
    CHECK(actions.ExecuteAction("Altra", noParams) == State::running);
}

TEST_CASE("MockActionHandler recorre una seqüència de resultats") {
    MockActionHandler actions;
    ActionParameters noParams;

    // Acció que triga tres frames a completar-se
    actions.SetResultSequence("MoveTo",
        { State::running, State::running, State::success });

    CHECK(actions.ExecuteAction("MoveTo", noParams) == State::running);
    CHECK(actions.ExecuteAction("MoveTo", noParams) == State::running);
    CHECK(actions.ExecuteAction("MoveTo", noParams) == State::success);
    // Un cop exhaurida, es manté l'últim valor
    CHECK(actions.ExecuteAction("MoveTo", noParams) == State::success);
}

TEST_CASE("MockActionHandler captura els paràmetres de l'acció") {
    MockActionHandler actions;

    ActionParameters params;
    ActionParameter speed;
    speed.type = ActionParameter::Type::Float;
    speed.float_val = 3.5f;
    params["speed"] = speed;

    actions.ExecuteAction("MoveTo", params);

    auto capturats = actions.LastParamsOf("MoveTo");
    REQUIRE(capturats.count("speed") == 1);
    CHECK(capturats["speed"].float_val == doctest::Approx(3.5f));
}

// =====================================================================
//  MockVariableUpdater
// =====================================================================

TEST_CASE("MockVariableUpdater retorna els valors programats") {
    MockVariableUpdater vars;

    vars.SetBool("isAlerted", true);
    vars.SetFloat("health", 15.5f);
    vars.SetInt("ammo", 7);
    vars.SetString("state", "patrolling");

    CHECK(vars.GetBoolVariable("isAlerted") == true);
    CHECK(vars.GetFloatVariable("health") == doctest::Approx(15.5f));
    CHECK(vars.GetIntVariable("ammo") == 7);
    CHECK(vars.GetStringVariable("state") == "patrolling");
}

TEST_CASE("MockVariableUpdater retorna valors per defecte si no estan configurats") {
    MockVariableUpdater vars;

    CHECK(vars.GetBoolVariable("desconeguda") == false);
    CHECK(vars.GetFloatVariable("desconeguda") == doctest::Approx(0.0f));
    CHECK(vars.GetIntVariable("desconeguda") == 0);
    CHECK(vars.GetStringVariable("desconeguda").empty());
}

TEST_CASE("MockVariableUpdater registra quines variables s'han consultat") {
    MockVariableUpdater vars;
    vars.SetFloat("health", 100.0f);
    vars.SetFloat("distance", 5.0f);

    vars.GetFloatVariable("health");
    vars.GetFloatVariable("health");

    CHECK(vars.WasQueried("health"));
    CHECK(vars.QueryCount("health") == 2);

    // Clau per validar el model de consulta sota demanda:
    // una variable que no s'avalua no s'ha de consultar mai.
    CHECK_FALSE(vars.WasQueried("distance"));
    CHECK(vars.TotalQueries() == 2);
}

TEST_CASE("MockVariableUpdater es pot reinicialitzar") {
    MockVariableUpdater vars;
    vars.SetInt("ammo", 3);
    vars.GetIntVariable("ammo");

    vars.Reset();   // esborra el registre, manté els valors

    CHECK(vars.TotalQueries() == 0);
    CHECK(vars.GetIntVariable("ammo") == 3);
}

// =====================================================================
//  MockNode
// =====================================================================

TEST_CASE("MockNode retorna l'estat programat i compta avaluacions") {
    Blackboard bb = MakeBlackboard();
    MockNode node(State::failure);

    CHECK(node.evaluate(bb) == State::failure);
    CHECK(node.evaluate(bb) == State::failure);
    CHECK(node.EvalCount() == 2);
    CHECK(node.WasEvaluated());
}

TEST_CASE("MockNode recorre una seqüència d'estats") {
    Blackboard bb = MakeBlackboard();
    MockNode node(std::vector<State>{ State::running, State::success });

    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::success);
    CHECK(node.evaluate(bb) == State::success);   // manté l'últim
    CHECK(node.EvalCount() == 3);
}

TEST_CASE("MockNode captura el deltaTime del blackboard") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.16f);
    MockNode node;

    node.evaluate(bb);

    CHECK(node.LastDeltaTime() == doctest::Approx(0.16f));
}

TEST_CASE("MakeMock permet inspeccionar el fill després de cedir-ne la propietat") {
    Blackboard bb = MakeBlackboard();

    MockNode* fill = nullptr;
    InverterNode<Blackboard> decorator(MakeMock(State::success, &fill));

    REQUIRE(fill != nullptr);
    CHECK_FALSE(fill->WasEvaluated());

    auto resultat = decorator.evaluate(bb);

    CHECK(fill->EvalCount() == 1);
    CHECK(resultat == State::failure);   // l'inversor nega l'èxit del fill
}