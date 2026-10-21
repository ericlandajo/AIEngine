// =====================================================================
//  Proves de compatibilitat amb els fitxers que genera realment
//  l'editor visual. La resta de tests fan servir cervells construïts
//  pel BrainBuilder, que és codi de proves: aquests fixtures verifiquen
//  que el format que produeix l'eina real és el que el motor espera.
// =====================================================================

#include <doctest/doctest.h>

#include <string>

#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "AIEngine.hpp"

using State = aitoolkit::bt::execution_state;

namespace {
    std::string Fixture(const std::string& nom) {
        return std::string(FIXTURES_DIR) + "/" + nom;
    }
}

// =====================================================================
//  editor_bt_basic.bin
//  Cas realista: enemic amb selector, seqüències, condicions i un
//  decorador Inverter.
// =====================================================================

TEST_CASE("El motor carrega un arbre exportat per l'editor") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK(engine.LoadFromFile(Fixture("editor_bt_basic.bin")));
}

TEST_CASE("L'Inverter del fixture bàsic s'aplica realment") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetBool("isTired", false);   // l'Inverter ha de convertir-ho en cert

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_basic.bin")));
    engine.Update(0.016f);

    CHECK(actions.WasCalled("Patrol"));
}

// =====================================================================
//  editor_bt_decorators.bin
//
//      Parallel (arrel)
//        ├── Inverter          -> ActInverter
//        ├── ForceSuccess      -> ActForceSuccess
//        ├── ForceFailure      -> ActForceFailure
//        ├── SuccessIsFailure  -> ActSuccessIsFailure
//        ├── Repeater(2)       -> ActRepeater
//        ├── RetryUntilSuccess -> ActRetryUntilSuccess
//        ├── Cooldown(1.0)     -> ActCooldown
//        └── Timeout(1.0)      -> ActTimeout
// =====================================================================

TEST_CASE("El fixture de decoradors es carrega correctament") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK(engine.LoadFromFile(Fixture("editor_bt_decorators.bin")));
}

TEST_CASE("Tots els decoradors de l'editor arriben a executar el seu fill") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_decorators.bin")));
    engine.Update(0.016f);

    // Un Parallel avalua totes les branques, de manera que en un sol
    // tick s'han d'haver executat les vuit accions. Si alguna no hi és,
    // el nom d'aquell decorador no coincideix entre l'editor i el motor.
    CHECK(actions.WasCalled("ActInverter"));
    CHECK(actions.WasCalled("ActForceSuccess"));
    CHECK(actions.WasCalled("ActForceFailure"));
    CHECK(actions.WasCalled("ActSuccessIsFailure"));
    CHECK(actions.WasCalled("ActRepeater"));
    CHECK(actions.WasCalled("ActRetryUntilSuccess"));
    CHECK(actions.WasCalled("ActCooldown"));
    CHECK(actions.WasCalled("ActTimeout"));
}

TEST_CASE("El Cooldown exportat per l'editor bloqueja el fill al tick següent") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_decorators.bin")));

    engine.Update(0.016f);   // primera execució: activa el cooldown
    engine.Update(0.016f);   // encara dins del cooldown d'1 segon

    // Aquesta comprovació és la que demostra que el decorador s'aplica
    // de debò: si el motor l'hagués descartat i hagués deixat passar
    // només el fill, l'acció s'hauria executat les dues vegades.
    CHECK(actions.CallCount("ActCooldown") == 1);
    // Una branca sense restriccions temporals sí que s'executa cada tick
    CHECK(actions.CallCount("ActInverter") == 2);
}

TEST_CASE("El Repeater exportat per l'editor repeteix el nombre de cops configurat") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_decorators.bin")));

    engine.Update(0.016f);
    engine.Update(0.016f);

    // Amb count = 2, el fill s'executa un cop per tick fins a completar
    // el cicle; el que es verifica aquí és que el paràmetre ha viatjat
    // correctament des de l'editor.
    CHECK(actions.CallCount("ActRepeater") == 2);
}

// =====================================================================
//  editor_bt_params.bin
//
//      Action: TestParams
//        speed     Float   3.5
//        priority  Int     7
//        isUrgent  Bool    true
//        target    String  "PlayerBase"
// =====================================================================

TEST_CASE("Els paràmetres dels quatre tipus arriben intactes des de l'editor") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_params.bin")));
    engine.Update(0.016f);

    REQUIRE(actions.WasCalled("TestParams"));
    auto params = actions.LastParamsOf("TestParams");

    REQUIRE(params.count("speed") == 1);
    CHECK(params["speed"].float_val == doctest::Approx(3.5f));

    REQUIRE(params.count("priority") == 1);
    CHECK(params["priority"].int_val == 7);

    REQUIRE(params.count("isUrgent") == 1);
    CHECK(params["isUrgent"].bool_val == true);

    // El tipus String és el més sensible del conjunt, perquè implica
    // un offset a una cadena dins del buffer.
    REQUIRE(params.count("target") == 1);
    CHECK(params["target"].string_val == "PlayerBase");
}

// =====================================================================
//  editor_bt_conditions.bin
//
//      Parallel (arrel)
//        ├── Sequence -> Condition intVar    >= 5      -> OkInt
//        ├── Sequence -> Condition floatVar  <= 20.0   -> OkFloat
//        ├── Sequence -> Condition boolVar   == true   -> OkBool
//        └── Sequence -> Condition stringVar != "xyz"  -> OkString
// =====================================================================

