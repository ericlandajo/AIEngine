// =====================================================================
//  Tests unitaris del ParallelNode. A diferència dels nodes seqüencials
//  (seq/sel), aquest avalua TOTS els seus fills a cada tick i decideix
//  el resultat comptant quants han reeixit i quants han fracassat,
//  comparant-ho amb dos llindars configurables.
//
//  Recordatori de la lògica:
//      failures >= failure_threshold  -> failure   (es comprova primer)
//      successes >= success_threshold -> success
//      altrament                      -> running
// =====================================================================

#include <doctest/doctest.h>

#include "mocks/MockNode.hpp"
#include "ParallelNode.hpp"

using State = aitoolkit::bt::execution_state;

// =====================================================================
//  Llindar d'èxit
// =====================================================================

TEST_CASE("ParallelNode reïx en arribar exactament al llindar d'èxit") {
    Blackboard bb = MakeBlackboard();
    // Cal 1 èxit per reeixir; calen 3 fracassos per fracassar
    ParallelNode<Blackboard> node(1, 3, MakeChildren(
        MakeMock(State::success),
        MakeMock(State::running),
        MakeMock(State::running)
    ));

    CHECK(node.evaluate(bb) == State::success);
}

TEST_CASE("ParallelNode reïx si supera el llindar d'èxit") {
    Blackboard bb = MakeBlackboard();
    // Cal 2 èxits; n'hi ha 3
    ParallelNode<Blackboard> node(2, 4, MakeChildren(
        MakeMock(State::success),
        MakeMock(State::success),
        MakeMock(State::success)
    ));

    CHECK(node.evaluate(bb) == State::success);
}

TEST_CASE("ParallelNode segueix en marxa si no arriba al llindar d'èxit") {
    Blackboard bb = MakeBlackboard();
    // Calen 2 èxits, però només n'hi ha 1 i cap fracàs suficient
    ParallelNode<Blackboard> node(2, 3, MakeChildren(
        MakeMock(State::success),
        MakeMock(State::running),
        MakeMock(State::running)
    ));

    CHECK(node.evaluate(bb) == State::running);
}

// =====================================================================
//  Llindar de fracàs
// =====================================================================

