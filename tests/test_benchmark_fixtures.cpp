// =====================================================================
//  tests/test_benchmark_fixtures.cpp
//
//  Valida els cervells emprats a la campanya de mesura de rendiment.
//
//  Aquests fitxers no s'han construït amb l'editor visual, sinó escrits
//  en JSON i convertits amb flatc, de manera que convé comprovar que
//  representen realment el que se suposa. Si un d'ells estigués mal
//  format, el motor no el carregaria, no executaria res i la mesura
//  corresponent donaria un temps artificialment baix que s'interpretaria
//  com un bon resultat.
//
//  A més de verificar que es carreguen, cada test compta les crides que
//  es produeixen en un tick. Aquest recompte és la variable que explica
//  el cost mesurat a la campanya, de manera que deixar-lo fixat aquí fa
//  que els resultats de rendiment siguin interpretables i reproduïbles.
//
//  Els cervells fan servir únicament les accions Patrol i Chase i les
//  variables booleanes playerVisible i isTired, perquè la mateixa classe
//  d'integració serveixi per a tots ells dins de Unity.
// =====================================================================

#include <doctest/doctest.h>

#include <string>

#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "AIEngine.hpp"

namespace {
    std::string Fixture(const std::string& nom) {
        return std::string(FIXTURES_DIR) + "/" + nom;
    }
}

// =====================================================================
//  bench_01_minimal: un únic node d'acció.
//  Estableix el cost mínim: una sola travessia de la frontera.
// =====================================================================

TEST_CASE("bench_01_minimal es carrega i executa una sola acció per tick") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("bench_01_minimal.bin")));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 1);
    CHECK(actions.WasCalled("Patrol"));
    CHECK(vars.TotalQueries() == 0);
}

// =====================================================================
//  bench_02_traversal: cinc decoradors encadenats sobre una acció.
//  Mateix nombre de crides que l'anterior, cinc nodes més de recorregut.
// =====================================================================

TEST_CASE("bench_02_traversal recorre sis nodes i executa una sola acció") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("bench_02_traversal.bin")));
    engine.Update(0.016f);

    // La diferència de cost respecte del cervell mínim s'atribueix
    // únicament al recorregut de l'arbre, no a crides addicionals.
    CHECK(actions.TotalCalls() == 1);
    CHECK(vars.TotalQueries() == 0);
}

TEST_CASE("bench_02_traversal manté el mateix nombre de crides a cada tick") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("bench_02_traversal.bin")));

    for (int i = 0; i < 5; ++i) engine.Update(0.016f);

    // Cap dels decoradors emprats manté estat, de manera que la feina
    // per fotograma és constant i la mesura, estable.
    CHECK(actions.TotalCalls() == 5);
}

// =====================================================================
//  bench_03_conditions: un node paral·lel amb cinc condicions.
//  Aïlla el cost d'una consulta de variable.
// =====================================================================

TEST_CASE("bench_03_conditions consulta cinc variables per tick i no executa accions") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", true);
    vars.SetBool("isTired", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("bench_03_conditions.bin")));
    engine.Update(0.016f);

    // El node paral·lel avalua sempre tots els fills, de manera que el
    // nombre de consultes no depèn dels valors de les variables.
    CHECK(vars.TotalQueries() == 5);
    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("bench_03_conditions manté les cinc consultes encara que canviïn els valors") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetBool("isTired", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("bench_03_conditions.bin")));
    engine.Update(0.016f);

    CHECK(vars.TotalQueries() == 5);
}

// =====================================================================
//  bench_04_actions: un node paral·lel amb cinc accions.
//  Aïlla el cost d'una execució d'acció, per comparar-lo amb el d'una
//  consulta de variable.
// =====================================================================

TEST_CASE("bench_04_actions executa cinc accions per tick i no consulta variables") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("bench_04_actions.bin")));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 5);
    CHECK(vars.TotalQueries() == 0);
    CHECK(actions.CallCount("Patrol") == 3);
    CHECK(actions.CallCount("Chase") == 2);
}

// =====================================================================
//  bench_05_fsm_narrow: màquina d'estats de tres estats i dues arestes.
//  Cas base per comparar el cost d'una FSM amb el d'un arbre.
// =====================================================================

