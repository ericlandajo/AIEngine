// =====================================================================
//  Tests unitaris dels decoradors. Cada decorador s'avalua de manera
//  aïllada amb un MockNode com a fill, de manera que el resultat depèn
//  únicament de la lògica del decorador i no del comportament d'un
//  node real ni de cap fitxer .bin.
// =====================================================================

#include <doctest/doctest.h>

#include "mocks/MockNode.hpp"
#include "DecoratorNode.hpp"

using State = aitoolkit::bt::execution_state;

// =====================================================================
//  InverterNode
//  Inverteix èxit i fracàs; deixa passar running sense modificar-lo.
// =====================================================================

TEST_CASE("InverterNode converteix l'èxit del fill en fracàs") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    InverterNode<Blackboard> node(MakeMock(State::success, &fill));

    CHECK(node.evaluate(bb) == State::failure);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("InverterNode converteix el fracàs del fill en èxit") {
    Blackboard bb = MakeBlackboard();
    InverterNode<Blackboard> node(MakeMock(State::failure));

    CHECK(node.evaluate(bb) == State::success);
}

TEST_CASE("InverterNode propaga running sense invertir-lo") {
    Blackboard bb = MakeBlackboard();
    InverterNode<Blackboard> node(MakeMock(State::running));

    // Una acció en curs no és ni èxit ni fracàs, de manera que no té
    // sentit invertir-la: ha de seguir informant que està en marxa.
    CHECK(node.evaluate(bb) == State::running);
}

// =====================================================================
//  ForceSuccessNode
//  Converteix qualsevol resultat acabat en èxit, però respecta running.
// =====================================================================

TEST_CASE("ForceSuccessNode manté l'èxit del fill") {
    Blackboard bb = MakeBlackboard();
    ForceSuccessNode<Blackboard> node(MakeMock(State::success));

    CHECK(node.evaluate(bb) == State::success);
}

TEST_CASE("ForceSuccessNode converteix el fracàs del fill en èxit") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    ForceSuccessNode<Blackboard> node(MakeMock(State::failure, &fill));

    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("ForceSuccessNode propaga running") {
    Blackboard bb = MakeBlackboard();
    ForceSuccessNode<Blackboard> node(MakeMock(State::running));

    CHECK(node.evaluate(bb) == State::running);
}

// =====================================================================
//  ForceFailureNode
//  Simètric a l'anterior: qualsevol resultat acabat esdevé fracàs.
// =====================================================================

TEST_CASE("ForceFailureNode converteix l'èxit del fill en fracàs") {
    Blackboard bb = MakeBlackboard();
    ForceFailureNode<Blackboard> node(MakeMock(State::success));

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("ForceFailureNode manté el fracàs del fill") {
    Blackboard bb = MakeBlackboard();
    ForceFailureNode<Blackboard> node(MakeMock(State::failure));

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("ForceFailureNode propaga running") {
    Blackboard bb = MakeBlackboard();
    ForceFailureNode<Blackboard> node(MakeMock(State::running));

    CHECK(node.evaluate(bb) == State::running);
}

// =====================================================================
//  SuccessIsFailureNode
//  Només transforma l'èxit en fracàs; la resta d'estats passen tal qual.
//  Compte: NO és equivalent a l'InverterNode, perquè un fracàs del fill
//  segueix sent un fracàs (l'inversor el convertiria en èxit).
// =====================================================================

TEST_CASE("SuccessIsFailureNode converteix l'èxit del fill en fracàs") {
    Blackboard bb = MakeBlackboard();
    SuccessIsFailureNode<Blackboard> node(MakeMock(State::success));

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("SuccessIsFailureNode manté el fracàs del fill, a diferència de l'Inverter") {
    Blackboard bb = MakeBlackboard();
    SuccessIsFailureNode<Blackboard> node(MakeMock(State::failure));

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("SuccessIsFailureNode propaga running") {
    Blackboard bb = MakeBlackboard();
    SuccessIsFailureNode<Blackboard> node(MakeMock(State::running));

    CHECK(node.evaluate(bb) == State::running);
}


// =====================================================================
//  RepeaterNode
// =====================================================================

TEST_CASE("RepeaterNode amb count 1 executa el fill una sola vegada") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(1, MakeMock(State::success, &fill));

    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("RepeaterNode amb count N executa el fill N vegades") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(3, MakeMock(State::success, &fill));

    // Mentre no s'ha completat el cicle, el node informa que segueix en marxa
    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::success);   // 3a iteració: acabat
    CHECK(fill->EvalCount() == 3);
}

TEST_CASE("RepeaterNode compta també les iteracions que fracassen") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(2, MakeMock(State::failure, &fill));

    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("RepeaterNode no compta les iteracions mentre el fill està en curs") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(2, MakeMock(State::running, &fill));

    // Una acció encara en marxa no tanca cap iteració
    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::running);
    CHECK(fill->EvalCount() == 3);
}

