// =====================================================================
//  Tests de la càrrega i deserialització de cervells d'IA. És el punt
//  del sistema on el motor rep dades externes que poden estar mal
//  formades, de manera que l'objectiu principal d'aquests tests és
//  comprovar que cap entrada invàlida provoca un tancament abrupte.
//
//  Nota sobre el valor de retorn de LoadFromFile: retorna false només
//  si no pot llegir el fitxer. Un fitxer llegible però amb contingut
//  incoherent retorna true, i és en construir o executar l'arbre que
//  el problema es manifesta com una degradació controlada.
// =====================================================================

#include <doctest/doctest.h>

#include <fstream>
#include <cstdio>
#include <string>

#include "helpers/BrainBuilder.hpp"
#include "mocks/MockActionHandler.hpp"
#include "mocks/MockVariableUpdater.hpp"
#include "AIEngine.hpp"

using namespace test;
namespace Core = AIEngineData::Core;

namespace {

// Escriu bytes arbitraris en un fitxer temporal, per poder provar
// entrades que el BrainBuilder no pot generar (buides o corruptes).
class RawFile {
public:
    RawFile(const std::string& nom, const std::string& contingut) {
        m_path = std::string(FIXTURES_DIR) + "/" + nom;
        std::ofstream out(m_path, std::ios::binary);
        out.write(contingut.data(), static_cast<std::streamsize>(contingut.size()));
    }
    ~RawFile() { std::remove(m_path.c_str()); }

    RawFile(const RawFile&) = delete;
    RawFile& operator=(const RawFile&) = delete;

    const std::string& Path() const { return m_path; }

private:
    std::string m_path;
};

} // namespace


// =====================================================================
//  Fitxers que no es poden llegir
// =====================================================================

TEST_CASE("Carregar un fitxer inexistent retorna false") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    // Cobreix el flux alternatiu 2a del cas d'ús CU-05
    CHECK_FALSE(engine.LoadFromFile("aquest_fitxer_no_existeix.bin"));
}

TEST_CASE("Carregar una ruta buida retorna false") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(""));
}

TEST_CASE("Un motor sense cervell carregat no fa res en actualitzar-se") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    // Update() sobre un motor sense arbre ni FSM no ha de petar
    engine.Update(0.016f);
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Actualitzar el motor després d'una càrrega fallida no provoca cap crida") {
    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE_FALSE(engine.LoadFromFile("no_existeix.bin"));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

// =====================================================================
//  Fitxers llegibles amb contingut invàlid
// =====================================================================

TEST_CASE("Un fitxer buit no provoca cap crida en actualitzar-se") {
    RawFile buit("tmp_buit.bin", "");

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(buit.Path()));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);

}

// =====================================================================
//  Cervells estructuralment incoherents
// =====================================================================

TEST_CASE("Un cervell sense cap node no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Un arbre amb arrel inexistent no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddAction(1, "Patrol")
        .SetRoot(99)          // no existeix cap node amb id 99
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Una seqüència amb fills inexistents omet els que no troba") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2, 99, 3 })   // el 99 no existeix
        .AddAction(2, "Primera")
        .AddAction(3, "Segona")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    // Els fills vàlids s'executen igualment; el que no es troba
    // simplement no s'afegeix a l'arbre.
    std::vector<std::string> esperat{ "Primera", "Segona" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("Una seqüència sense cap fill vàlid no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 98, 99 })
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

// =====================================================================
//  Decoradors mal formats
// =====================================================================

TEST_CASE("Un decorador sense fill no provoca cap crida") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2 })
        .AddDecorator(2, "Inverter", 99)   // el fill no existeix
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    engine.LoadFromFile(brain.Path());
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Un decorador de tipus desconegut es degrada al seu fill") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddDecorator(1, "DecoradorInventat", 2)
        .AddAction(2, "Patrol")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    // El motor avisa per consola i retorna directament el fill, de
    // manera que el comportament segueix sent funcional.
    CHECK(actions.WasCalled("Patrol"));
}

