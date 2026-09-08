#pragma once
// =====================================================================
//  Constructor de cervells d'IA per a proves. Genera fitxers .bin amb
//  el mateix esquema FlatBuffers que produeix l'editor visual, però
//  programàticament, de manera que cada test pot fabricar-se el cas
//  exacte que necessita sense dependre de fitxers preparats a mà.
//
//  El fitxer temporal s'esborra automàticament en destruir-se el
//  TempBrain, de manera que els tests no deixen residus al disc.
// =====================================================================

#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <atomic>

#include "flatbuffers/flatbuffers.h"
#include "AIBrainTemplate_generated.h"

namespace test {

namespace Core = AIEngineData::Core;

// ---------------------------------------------------------------------
//  Fitxer .bin temporal amb esborrat automàtic
// ---------------------------------------------------------------------
class TempBrain {
public:
    explicit TempBrain(std::string path) : m_path(std::move(path)) {}

    ~TempBrain() {
        if (!m_path.empty()) std::remove(m_path.c_str());
    }

    // No copiable, sí movible (per poder-lo retornar des de Build())
    TempBrain(const TempBrain&) = delete;
    TempBrain& operator=(const TempBrain&) = delete;
    TempBrain(TempBrain&& other) noexcept : m_path(std::move(other.m_path)) {
        other.m_path.clear();
    }

    const std::string& Path() const { return m_path; }

private:
    std::string m_path;
};


// ---------------------------------------------------------------------
//  Descripció d'una condició, per no haver de repetir paràmetres
// ---------------------------------------------------------------------
struct Cond {
    std::string variable;
    Core::Operator op = Core::Operator_Equal;
    Core::DataType type = Core::DataType_Int;

    int         int_val    = 0;
    float       float_val  = 0.0f;
    bool        bool_val   = false;
    std::string string_val;

    static Cond Int(std::string var, Core::Operator op, int value) {
        Cond c; c.variable = std::move(var); c.op = op;
        c.type = Core::DataType_Int; c.int_val = value; return c;
    }
    static Cond Float(std::string var, Core::Operator op, float value) {
        Cond c; c.variable = std::move(var); c.op = op;
        c.type = Core::DataType_Float; c.float_val = value; return c;
    }
    static Cond Bool(std::string var, Core::Operator op, bool value) {
        Cond c; c.variable = std::move(var); c.op = op;
        c.type = Core::DataType_Bool; c.bool_val = value; return c;
    }
    static Cond String(std::string var, Core::Operator op, std::string value) {
        Cond c; c.variable = std::move(var); c.op = op;
        c.type = Core::DataType_String; c.string_val = std::move(value); return c;
    }
};


// ---------------------------------------------------------------------
//  Paràmetre d'acció
// ---------------------------------------------------------------------
struct Param {
    std::string name;
    Core::DataType type = Core::DataType_Int;
    int         int_val   = 0;
    float       float_val = 0.0f;
    bool        bool_val  = false;
    std::string string_val;

    static Param Int(std::string n, int v) {
        Param p; p.name = std::move(n); p.type = Core::DataType_Int; p.int_val = v; return p;
    }
    static Param Float(std::string n, float v) {
        Param p; p.name = std::move(n); p.type = Core::DataType_Float; p.float_val = v; return p;
    }
    static Param Bool(std::string n, bool v) {
        Param p; p.name = std::move(n); p.type = Core::DataType_Bool; p.bool_val = v; return p;
    }
    static Param String(std::string n, std::string v) {
        Param p; p.name = std::move(n); p.type = Core::DataType_String; p.string_val = std::move(v); return p;
    }
};


// ---------------------------------------------------------------------
//  BrainBuilder
// ---------------------------------------------------------------------
class BrainBuilder {
public:
    // ---- Tipus de cervell -------------------------------------------
    BrainBuilder& AsBehaviorTree() {
        m_type = Core::BrainType_BehaviorTree;
        return *this;
    }
    BrainBuilder& AsStateMachine() {
        m_type = Core::BrainType_FiniteStateMachine;
        return *this;
    }

    BrainBuilder& SetRoot(uint32_t nodeId) {
        m_rootId = nodeId;
        return *this;
    }