TEST_CASE("El fixture de condicions executa totes les branques quan es compleixen") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetInt("intVar", 10);
    vars.SetFloat("floatVar", 5.0f);
    vars.SetBool("boolVar", true);
    vars.SetString("stringVar", "abc");

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_conditions.bin")));
    engine.Update(0.016f);

    CHECK(actions.WasCalled("OkInt"));
    CHECK(actions.WasCalled("OkFloat"));
    CHECK(actions.WasCalled("OkBool"));
    CHECK(actions.WasCalled("OkString"));
}

TEST_CASE("Els operadors exportats per l'editor es corresponen amb els seleccionats") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    // Valors triats just per fora de cada condició
    vars.SetInt("intVar", 4);          // 4 >= 5 és fals
    vars.SetFloat("floatVar", 25.0f);  // 25 <= 20 és fals
    vars.SetBool("boolVar", false);    // false == true és fals
    vars.SetString("stringVar", "xyz");// "xyz" != "xyz" és fals

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_conditions.bin")));
    engine.Update(0.016f);

    // Si l'editor hagués exportat un operador diferent del seleccionat,
    // alguna d'aquestes branques s'executaria igualment.
    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Els operadors amb igualtat inclouen el valor límit") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetInt("intVar", 5);          // 5 >= 5 és cert
    vars.SetFloat("floatVar", 20.0f);  // 20 <= 20 és cert
    vars.SetBool("boolVar", false);
    vars.SetString("stringVar", "xyz");

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_conditions.bin")));
    engine.Update(0.016f);

    // Distingeix GreaterOrEqual de Greater i LessOrEqual de Less
    CHECK(actions.WasCalled("OkInt"));
    CHECK(actions.WasCalled("OkFloat"));
    CHECK_FALSE(actions.WasCalled("OkBool"));
    CHECK_FALSE(actions.WasCalled("OkString"));
}

// =====================================================================
//  editor_bt_conditioncheck.bin
//
//      ConditionCheck (ready == true)
//        └── Action: Attack
// =====================================================================

TEST_CASE("El ConditionCheck bloqueja el fill si la condició no es compleix") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("ready", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_conditioncheck.bin")));
    engine.Update(0.016f);

    CHECK_FALSE(actions.WasCalled("Attack"));
}

TEST_CASE("El ConditionCheck deixa passar el fill si la condició es compleix") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("ready", true);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_bt_conditioncheck.bin")));
    engine.Update(0.016f);

    CHECK(actions.WasCalled("Attack"));
}

// =====================================================================
//  editor_fsm_complete.bin
//
//      Estats:  Patrol (inicial, speed 2.0)
//               Chase  (speed 5.0)
//               Flee   (speed 7.0)
//
//      Patrol -> Chase   si  playerVisible == true
//      Chase  -> Flee    si  health < 20
//      Chase  -> Patrol  si  playerVisible == false
//      Flee   -> Patrol  si  health > 50
// =====================================================================

TEST_CASE("El motor carrega una màquina d'estats exportada per l'editor") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK(engine.LoadFromFile(Fixture("editor_fsm_complete.bin")));
}

TEST_CASE("La FSM de l'editor comença a l'estat marcat com a inicial") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetInt("health", 100);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_fsm_complete.bin")));
    engine.Update(0.016f);

    CHECK(actions.WasCalled("Patrol"));
    CHECK_FALSE(actions.WasCalled("Chase"));
    CHECK_FALSE(actions.WasCalled("Flee"));
}

TEST_CASE("La FSM de l'editor recorre el cicle complet d'estats") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetInt("health", 100);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_fsm_complete.bin")));

    engine.Update(0.016f);                    // Patrol

    vars.SetBool("playerVisible", true);
    engine.Update(0.016f);                    // Patrol -> Chase

    // Es manté playerVisible a cert perquè només una de les dues
    // transicions que surten de Chase pugui complir-se, i el resultat
    // no depengui de l'ordre en què s'hagin declarat les arestes.
    vars.SetInt("health", 15);
    engine.Update(0.016f);                    // Chase -> Flee

    vars.SetInt("health", 60);
    engine.Update(0.016f);                    // Flee -> Patrol

    std::vector<std::string> esperat{ "Patrol", "Chase", "Flee", "Patrol" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("La FSM de l'editor pot tornar de Chase a Patrol sense passar per Flee") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", true);
    vars.SetInt("health", 100);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_fsm_complete.bin")));

    engine.Update(0.016f);                    // ja transiciona a Chase

    vars.SetBool("playerVisible", false);
    engine.Update(0.016f);                    // Chase -> Patrol

    std::vector<std::string> esperat{ "Chase", "Patrol" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("Els paràmetres dels estats de la FSM arriben al handler") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetInt("health", 100);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_fsm_complete.bin")));

    engine.Update(0.016f);                    // Patrol
    auto patrolParams = actions.LastParamsOf("Patrol");
    REQUIRE(patrolParams.count("speed") == 1);
    CHECK(patrolParams["speed"].float_val == doctest::Approx(2.0f));

    vars.SetBool("playerVisible", true);
    engine.Update(0.016f);                    // Chase
    auto chaseParams = actions.LastParamsOf("Chase");
    REQUIRE(chaseParams.count("speed") == 1);
    CHECK(chaseParams["speed"].float_val == doctest::Approx(5.0f));
}

TEST_CASE("La FSM de l'editor només consulta les variables de l'estat actual") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("playerVisible", false);
    vars.SetInt("health", 10);   // provocaria Chase -> Flee, però no hi som

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(Fixture("editor_fsm_complete.bin")));
    engine.Update(0.016f);

    CHECK(vars.WasQueried("playerVisible"));
    // Des de Patrol, cap transició fa servir health
    CHECK_FALSE(vars.WasQueried("health"));
}