TEST_CASE("Un decorador parametritzat sense paràmetres fa servir el valor per defecte") {
    // Repeater sense el paràmetre count: el motor n'assumeix 3
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddDecorator(1, "Repeater", 2)
        .AddAction(2, "Patrol")
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

TEST_CASE("El decorador ConditionCheck només executa el fill si la condició es compleix") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddConditionCheck(1, 2, Cond::Bool("ready", Core::Operator_Equal, true))
        .AddAction(2, "Attack")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    vars.SetBool("ready", false);

    AIEngine engine(&actions, &vars);
    REQUIRE(engine.LoadFromFile(brain.Path()));

    engine.Update(0.016f);
    CHECK_FALSE(actions.WasCalled("Attack"));

    vars.SetBool("ready", true);
    engine.Update(0.016f);
    CHECK(actions.WasCalled("Attack"));
}

// =====================================================================
//  Recàrrega
// =====================================================================

TEST_CASE("Carregar un segon cervell substitueix completament l'anterior") {
    auto primer = BrainBuilder()
        .AsBehaviorTree()
        .AddAction(1, "Primer")
        .SetRoot(1)
        .Build();

    auto segon = BrainBuilder()
        .AsBehaviorTree()
        .AddAction(1, "Segon")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(primer.Path()));
    engine.Update(0.016f);

    REQUIRE(engine.LoadFromFile(segon.Path()));
    engine.Update(0.016f);

    std::vector<std::string> esperat{ "Primer", "Segon" };
    CHECK(actions.CallSequence() == esperat);
}

TEST_CASE("Es pot passar d'un arbre de comportament a una màquina d'estats") {
    auto arbre = BrainBuilder()
        .AsBehaviorTree()
        .AddAction(1, "AccioBT")
        .SetRoot(1)
        .Build();

    auto fsm = BrainBuilder()
        .AsStateMachine()
        .AddState(1, "EstatFSM")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(arbre.Path()));
    engine.Update(0.016f);

    REQUIRE(engine.LoadFromFile(fsm.Path()));
    engine.Update(0.016f);

    std::vector<std::string> esperat{ "AccioBT", "EstatFSM" };
    CHECK(actions.CallSequence() == esperat);
}

// =====================================================================
//  Tipus de node no suportats dins d'un arbre
// =====================================================================

TEST_CASE("Un node de tipus State dins d'un arbre no és construïble") {
    // Els estats pertanyen a les FSM; dins d'un BT el motor no els
    // sap construir i el node queda descartat.
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddState(1, "Patrol")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    CHECK(actions.TotalCalls() == 0);
}

TEST_CASE("Un arbre amb un cicle es rebutja en lloc de provocar recursió infinita") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 1 })   // el node es té a si mateix com a fill
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(brain.Path()));
}

TEST_CASE("Un cicle indirecte entre diversos nodes també es detecta") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2 })
        .AddSequence(2, { 1 })   // torna enrere cap al node 1
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    CHECK_FALSE(engine.LoadFromFile(brain.Path()));
}

TEST_CASE("Un node compartit per dues branques no es confon amb un cicle") {
    auto brain = BrainBuilder()
        .AsBehaviorTree()
        .AddSequence(1, { 2, 3 })
        .AddSequence(2, { 4 })
        .AddSequence(3, { 4 })   // totes dues branques apunten al node 4
        .AddAction(4, "Compartida")
        .SetRoot(1)
        .Build();

    MockActionHandler actions;
    MockVariableUpdater vars;
    AIEngine engine(&actions, &vars);

    REQUIRE(engine.LoadFromFile(brain.Path()));
    engine.Update(0.016f);

    // El node 4 apareix a dues branques, així que s'executa dues vegades
    CHECK(actions.CallCount("Compartida") == 2);
}