    // ---- Nodes -------------------------------------------------------
    BrainBuilder& AddAction(uint32_t id, std::string actionName,
                            std::vector<Param> params = {}) {
        NodeDesc n;
        n.id = id;
        n.type = Core::NodeType_Action;
        n.actionName = std::move(actionName);
        n.params = std::move(params);
        m_nodes.push_back(std::move(n));
        return *this;
    }

    BrainBuilder& AddCondition(uint32_t id, Cond condition) {
        NodeDesc n;
        n.id = id;
        n.type = Core::NodeType_ConditionNode;
        n.condition = std::move(condition);
        n.hasCondition = true;
        m_nodes.push_back(std::move(n));
        return *this;
    }

    BrainBuilder& AddSequence(uint32_t id, std::vector<uint32_t> children) {
        return AddComposite(id, Core::NodeType_Sequence, std::move(children));
    }

    BrainBuilder& AddSelector(uint32_t id, std::vector<uint32_t> children) {
        return AddComposite(id, Core::NodeType_Selector, std::move(children));
    }

    BrainBuilder& AddParallel(uint32_t id, std::vector<uint32_t> children) {
        return AddComposite(id, Core::NodeType_Parallel, std::move(children));
    }

    // El tipus de decorador viatja al camp action_name, tal com fa
    // l'editor real (vegeu l'apartat d'Implementació de la memòria).
    BrainBuilder& AddDecorator(uint32_t id, std::string decoratorType,
                               uint32_t childId,
                               std::vector<Param> params = {}) {
        NodeDesc n;
        n.id = id;
        n.type = Core::NodeType_Decorator;
        n.actionName = std::move(decoratorType);
        n.children = { childId };
        n.params = std::move(params);
        m_nodes.push_back(std::move(n));
        return *this;
    }

    // Decorador de tipus ConditionCheck, que sí que porta condició
    BrainBuilder& AddConditionCheck(uint32_t id, uint32_t childId, Cond condition) {
        NodeDesc n;
        n.id = id;
        n.type = Core::NodeType_Decorator;
        n.actionName = "ConditionCheck";
        n.children = { childId };
        n.condition = std::move(condition);
        n.hasCondition = true;
        m_nodes.push_back(std::move(n));
        return *this;
    }

    // ---- Estats i transicions (FSM) ----------------------------------
    BrainBuilder& AddState(uint32_t id, std::string actionName,
                           std::vector<Param> params = {}) {
        NodeDesc n;
        n.id = id;
        n.type = Core::NodeType_State;
        n.actionName = std::move(actionName);
        n.params = std::move(params);
        m_nodes.push_back(std::move(n));
        return *this;
    }

    BrainBuilder& AddTransition(uint32_t from, uint32_t to, Cond condition) {
        EdgeDesc e;
        e.source = from;
        e.target = to;
        e.condition = std::move(condition);
        e.hasCondition = true;
        m_edges.push_back(std::move(e));
        return *this;
    }

    // Transició sense condició (per provar el cas límit)
    BrainBuilder& AddTransitionWithoutCondition(uint32_t from, uint32_t to) {
        EdgeDesc e;
        e.source = from;
        e.target = to;
        m_edges.push_back(std::move(e));
        return *this;
    }

