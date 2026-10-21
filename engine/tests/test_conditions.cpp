// =====================================================================
//  Tests de l'avaluació de condicions. Com que EvaluateCondition és un
//  mètode privat que rep un punter a una taula FlatBuffers, s'exercita
//  indirectament: es construeix un arbre amb un únic node de condició
//  seguit d'una acció, i s'observa amb els mocks si l'acció s'executa.
//
//      Sequence(1)
//        ├── ConditionNode(2)   <- la condició sota prova
//        └── Action(3) "Fet"    <- només s'executa si la condició es compleix
//
//  Com que una seqüència s'atura al primer fracàs, que l'acció "Fet"
//  s'hagi executat equival a dir que la condició s'ha avaluat certa.
// =====================================================================

#include <doctest/doctest.h>

#include "helpers/BrainBuilder.hpp"
#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "AIEngine.hpp"

using namespace test;
namespace Core = AIEngineData::Core;

namespace {

// Munta l'arbre descrit a la capçalera, l'executa un tick i retorna
// si la condició s'ha considerat certa.
bool AvaluaCondicio(Cond condition, MockVariableUpdater& variables) {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2, 3 })
        .AddCondition(2, std::move(condition))
        .AddAction(3, "Fet")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    AIEngine engine(&actions, &variables);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);
    return actions.WasCalled("Fet");
}

} // namespace


// =====================================================================
//  Tipus Int: els sis operadors
// =====================================================================

TEST_CASE("Condicions de tipus Int amb tots els operadors") {
    MockVariableUpdater vars;
    vars.SetInt("ammo", 5);

    SUBCASE("Equal cert")        { CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Equal, 5), vars)); }
    SUBCASE("Equal fals")        { CHECK_FALSE(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Equal, 4), vars)); }
    SUBCASE("NotEqual cert")     { CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_NotEqual, 4), vars)); }
    SUBCASE("NotEqual fals")     { CHECK_FALSE(AvaluaCondicio(Cond::Int("ammo", Core::Operator_NotEqual, 5), vars)); }
    SUBCASE("Greater cert")      { CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Greater, 4), vars)); }
    SUBCASE("Greater fals")      { CHECK_FALSE(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Greater, 5), vars)); }
    SUBCASE("Less cert")         { CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Less, 6), vars)); }
    SUBCASE("Less fals")         { CHECK_FALSE(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Less, 5), vars)); }
}

TEST_CASE("Els operadors amb igualtat inclouen el valor exacte") {
    MockVariableUpdater vars;
    vars.SetInt("ammo", 5);

    // Valor límit: el cas que distingeix Greater de GreaterOrEqual
    SUBCASE("GreaterOrEqual al valor exacte") {
        CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_GreaterOrEqual, 5), vars));
    }
    SUBCASE("GreaterOrEqual per sota") {
        CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_GreaterOrEqual, 4), vars));
    }
    SUBCASE("GreaterOrEqual per sobre") {
        CHECK_FALSE(AvaluaCondicio(Cond::Int("ammo", Core::Operator_GreaterOrEqual, 6), vars));
    }
    SUBCASE("LessOrEqual al valor exacte") {
        CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_LessOrEqual, 5), vars));
    }
    SUBCASE("LessOrEqual per sobre") {
        CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_LessOrEqual, 6), vars));
    }
    SUBCASE("LessOrEqual per sota") {
        CHECK_FALSE(AvaluaCondicio(Cond::Int("ammo", Core::Operator_LessOrEqual, 4), vars));
    }
}

TEST_CASE("Condicions Int amb valors negatius i zero") {
    MockVariableUpdater vars;

    SUBCASE("valor negatiu") {
        vars.SetInt("temperature", -10);
        CHECK(AvaluaCondicio(Cond::Int("temperature", Core::Operator_Less, 0), vars));
    }
    SUBCASE("valor zero") {
        vars.SetInt("ammo", 0);
        CHECK(AvaluaCondicio(Cond::Int("ammo", Core::Operator_Equal, 0), vars));
    }
}

// =====================================================================
//  Tipus Float
// =====================================================================