TEST_CASE("RepeaterNode es reinicia després de completar el cicle") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(2, MakeMock(State::success, &fill));

    node.evaluate(bb);
    CHECK(node.evaluate(bb) == State::success);   // fi del primer cicle

    // El comptador s'ha reiniciat, així que el segon cicle és idèntic
    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 4);
}

TEST_CASE("RepeaterNode amb count 0 no executa el fill cap vegada") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(0, MakeMock(State::success, &fill));

    // Zero iteracions vol dir zero execucions, igual que un bucle buit
    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 0);
}

TEST_CASE("RepeaterNode amb count negatiu es comporta com amb count 0") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RepeaterNode<Blackboard> node(-1, MakeMock(State::success, &fill));

    // L'editor impedeix valors < 1, però el motor ha de ser robust
    // davant d'un .bin generat per una altra eina o manipulat a mà.
    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 0);
}

// =====================================================================
//  RetryUntilSuccessNode
// =====================================================================

TEST_CASE("RetryUntilSuccessNode té èxit immediatament si el fill té èxit") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RetryUntilSuccessNode<Blackboard> node(MakeMock(State::success, &fill));

    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("RetryUntilSuccessNode segueix intentant-ho mentre el fill fracassa") {
    Blackboard bb = MakeBlackboard();
    MockNode* fill = nullptr;
    RetryUntilSuccessNode<Blackboard> node(
        MakeMock({ State::failure, State::failure, State::success }, &fill));

    CHECK(node.evaluate(bb) == State::running);   // 1r intent: fracassa
    CHECK(node.evaluate(bb) == State::running);   // 2n intent: fracassa
    CHECK(node.evaluate(bb) == State::success);   // 3r intent: té èxit
    CHECK(fill->EvalCount() == 3);
}

TEST_CASE("RetryUntilSuccessNode tracta el running del fill com un intent no reeixit") {
    Blackboard bb = MakeBlackboard();
    RetryUntilSuccessNode<Blackboard> node(MakeMock(State::running));

    CHECK(node.evaluate(bb) == State::running);
}

// =====================================================================
//  CooldownNode
// =====================================================================

TEST_CASE("CooldownNode avalua el fill la primera vegada") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.0f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(1.0f, MakeMock(State::success, &fill));

    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("CooldownNode bloqueja el fill mentre dura el cooldown") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.4f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(1.0f, MakeMock(State::success, &fill));

    node.evaluate(bb);                          // activa el cooldown
    CHECK(node.evaluate(bb) == State::failure); // 0.4 acumulats
    CHECK(node.evaluate(bb) == State::failure); // 0.8 acumulats
    CHECK(fill->EvalCount() == 1);              // el fill no s'ha tornat a avaluar
}

TEST_CASE("CooldownNode torna a executar el fill quan expira el temps") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.4f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(1.0f, MakeMock(State::success, &fill));

    node.evaluate(bb);   // activa el cooldown
    node.evaluate(bb);   // 0.4
    node.evaluate(bb);   // 0.8
    CHECK(node.evaluate(bb) == State::success);  // 1.2 >= 1.0 → allibera
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("CooldownNode allibera el fill just al llindar exacte") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.5f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(1.0f, MakeMock(State::success, &fill));

    node.evaluate(bb);   // activa el cooldown
    node.evaluate(bb);   // 0.5 acumulats
    // Amb exactament 1.0 acumulats, la comparació és >=, de manera que
    // el cooldown ja es considera exhaurit en aquest mateix tick.
    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("CooldownNode no s'activa si el fill no té èxit") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.4f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(1.0f, MakeMock(State::failure, &fill));

    CHECK(node.evaluate(bb) == State::failure);
    CHECK(node.evaluate(bb) == State::failure);
    // Sense èxit no hi ha cooldown, així que el fill s'avalua cada cop.
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("CooldownNode amb temps 0 no bloqueja mai el fill") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.0f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(0.0f, MakeMock(State::success, &fill));

    CHECK(node.evaluate(bb) == State::success);
    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("CooldownNode no s'activa si el fill encara està en marxa") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.4f);
    MockNode* fill = nullptr;
    CooldownNode<Blackboard> node(1.0f, MakeMock(State::running, &fill));

    // El cooldown és conseqüència de l'èxit, no de la simple execució
    CHECK(node.evaluate(bb) == State::running);
    CHECK(node.evaluate(bb) == State::running);
    CHECK(fill->EvalCount() == 2);
}

// =====================================================================
//  TimeoutNode
//
//  A diferència del Cooldown, el temps s'acumula SEMPRE (no només en
//  un estat concret) i es reinicia quan el fill acaba, tant si reïx
//  com si fracassa. Mentre el fill segueix en marxa, el rellotge corre.
// =====================================================================