    // ---- Serialització ------------------------------------------------
    std::vector<uint8_t> BuildBuffer() {
        flatbuffers::FlatBufferBuilder fbb(1024);

        std::vector<flatbuffers::Offset<Core::Node>> nodeOffsets;
        for (const auto& n : m_nodes) {
            auto nameOff = n.actionName.empty()
                ? flatbuffers::Offset<flatbuffers::String>()
                : fbb.CreateString(n.actionName);

            flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Core::Parameter>>> paramsOff = 0;
            if (!n.params.empty()) {
                std::vector<flatbuffers::Offset<Core::Parameter>> po;
                for (const auto& p : n.params) po.push_back(BuildParam(fbb, p));
                paramsOff = fbb.CreateVector(po);
            }

            flatbuffers::Offset<Core::Condition> condOff = 0;
            if (n.hasCondition) condOff = BuildCondition(fbb, n.condition);

            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> childrenOff = 0;
            if (!n.children.empty()) childrenOff = fbb.CreateVector(n.children);

            Core::NodeBuilder nb(fbb);
            nb.add_id(n.id);
            nb.add_type(n.type);
            if (!nameOff.IsNull()) nb.add_action_name(nameOff);
            if (!paramsOff.IsNull()) nb.add_action_params(paramsOff);
            if (!condOff.IsNull()) nb.add_condition(condOff);
            if (!childrenOff.IsNull()) nb.add_children_ids(childrenOff);
            nodeOffsets.push_back(nb.Finish());
        }
        auto nodesVec = fbb.CreateVector(nodeOffsets);

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Core::Edge>>> edgesVec = 0;
        if (!m_edges.empty()) {
            std::vector<flatbuffers::Offset<Core::Edge>> edgeOffsets;
            for (const auto& e : m_edges) {
                flatbuffers::Offset<Core::Condition> condOff = 0;
                if (e.hasCondition) condOff = BuildCondition(fbb, e.condition);

                Core::EdgeBuilder eb(fbb);
                eb.add_source_id(e.source);
                eb.add_target_id(e.target);
                if (!condOff.IsNull()) eb.add_condition(condOff);
                edgeOffsets.push_back(eb.Finish());
            }
            edgesVec = fbb.CreateVector(edgeOffsets);
        }

        Core::AIBrainTemplateBuilder tb(fbb);
        tb.add_type(m_type);
        tb.add_root_node_id(m_rootId);
        tb.add_nodes(nodesVec);
        if (!edgesVec.IsNull()) tb.add_edges(edgesVec);
        auto root = tb.Finish();

        fbb.Finish(root);

        return std::vector<uint8_t>(fbb.GetBufferPointer(),
                                    fbb.GetBufferPointer() + fbb.GetSize());
    }

    // Escriu el cervell a un fitxer temporal i en retorna el gestor.
    TempBrain Build() {
        auto buffer = BuildBuffer();
        std::string path = NextTempPath();

        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
        out.close();

        return TempBrain(path);
    }

private:
    struct NodeDesc {
        uint32_t id = 0;
        Core::NodeType type = Core::NodeType_Action;
        std::string actionName;
        std::vector<Param> params;
        std::vector<uint32_t> children;
        Cond condition;
        bool hasCondition = false;
    };

    struct EdgeDesc {
        uint32_t source = 0;
        uint32_t target = 0;
        Cond condition;
        bool hasCondition = false;
    };

    BrainBuilder& AddComposite(uint32_t id, Core::NodeType type,
                               std::vector<uint32_t> children) {
        NodeDesc n;
        n.id = id;
        n.type = type;
        n.children = std::move(children);
        m_nodes.push_back(std::move(n));
        return *this;
    }

    static flatbuffers::Offset<Core::Parameter>
    BuildParam(flatbuffers::FlatBufferBuilder& fbb, const Param& p) {
        auto nameOff = fbb.CreateString(p.name);
        auto strOff = p.string_val.empty()
            ? flatbuffers::Offset<flatbuffers::String>()
            : fbb.CreateString(p.string_val);

        Core::ParameterBuilder pb(fbb);
        pb.add_name(nameOff);
        pb.add_type(p.type);
        pb.add_int_val(p.int_val);
        pb.add_float_val(p.float_val);
        pb.add_bool_val(p.bool_val);
        if (!strOff.IsNull()) pb.add_string_val(strOff);
        return pb.Finish();
    }

    static flatbuffers::Offset<Core::Condition>
    BuildCondition(flatbuffers::FlatBufferBuilder& fbb, const Cond& c) {
        auto varOff = c.variable.empty()
            ? flatbuffers::Offset<flatbuffers::String>()
            : fbb.CreateString(c.variable);
        auto strOff = c.string_val.empty()
            ? flatbuffers::Offset<flatbuffers::String>()
            : fbb.CreateString(c.string_val);

        Core::ConditionBuilder cb(fbb);
        if (!varOff.IsNull()) cb.add_variable(varOff);
        cb.add_op(c.op);
        cb.add_compare_type(c.type);
        cb.add_int_val(c.int_val);
        cb.add_float_val(c.float_val);
        cb.add_bool_val(c.bool_val);
        if (!strOff.IsNull()) cb.add_string_val(strOff);
        return cb.Finish();
    }

    static std::string NextTempPath() {
        static std::atomic<int> counter{ 0 };
        return std::string(FIXTURES_DIR) + "/tmp_brain_"
             + std::to_string(counter++) + ".bin";
    }

    Core::BrainType m_type = Core::BrainType_BehaviorTree;
    uint32_t m_rootId = 0;
    std::vector<NodeDesc> m_nodes;
    std::vector<EdgeDesc> m_edges;
};

} // namespace test