TEST_CASE("Condicions de tipus Float amb els operadors de comparació") {
    MockVariableUpdater vars;
    vars.SetFloat("health", 15.5f);

    SUBCASE("Greater cert")    { CHECK(AvaluaCondicio(Cond::Float("health", Core::Operator_Greater, 10.0f), vars)); }
    SUBCASE("Greater fals")    { CHECK_FALSE(AvaluaCondicio(Cond::Float("health", Core::Operator_Greater, 20.0f), vars)); }
    SUBCASE("Less cert")       { CHECK(AvaluaCondicio(Cond::Float("health", Core::Operator_Less, 20.0f), vars)); }
    SUBCASE("Less fals")       { CHECK_FALSE(AvaluaCondicio(Cond::Float("health", Core::Operator_Less, 10.0f), vars)); }
    SUBCASE("NotEqual cert")   { CHECK(AvaluaCondicio(Cond::Float("health", Core::Operator_NotEqual, 20.0f), vars)); }
    SUBCASE("GreaterOrEqual al valor exacte") {
        CHECK(AvaluaCondicio(Cond::Float("health", Core::Operator_GreaterOrEqual, 15.5f), vars));
    }
    SUBCASE("LessOrEqual al valor exacte") {
        CHECK(AvaluaCondicio(Cond::Float("health", Core::Operator_LessOrEqual, 15.5f), vars));
    }
}

TEST_CASE("La igualtat de floats és una comparació exacta") {
    MockVariableUpdater vars;

    SUBCASE("valor representable exactament") {
        // 0.5 té representació binària exacta, així que la igualtat funciona
        vars.SetFloat("health", 0.5f);
        CHECK(AvaluaCondicio(Cond::Float("health", Core::Operator_Equal, 0.5f), vars));
    }

    SUBCASE("valor acumulat al llarg de diversos frames") {
        // Un valor que el joc va acumulant frame a frame no arriba mai
        // exactament al total esperat: sumar 0.1f deu vegades dona
        // 1.000000119, no 1.0f. Com que el motor compara amb ==, la
        // condició no es compleix mai. És una limitació inherent a
        // l'aritmètica de coma flotant, no un error del motor, però cal
        // tenir-la present: per a temps acumulat, distàncies o salut és molt 
        // més fiable fer servir GreaterOrEqualo LessOrEqual que no pas Equal.
        float acumulat = 0.0f;
        for (int i = 0; i < 10; ++i) acumulat += 0.1f;

        vars.SetFloat("charge", acumulat);
        CHECK_FALSE(AvaluaCondicio(Cond::Float("charge", Core::Operator_Equal, 1.0f), vars));
    }
}

// =====================================================================
//  Tipus Bool
// =====================================================================

TEST_CASE("Condicions de tipus Bool") {
    MockVariableUpdater vars;
    vars.SetBool("isAlerted", true);

    SUBCASE("Equal true cert")     { CHECK(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_Equal, true), vars)); }
    SUBCASE("Equal false fals")    { CHECK_FALSE(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_Equal, false), vars)); }
    SUBCASE("NotEqual false cert") { CHECK(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_NotEqual, false), vars)); }
    SUBCASE("NotEqual true fals")  { CHECK_FALSE(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_NotEqual, true), vars)); }
}

TEST_CASE("Els operadors d'ordre no s'apliquen a valors booleans") {
    MockVariableUpdater vars;
    vars.SetBool("isAlerted", true);

    // Comparar booleans amb Greater o Less no té sentit semàntic. El
    // motor no ho tracta com un error: retorna simplement fals. Convé
    // saber-ho perquè una condició mal configurada no avisa, només
    // deixa de complir-se sempre.
    SUBCASE("Greater")        { CHECK_FALSE(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_Greater, false), vars)); }
    SUBCASE("Less")           { CHECK_FALSE(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_Less, false), vars)); }
    SUBCASE("GreaterOrEqual") { CHECK_FALSE(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_GreaterOrEqual, true), vars)); }
    SUBCASE("LessOrEqual")    { CHECK_FALSE(AvaluaCondicio(Cond::Bool("isAlerted", Core::Operator_LessOrEqual, true), vars)); }
}

// =====================================================================
//  Tipus String
// =====================================================================