TEST_CASE("ParallelNode fracassa en arribar exactament al llindar de fracàs") {
    Blackboard bb = MakeBlackboard();
    // Calen 2 fracassos per fracassar; n'hi ha exactament 2
    ParallelNode<Blackboard> node(3, 2, MakeChildren(
        MakeMock(State::failure),
        MakeMock(State::failure),
        MakeMock(State::running)
    ));

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("ParallelNode segueix en marxa si no arriba al llindar de fracàs") {
    Blackboard bb = MakeBlackboard();
    // Calen 3 fracassos, només n'hi ha 2
    ParallelNode<Blackboard> node(3, 3, MakeChildren(
        MakeMock(State::failure),
        MakeMock(State::failure),
        MakeMock(State::running)
    ));

    CHECK(node.evaluate(bb) == State::running);
}

TEST_CASE("El llindar de fracàs té prioritat sobre el d'èxit") {
    Blackboard bb = MakeBlackboard();
    // Es compleixen els dos llindars alhora (1 èxit i 2 fracassos).
    // La comprovació de fracàs va primer, així que guanya.
    ParallelNode<Blackboard> node(1, 2, MakeChildren(
        MakeMock(State::success),
        MakeMock(State::failure),
        MakeMock(State::failure)
    ));

    CHECK(node.evaluate(bb) == State::failure);
}

// =====================================================================
//  Avaluació de tots els fills
// =====================================================================

TEST_CASE("ParallelNode avalua tots els fills, també després d'assolir el llindar") {
    Blackboard bb = MakeBlackboard();
    MockNode* primer = nullptr;
    MockNode* segon = nullptr;
    MockNode* tercer = nullptr;

    // Amb un sol èxit ja n'hi hauria prou, però el node no s'atura:
    // aquesta és la diferència essencial respecte a seq/sel.
    ParallelNode<Blackboard> node(1, 3, MakeChildren(
        MakeMock(State::success, &primer),
        MakeMock(State::running, &segon),
        MakeMock(State::running, &tercer)
    ));

    node.evaluate(bb);

    CHECK(primer->EvalCount() == 1);
    CHECK(segon->EvalCount() == 1);
    CHECK(tercer->EvalCount() == 1);
}

TEST_CASE("ParallelNode avalua tots els fills encara que el primer fracassi") {
    Blackboard bb = MakeBlackboard();
    MockNode* primer = nullptr;
    MockNode* segon = nullptr;

    ParallelNode<Blackboard> node(1, 1, MakeChildren(
        MakeMock(State::failure, &primer),
        MakeMock(State::success, &segon)
    ));

    node.evaluate(bb);

    // Una seqüència s'hauria aturat al primer fracàs; aquest node no.
    CHECK(primer->EvalCount() == 1);
    CHECK(segon->EvalCount() == 1);
}

// =====================================================================
//  Estat running dels fills
// =====================================================================

TEST_CASE("ParallelNode segueix en marxa si tots els fills estan en marxa") {
    Blackboard bb = MakeBlackboard();
    ParallelNode<Blackboard> node(1, 2, MakeChildren(
        MakeMock(State::running),
        MakeMock(State::running)
    ));

    // Els fills en marxa no compten ni com a èxit ni com a fracàs
    CHECK(node.evaluate(bb) == State::running);
}

TEST_CASE("ParallelNode acumula resultats a mesura que els fills acaben") {
    Blackboard bb = MakeBlackboard();
    // Calen 2 èxits. El primer fill reïx al segon tick, el segon al tercer.
    ParallelNode<Blackboard> node(2, 3, MakeChildren(
        MakeMock({ State::running, State::success }),
        MakeMock({ State::running, State::running, State::success })
    ));

    CHECK(node.evaluate(bb) == State::running);   // cap acabat
    CHECK(node.evaluate(bb) == State::running);   // 1 èxit, en calen 2
    CHECK(node.evaluate(bb) == State::success);   // 2 èxits
}

// =====================================================================
//  Configuració equivalent a la que genera actualment el motor
//  (success_threshold = 1, failure_threshold = nombre de fills),
//  que es comporta com un selector concurrent.
// =====================================================================

TEST_CASE("Amb els llindars per defecte, n'hi ha prou amb un èxit") {
    Blackboard bb = MakeBlackboard();
    ParallelNode<Blackboard> node(1, 3, MakeChildren(
        MakeMock(State::failure),
        MakeMock(State::failure),
        MakeMock(State::success)
    ));

    CHECK(node.evaluate(bb) == State::success);
}

TEST_CASE("Amb els llindars per defecte, cal que fracassin tots per fracassar") {
    Blackboard bb = MakeBlackboard();
    ParallelNode<Blackboard> node(1, 3, MakeChildren(
        MakeMock(State::failure),
        MakeMock(State::failure),
        MakeMock(State::failure)
    ));

    CHECK(node.evaluate(bb) == State::failure);
}

// =====================================================================
//  Robustesa
// =====================================================================

TEST_CASE("ParallelNode sense fills retorna failure") {
    Blackboard bb = MakeBlackboard();
    ParallelNode<Blackboard> node(1, 1,
        std::vector<std::unique_ptr<aitoolkit::bt::node<Blackboard>>>{});

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("ParallelNode amb el primer fill nul retorna failure") {
    Blackboard bb = MakeBlackboard();
    ParallelNode<Blackboard> node(1, 1, MakeChildren(
        std::unique_ptr<aitoolkit::bt::node<Blackboard>>(nullptr),
        MakeMock(State::success)
    ));

    // El node comprova explícitament el primer fill abans de començar
    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("ParallelNode compta com a fracàs els fills nuls intermedis") {
    Blackboard bb = MakeBlackboard();
    MockNode* primer = nullptr;

    // Calen 2 fracassos: el fill nul n'aporta un i el segon fill l'altre
    ParallelNode<Blackboard> node(3, 2, MakeChildren(
        MakeMock(State::failure, &primer),
        std::unique_ptr<aitoolkit::bt::node<Blackboard>>(nullptr)
    ));

    CHECK(node.evaluate(bb) == State::failure);
    CHECK(primer->EvalCount() == 1);
}

// =====================================================================
//  Propagació del blackboard
// =====================================================================

TEST_CASE("ParallelNode propaga el mateix blackboard a tots els fills") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.16f);
    MockNode* primer = nullptr;
    MockNode* segon = nullptr;

    ParallelNode<Blackboard> node(1, 2, MakeChildren(
        MakeMock(State::running, &primer),
        MakeMock(State::running, &segon)
    ));

    node.evaluate(bb);

    CHECK(primer->LastDeltaTime() == doctest::Approx(0.16f));
    CHECK(segon->LastDeltaTime() == doctest::Approx(0.16f));
}