TEST_CASE("bench_05_fsm_narrow avalua una transició i executa l'estat inicial") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("isTired", false);   // cap transició es dispara

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("bench_05_fsm_narrow.bin")));
    engine.Update(0.016f);

    // Des de l'estat inicial només surt una aresta
    CHECK(vars.TotalQueries() == 1);
    CHECK(actions.TotalCalls() == 1);
    CHECK(actions.WasCalled("Patrol"));
}

TEST_CASE("bench_05_fsm_narrow es manté a l'estat inicial durant tota la mesura") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("isTired", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("bench_05_fsm_narrow.bin")));

    for (int i = 0; i < 10; ++i) engine.Update(0.016f);

    // Que l'agent no canviï d'estat és intencionat: garanteix que cada
    // fotograma fa exactament la mateixa feina.
    CHECK(actions.CallCount("Patrol") == 10);
    CHECK(vars.TotalQueries() == 10);
}

// =====================================================================
//  bench_06_fsm_wide: vint estats i vint-i-quatre arestes, cinc de les
//  quals surten de l'estat inicial.
//  Mesura si el recorregut lineal de totes les arestes es nota.
// =====================================================================

TEST_CASE("bench_06_fsm_wide avalua cinc transicions per tick") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("isTired", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("bench_06_fsm_wide.bin")));
    engine.Update(0.016f);

    // Cinc arestes surten de l'estat inicial. La resta del graf es
    // recorre però no genera cap consulta, que és precisament el que
    // permet aïllar el cost del recorregut.
    CHECK(vars.TotalQueries() == 5);
    CHECK(actions.TotalCalls() == 1);
    CHECK(actions.WasCalled("Patrol"));
}

TEST_CASE("bench_06_fsm_wide es manté a l'estat inicial durant tota la mesura") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("isTired", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("bench_06_fsm_wide.bin")));

    for (int i = 0; i < 10; ++i) engine.Update(0.016f);

    CHECK(vars.TotalQueries() == 50);
    CHECK(actions.CallCount("Patrol") == 10);
}

// =====================================================================
//  Comparabilitat entre cervells
//
//  Les conclusions de la campanya es basen a comparar cervells que
//  difereixen en una sola dimensió. Aquests tests deixen constància
//  d'aquestes relacions, de manera que si algú modifica un fixture,
//  la comparació deixa de ser vàlida i es detecta aquí.
// =====================================================================

TEST_CASE("Els cervells 01 i 02 difereixen només en el recorregut") {
    MockActionHandler accionsMinimal, accionsTraversal;
    MockVariableUpdater varsMinimal, varsTraversal;

    AIEngine minimal(&accionsMinimal, &varsMinimal);
    AIEngine traversal(&accionsTraversal, &varsTraversal);

    REQUIRE(minimal.LoadFromFile(Fixture("bench_01_minimal.bin")));
    REQUIRE(traversal.LoadFromFile(Fixture("bench_02_traversal.bin")));

    minimal.Update(0.016f);
    traversal.Update(0.016f);

    // Mateix nombre de crides en tots dos: la diferència de temps
    // mesurada correspon únicament als nodes intermedis.
    CHECK(accionsMinimal.TotalCalls() == accionsTraversal.TotalCalls());
    CHECK(varsMinimal.TotalQueries() == varsTraversal.TotalQueries());
}

TEST_CASE("Els cervells 03 i 04 tenen el mateix nombre de crides de tipus diferent") {
    MockActionHandler accionsCond, accionsAcc;
    MockVariableUpdater varsCond, varsAcc;

    AIEngine conditions(&accionsCond, &varsCond);
    AIEngine accions(&accionsAcc, &varsAcc);

    REQUIRE(conditions.LoadFromFile(Fixture("bench_03_conditions.bin")));
    REQUIRE(accions.LoadFromFile(Fixture("bench_04_actions.bin")));

    conditions.Update(0.016f);
    accions.Update(0.016f);

    // Cinc travessies de la frontera en tots dos casos, però en un són
    // consultes de variable i en l'altre execucions d'acció.
    CHECK(varsCond.TotalQueries() == 5);
    CHECK(accionsCond.TotalCalls() == 0);

    CHECK(accionsAcc.TotalCalls() == 5);
    CHECK(varsAcc.TotalQueries() == 0);
}