TEST_CASE("Condicions de tipus String") {
    MockVariableUpdater vars;
    vars.SetString("weapon", "sword");

    SUBCASE("Equal cert")     { CHECK(AvaluaCondicio(Cond::String("weapon", Core::Operator_Equal, "sword"), vars)); }
    SUBCASE("Equal fals")     { CHECK_FALSE(AvaluaCondicio(Cond::String("weapon", Core::Operator_Equal, "bow"), vars)); }
    SUBCASE("NotEqual cert")  { CHECK(AvaluaCondicio(Cond::String("weapon", Core::Operator_NotEqual, "bow"), vars)); }
    SUBCASE("NotEqual fals")  { CHECK_FALSE(AvaluaCondicio(Cond::String("weapon", Core::Operator_NotEqual, "sword"), vars)); }
}

TEST_CASE("La comparació de cadenes distingeix majúscules i minúscules") {
    MockVariableUpdater vars;
    vars.SetString("weapon", "Sword");

    CHECK_FALSE(AvaluaCondicio(Cond::String("weapon", Core::Operator_Equal, "sword"), vars));
}

TEST_CASE("Els operadors d'ordre no s'apliquen a cadenes") {
    MockVariableUpdater vars;
    vars.SetString("weapon", "sword");

    SUBCASE("Greater") { CHECK_FALSE(AvaluaCondicio(Cond::String("weapon", Core::Operator_Greater, "axe"), vars)); }
    SUBCASE("Less")    { CHECK_FALSE(AvaluaCondicio(Cond::String("weapon", Core::Operator_Less, "zebra"), vars)); }
}

// =====================================================================
//  Variables no configurades
// =====================================================================

TEST_CASE("Una variable que el joc no ha establert pren el valor per defecte del tipus") {
    MockVariableUpdater vars;   // cap variable configurada

    SUBCASE("Int per defecte és 0") {
        CHECK(AvaluaCondicio(Cond::Int("desconeguda", Core::Operator_Equal, 0), vars));
    }
    SUBCASE("Float per defecte és 0") {
        CHECK(AvaluaCondicio(Cond::Float("desconeguda", Core::Operator_Equal, 0.0f), vars));
    }
    SUBCASE("Bool per defecte és false") {
        CHECK(AvaluaCondicio(Cond::Bool("desconeguda", Core::Operator_Equal, false), vars));
    }
    SUBCASE("String per defecte és buida") {
        CHECK(AvaluaCondicio(Cond::String("desconeguda", Core::Operator_Equal, ""), vars));
    }
}

// =====================================================================
//  Consulta sota demanda
//
//  El motor només ha de consultar les variables que necessita per
//  avaluar l'estat actual, tal com estableix el disseny del sistema.
// =====================================================================

TEST_CASE("El motor consulta la variable de la condició i cap altra") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2, 3 })
        .AddCondition(2, Cond::Float("health", Core::Operator_Less, 20.0f))
        .AddAction(3, "Flee")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetFloat("health", 50.0f);
    vars.SetFloat("distance", 3.0f);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(vars.QueryCount("health") == 1);
    // "distance" existeix al joc però cap condició la fa servir:
    // el motor no l'ha de consultar mai.
    CHECK_FALSE(vars.WasQueried("distance"));
}

TEST_CASE("Una branca que no s'avalua no provoca cap consulta") {
    // Selector: si la primera condició es compleix, la segona branca
    // no s'arriba a avaluar i la seva variable no s'ha de consultar.
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSelector(1, { 2, 4 })
        .AddSequence(2, { 3, 6 })
        .AddCondition(3, Cond::Bool("isAlerted", Core::Operator_Equal, true))
        .AddSequence(4, { 5, 7 })
        .AddCondition(5, Cond::Float("distance", Core::Operator_Less, 5.0f))
        .AddAction(6, "Alert")
        .AddAction(7, "Approach")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("isAlerted", true);
    vars.SetFloat("distance", 1.0f);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);

    CHECK(actions.WasCalled("Alert"));
    CHECK(vars.WasQueried("isAlerted"));
    // La segona branca del selector no s'ha arribat a avaluar
    CHECK_FALSE(vars.WasQueried("distance"));
    CHECK_FALSE(actions.WasCalled("Approach"));
}

// =====================================================================
//  Robustesa
// =====================================================================

TEST_CASE("Una condició sense nom de variable es considera falsa") {
    MockVariableUpdater vars;

    Cond senseVariable = Cond::Int("", Core::Operator_Equal, 0);
    CHECK_FALSE(AvaluaCondicio(std::move(senseVariable), vars));
}