TEST_CASE("TimeoutNode deixa passar el resultat si el fill acaba a temps") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.1f);
    MockNode* fill = nullptr;
    TimeoutNode<Blackboard> node(1.0f, MakeMock(State::success, &fill));

    CHECK(node.evaluate(bb) == State::success);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("TimeoutNode propaga el fracàs del fill si acaba a temps") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.1f);
    TimeoutNode<Blackboard> node(1.0f, MakeMock(State::failure));

    CHECK(node.evaluate(bb) == State::failure);
}

TEST_CASE("TimeoutNode deixa continuar el fill mentre no s'exhaureix el temps") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.3f);
    MockNode* fill = nullptr;
    TimeoutNode<Blackboard> node(1.0f, MakeMock(State::running, &fill));

    CHECK(node.evaluate(bb) == State::running);   // 0.3 acumulats
    CHECK(node.evaluate(bb) == State::running);   // 0.6 acumulats
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("TimeoutNode interromp el fill en excedir el temps límit") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.4f);
    MockNode* fill = nullptr;
    TimeoutNode<Blackboard> node(1.0f, MakeMock(State::running, &fill));

    node.evaluate(bb);   // 0.4
    node.evaluate(bb);   // 0.8
    // 1.2 >= 1.0: el node fracassa i ja no arriba a avaluar el fill
    CHECK(node.evaluate(bb) == State::failure);
    CHECK(fill->EvalCount() == 2);
}

TEST_CASE("TimeoutNode fracassa just al llindar exacte") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.5f);
    MockNode* fill = nullptr;
    TimeoutNode<Blackboard> node(1.0f, MakeMock(State::running, &fill));

    node.evaluate(bb);   // 0.5 acumulats
    // Amb exactament 1.0 acumulats la comparació és >=, de manera que
    // el temps ja es considera exhaurit en aquest mateix tick.
    CHECK(node.evaluate(bb) == State::failure);
    CHECK(fill->EvalCount() == 1);
}

TEST_CASE("TimeoutNode reinicia el rellotge quan el fill acaba") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.4f);
    MockNode* fill = nullptr;
    TimeoutNode<Blackboard> node(1.0f,
        MakeMock({ State::running, State::success, State::running }, &fill));

    CHECK(node.evaluate(bb) == State::running);   // 0.4
    CHECK(node.evaluate(bb) == State::success);   // 0.8 -> el fill acaba, es reinicia
    // Si el rellotge no s'hagués reiniciat, aquí ja s'hauria superat 1.0
    CHECK(node.evaluate(bb) == State::running);   // 0.4 de nou
    CHECK(fill->EvalCount() == 3);
}

TEST_CASE("TimeoutNode amb límit 0 fracassa sense arribar a executar el fill") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.0f);
    MockNode* fill = nullptr;
    TimeoutNode<Blackboard> node(0.0f, MakeMock(State::success, &fill));

    // L'editor permet introduir 0 en aquest camp (min="0"), i el node
    // resultant no executa mai el seu fill. Es documenta el comportament
    // per deixar constància que és un valor a evitar en el disseny.
    CHECK(node.evaluate(bb) == State::failure);
    CHECK(fill->EvalCount() == 0);
}

// =====================================================================
//  Robustesa: decorador amb fill nul
//
//  Tots els decoradors comproven el fill abans d'avaluar-lo, de manera
//  que un .bin mal format no ha de provocar cap accés a memòria
//  invàlid, sinó una degradació controlada a failure.
// =====================================================================

TEST_CASE("Els decoradors retornen failure si el fill és nul, sense petar") {
    Blackboard bb = MakeBlackboard();

    SUBCASE("InverterNode") {
        InverterNode<Blackboard> node(nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("ForceSuccessNode") {
        ForceSuccessNode<Blackboard> node(nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("ForceFailureNode") {
        ForceFailureNode<Blackboard> node(nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("SuccessIsFailureNode") {
        SuccessIsFailureNode<Blackboard> node(nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("RepeaterNode") {
        RepeaterNode<Blackboard> node(3, nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("RetryUntilSuccessNode") {
        RetryUntilSuccessNode<Blackboard> node(nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("CooldownNode") {
        CooldownNode<Blackboard> node(1.0f, nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
    SUBCASE("TimeoutNode") {
        TimeoutNode<Blackboard> node(1.0f, nullptr);
        CHECK(node.evaluate(bb) == State::failure);
    }
}

// =====================================================================
//  Propagació del deltaTime
// =====================================================================

TEST_CASE("Els decoradors propaguen el blackboard intacte cap al fill") {
    Blackboard bb = MakeBlackboard(nullptr, nullptr, 0.16f);
    MockNode* fill = nullptr;
    InverterNode<Blackboard> node(MakeMock(State::success, &fill));

    node.evaluate(bb);

    CHECK(fill->LastDeltaTime() == doctest::Approx(0.16f));
}