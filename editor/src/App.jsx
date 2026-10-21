import React, { useCallback, useState } from 'react';
import {
  ReactFlow,
  useNodesState,
  useEdgesState,
  addEdge,
  Background,
  Controls,
  Handle,
  Position,
  BaseEdge,
  getSmoothStepPath,
  useReactFlow,
  ReactFlowProvider,
  MarkerType,
  Panel
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';

import * as flatbuffers from 'flatbuffers';
import { DataType } from './aiengine-data/core/data-type';
import { Operator } from './aiengine-data/core/operator';
import { NodeType } from './aiengine-data/core/node-type';
import { BrainType } from './aiengine-data/core/brain-type';
import { Parameter } from './aiengine-data/core/parameter';
import { Condition } from './aiengine-data/core/condition';
import { Node } from './aiengine-data/core/node';
import { Edge } from './aiengine-data/core/edge';
import { AIBrainTemplate } from './aiengine-data/core/aibrain-template';
import JSZip from 'jszip';

const inputStyle = { width: '100%', padding: '6px', borderRadius: '4px', border: '1px solid #ccc', boxSizing: 'border-box', fontSize: '13px' };
const labelStyle = { fontSize: '12px', fontWeight: 'bold', color: '#444', display: 'block', marginTop: '10px', marginBottom: '5px' };
const buttonStyle = (bg) => ({ padding: '8px 16px', cursor: 'pointer', backgroundColor: bg, color: 'white', border: 'none', borderRadius: '6px', fontFamily: 'inherit', fontSize: '14px', fontWeight: 600 });

const toolbarButton = (variant) => {
  const variants = {
    primary:   { backgroundColor: '#aa3bff', color: 'white', border: '1px solid #aa3bff' },
    secondary: { backgroundColor: 'white',   color: '#333',  border: '1px solid #ccc' },
    neutral:   { backgroundColor: '#4a5568', color: 'white', border: '1px solid #4a5568' },
    primaryOutline: { backgroundColor: 'white', color: '#8a1fe0', border: '1.5px solid #aa3bff' },
  };
  return {
    display: 'inline-flex', alignItems: 'center', height: '36px', padding: '0 14px',
    boxSizing: 'border-box', borderRadius: '6px', cursor: 'pointer', whiteSpace: 'nowrap',
    fontFamily: 'inherit', fontSize: '14px', fontWeight: 600, lineHeight: 1,
    ...variants[variant],
  };
};
const toolbarDivider = { width: '1px', height: '24px', backgroundColor: '#ccc' };

const MapDataType = { Int: DataType.Int, Float: DataType.Float, Bool: DataType.Bool, String: DataType.String };
const MapOperator = { Equal: Operator.Equal, NotEqual: Operator.NotEqual, Greater: Operator.Greater, Less: Operator.Less, GreaterOrEqual: Operator.GreaterOrEqual, LessOrEqual: Operator.LessOrEqual };
const MapNodeType = { State: NodeType.State, Sequence: NodeType.Sequence, Selector: NodeType.Selector, Parallel: NodeType.Parallel, Decorator: NodeType.Decorator, Action: NodeType.Action, Condition: NodeType.ConditionNode };

const downloadBin = (uint8Array, filename) => {
  const blob = new Blob([uint8Array], { type: 'application/octet-stream' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
};

const downloadText = (text, filename) => {
  const blob = new Blob([text], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
};

const askBrainName = () => {
  const IDENTIFICADOR = /^[A-Za-z_][A-Za-z0-9_]*$/;

  while (true) {
    const resposta = window.prompt(
      "Name of this brain (e.g. Guardian, Archer, Boss).\n" +
      "It will be used for the .bin file and for the generated C# class.",
      ""
    );

    if (resposta === null) return null;

    const net = resposta.trim();
    if (net === "") {
      alert("The name cannot be empty.");
      continue;
    }
    if (!IDENTIFICADOR.test(net)) {
      alert("The name can only contain letters, digits, and underscores, " +
            "and cannot start with a digit.");
      continue;
    }
    return net;
  }
};

const extractActionsAndVariables = (nodes, edges) => {
  const actions = new Set();
  const variables = new Map();

  nodes.forEach(n => {
    const isDecorator = n.data?.btType === 'Decorator';
    if (n.data?.action_name && !isDecorator && n.data.action_name.trim() !== '') {
      actions.add(n.data.action_name.trim());
    }

    (n.data?.conditions || []).forEach(c => {
      if (c.variable && c.variable.trim() !== '') {
        variables.set(c.variable.trim(), c.compare_type);
      }
    });
  });

  edges.forEach(e => {
    (e.data?.conditions || []).forEach(c => {
      if (c.variable && c.variable.trim() !== '') {
        variables.set(c.variable.trim(), c.compare_type);
      }
    });
  });

  return { actions: Array.from(actions), variables };
};

const generateCSharpScaffolding = (brainType, actions, variables, brainName) => {
  const brainKind = brainType === BrainType.FiniteStateMachine ? "FSM" : "BT";

  let abstractMethods = "";
  let switchActions = "";
  let switchFloats = "";
  let switchBools = "";
  let switchInts = "";
  let switchStrings = "";

  actions.forEach(action => {
    abstractMethods += `    protected abstract NodeState OnAction_${action}();\n`;
    switchActions += `                "${action}" => OnAction_${action}(),\n`;
  });

  variables.forEach((type, name) => {
    if (type === "Float") {
      abstractMethods += `    protected abstract float OnGetFloat_${name}();\n`;
      switchFloats += `                "${name}" => OnGetFloat_${name}(),\n`;
    } else if (type === "Bool") {
      abstractMethods += `    protected abstract bool OnGetBool_${name}();\n`;
      switchBools += `                "${name}" => OnGetBool_${name}(),\n`;
    } else if (type === "Int") {
      abstractMethods += `    protected abstract int OnGetInt_${name}();\n`;
      switchInts += `                "${name}" => OnGetInt_${name}(),\n`;
    } else if (type === "String") {
      abstractMethods += `    protected abstract string OnGetString_${name}();\n`;
      switchStrings += `                "${name}" => OnGetString_${name}(),\n`;
    }
  });

  return `// ------------------------------------------------------------------------------
//  <autogenerated>
//      File automatically generated by the AI Logic Editor.
//      Brain Type: ${brainKind}
//      DO NOT EDIT THIS FILE. Changes will be lost when the brain is exported again.
//  </autogenerated>
// ------------------------------------------------------------------------------

using System;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;

public abstract class AIBrainBase_${brainName} : MonoBehaviour
{
    [Header("AI Engine Settings")]
    [Tooltip("Name of the exported brain file (e.g. ${brainName}.bin), located in Assets/StreamingAssets")]
    public string brainFileName;

    // --- METHODS TO BE IMPLEMENTED BY THE PROGRAMMER ---
${abstractMethods}
    // --- NATIVE PLUGIN INTEGRATION ---
    private const string DllName = "AIEnginePlugin";
    private IntPtr _enginePtr = IntPtr.Zero;
    private bool _isBrainLoaded = false;

    // --- DELEGATES ---
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int ActionCallback([MarshalAs(UnmanagedType.LPStr)] string name);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate float GetFloatCallback([MarshalAs(UnmanagedType.LPStr)] string name);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    private delegate bool GetBoolCallback([MarshalAs(UnmanagedType.LPStr)] string name);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int GetIntCallback([MarshalAs(UnmanagedType.LPStr)] string name);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.LPStr)]
    private delegate string GetStringCallback([MarshalAs(UnmanagedType.LPStr)] string name);

    // --- DLL IMPORTS ---
    [DllImport(DllName, EntryPoint = "AIEngine_Create", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr AIEngine_Create(ActionCallback actionCb, GetFloatCallback floatCb, GetBoolCallback boolCb, GetIntCallback intCb, GetStringCallback stringCb);

    [DllImport(DllName, EntryPoint = "AIEngine_Destroy", CallingConvention = CallingConvention.Cdecl)]
    private static extern void AIEngine_Destroy(IntPtr enginePtr);

    [DllImport(DllName, EntryPoint = "AIEngine_LoadBrain", CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    private static extern bool AIEngine_LoadBrain(IntPtr enginePtr, [MarshalAs(UnmanagedType.LPStr)] string filePath);

    [DllImport(DllName, EntryPoint = "AIEngine_Update", CallingConvention = CallingConvention.Cdecl)]
    private static extern void AIEngine_Update(IntPtr enginePtr, float deltaTime);

    [DllImport(DllName, EntryPoint = "AIEngine_GetActionParamInt", CallingConvention = CallingConvention.Cdecl)]
    private static extern int AIEngine_GetActionParamInt([MarshalAs(UnmanagedType.LPStr)] string name, int fallback);

    [DllImport(DllName, EntryPoint = "AIEngine_GetActionParamFloat", CallingConvention = CallingConvention.Cdecl)]
    private static extern float AIEngine_GetActionParamFloat([MarshalAs(UnmanagedType.LPStr)] string name, float fallback);

    [DllImport(DllName, EntryPoint = "AIEngine_GetActionParamBool", CallingConvention = CallingConvention.Cdecl)]
    private static extern int AIEngine_GetActionParamBool([MarshalAs(UnmanagedType.LPStr)] string name, int fallback);

    [DllImport(DllName, EntryPoint = "AIEngine_GetActionParamString", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr AIEngine_GetActionParamString([MarshalAs(UnmanagedType.LPStr)] string name);

    private ActionCallback _actionCallbackInstance;
    private GetFloatCallback _floatCallbackInstance;
    private GetBoolCallback _boolCallbackInstance;
    private GetIntCallback _intCallbackInstance;
    private GetStringCallback _stringCallbackInstance;

    private GCHandle _actionHandle;
    private GCHandle _floatHandle;
    private GCHandle _boolHandle;
    private GCHandle _intHandle;
    private GCHandle _stringHandle;

    protected virtual void Awake()
    {
        _actionCallbackInstance = new ActionCallback(DispatchAction);
        _floatCallbackInstance = new GetFloatCallback(DispatchFloat);
        _boolCallbackInstance = new GetBoolCallback(DispatchBool);
        _intCallbackInstance = new GetIntCallback(DispatchInt);
        _stringCallbackInstance = new GetStringCallback(DispatchString);

        _actionHandle = GCHandle.Alloc(_actionCallbackInstance, GCHandleType.Normal);
        _floatHandle = GCHandle.Alloc(_floatCallbackInstance, GCHandleType.Normal);
        _boolHandle = GCHandle.Alloc(_boolCallbackInstance, GCHandleType.Normal);
        _intHandle = GCHandle.Alloc(_intCallbackInstance, GCHandleType.Normal);
        _stringHandle = GCHandle.Alloc(_stringCallbackInstance, GCHandleType.Normal);

        _enginePtr = AIEngine_Create(
            _actionCallbackInstance,
            _floatCallbackInstance,
            _boolCallbackInstance,
            _intCallbackInstance,
            _stringCallbackInstance
        );
    }

    protected virtual void Start()
    {
        if (string.IsNullOrEmpty(brainFileName))
        {
            Debug.LogError($"[AI ENGINE] Error in {gameObject.name}: No 'Brain File Name' has been set in the Inspector.");
            return;
        }

        string fullPath = Path.Combine(Application.streamingAssetsPath, brainFileName);

        if (_enginePtr != IntPtr.Zero)
        {
            _isBrainLoaded = AIEngine_LoadBrain(_enginePtr, fullPath);
            if (_isBrainLoaded)
                Debug.Log($"<color=green>[AI ENGINE] Brain '{brainFileName}' loaded successfully in {gameObject.name}.</color>");
            else
                Debug.LogError($"[AI ENGINE] The engine could not load the brain file: {fullPath}");
        }
    }

    protected virtual void Update()
    {
        if (_enginePtr != IntPtr.Zero && _isBrainLoaded)
        {
            AIEngine_Update(_enginePtr, Time.deltaTime);
        }
    }

    protected virtual void OnDestroy()
    {
        if (_enginePtr != IntPtr.Zero)
        {
            AIEngine_Destroy(_enginePtr);
            _enginePtr = IntPtr.Zero;
        }

        if (_actionHandle.IsAllocated) _actionHandle.Free();
        if (_floatHandle.IsAllocated) _floatHandle.Free();
        if (_boolHandle.IsAllocated) _boolHandle.Free();
        if (_intHandle.IsAllocated) _intHandle.Free();
        if (_stringHandle.IsAllocated) _stringHandle.Free();
    }

    public bool LoadBrain(string filePath)
    {
        if (_enginePtr == IntPtr.Zero) return false;
        _isBrainLoaded = AIEngine_LoadBrain(_enginePtr, filePath);
        return _isBrainLoaded;
    }

    // --- ACTION PARAMETERS (only valid inside an OnAction_* method) ---
    protected int GetIntParam(string name, int fallback = 0) => AIEngine_GetActionParamInt(name, fallback);
    protected float GetFloatParam(string name, float fallback = 0f) => AIEngine_GetActionParamFloat(name, fallback);
    protected bool GetBoolParam(string name, bool fallback = false) => AIEngine_GetActionParamBool(name, fallback ? 1 : 0) != 0;
    protected string GetStringParam(string name, string fallback = "")
    {
        IntPtr ptr = AIEngine_GetActionParamString(name);
        return ptr == IntPtr.Zero ? fallback : Marshal.PtrToStringAnsi(ptr);
    }

    private int DispatchAction(string actionName)
    {
        try
        {
            NodeState result = actionName switch
            {
${switchActions}                _ => NodeState.Failure
            };
            return (int)result;
        }
        catch (Exception e)
        {
            Debug.LogError($"[AI ENGINE] The action {actionName} failed: {e.Message}");
            return (int)NodeState.Failure;
        }
    }

    private float DispatchFloat(string varName)
    {
        try { return varName switch { ${switchFloats} _ => 0.0f }; }
        catch (Exception e) { Debug.LogError($"[AI ENGINE] Float variable {varName} failed: {e.Message}"); return 0f; }
    }

    private bool DispatchBool(string varName)
    {
        try { return varName switch { ${switchBools} _ => false }; }
        catch (Exception e) { Debug.LogError($"[AI ENGINE] Bool variable {varName} failed: {e.Message}"); return false; }
    }

    private int DispatchInt(string varName)
    {
        try { return varName switch { ${switchInts} _ => 0 }; }
        catch (Exception e) { Debug.LogError($"[AI ENGINE] Int variable {varName} failed: {e.Message}"); return 0; }
    }

    private string DispatchString(string varName)
    {
        try { return varName switch { ${switchStrings} _ => "" }; }
        catch (Exception e) { Debug.LogError($"[AI ENGINE] String variable {varName} failed: {e.Message}"); return ""; }
    }
}
`;
};



const exportToFlatBuffer = (brainType, reactNodes, reactEdges, rootNodeIdString) => {

  try {
    const brainName = askBrainName();
    if (!brainName) return;

    const builder = new flatbuffers.Builder(1024);

    let resolvedBrainType = BrainType.BehaviorTree;
    if (brainType === 'FSM' || brainType === BrainType.FiniteStateMachine) {
        resolvedBrainType = BrainType.FiniteStateMachine;
    }

    const idMap = new Map();
    reactNodes.forEach((n, i) => idMap.set(n.id, i + 1));

    const nodeOffsets = reactNodes.map(reactNode => {

      const btType = reactNode.data?.btType;
      let nameToExport = '';
      if (resolvedBrainType === BrainType.FiniteStateMachine) {
        nameToExport = reactNode.data.action_name || '';
      } else if (btType === 'Action' || btType === 'Decorator') {
        nameToExport = reactNode.data.action_name || '';
      }
      const actionNameOffset = builder.createString(nameToExport);

      const paramOffsets = (reactNode.data.action_params || []).map(p => {
        const nameOff = builder.createString(p.name || '');
        let strValOff = 0;
        if (p.type === 'String' && p.string_val) strValOff = builder.createString(p.string_val);

        Parameter.startParameter(builder);
        Parameter.addName(builder, nameOff);
        Parameter.addType(builder, MapDataType[p.type] ?? DataType.Int);

        if (p.type === 'Int') Parameter.addIntVal(builder, Number(p.int_val) || 0);
        if (p.type === 'Float') Parameter.addFloatVal(builder, Number(p.float_val) || 0.0);
        if (p.type === 'Bool') Parameter.addBoolVal(builder, p.bool_val === true || p.bool_val === 'true');
        if (strValOff !== 0) Parameter.addStringVal(builder, strValOff);

        return Parameter.endParameter(builder);
      });
      const paramsVectorOffset = Node.createActionParamsVector(builder, paramOffsets);

      let conditionOffset = 0;

      const c = (reactNode.data.conditions && reactNode.data.conditions.length > 0)
                  ? reactNode.data.conditions[0]
                  : null;

      if (c) {
        const varNameOff = builder.createString(c.variable || '');
        let strValOff = 0;
        if (c.compare_type === 'String' && c.string_val) strValOff = builder.createString(c.string_val);

        Condition.startCondition(builder);
        Condition.addVariable(builder, varNameOff);
        Condition.addOp(builder, MapOperator[c.op] ?? Operator.Equal);
        Condition.addCompareType(builder, MapDataType[c.compare_type] ?? DataType.Int);

        if (c.compare_type === 'Int') Condition.addIntVal(builder, Number(c.int_val) || 0);
        if (c.compare_type === 'Float') Condition.addFloatVal(builder, Number(c.float_val) || 0.0);
        if (c.compare_type === 'Bool') Condition.addBoolVal(builder, c.bool_val === true || c.bool_val === 'true');
        if (strValOff !== 0) Condition.addStringVal(builder, strValOff);

        conditionOffset = Condition.endCondition(builder);
      }

      let childrenVectorOffset = 0;
      if (resolvedBrainType === BrainType.BehaviorTree) {
        const childEdges = reactEdges.filter(e => e.source === reactNode.id);
        const childNodes = childEdges.map(e => reactNodes.find(n => n.id === e.target)).filter(Boolean);
        childNodes.sort((a, b) => a.position.x - b.position.x);

        const childrenIntIds = childNodes.map(child => idMap.get(child.id) || 0);
        childrenVectorOffset = Node.createChildrenIdsVector(builder, childrenIntIds);
      }

      Node.startNode(builder);
      Node.addId(builder, idMap.get(reactNode.id) || 0);

      const nType = resolvedBrainType === BrainType.FiniteStateMachine
                    ? NodeType.State
                    : (MapNodeType[reactNode.data.btType] ?? NodeType.Action);

      Node.addType(builder, nType);
      Node.addActionName(builder, actionNameOffset);
      Node.addActionParams(builder, paramsVectorOffset);

      if (conditionOffset !== 0) {
        Node.addCondition(builder, conditionOffset);
      }

      if (childrenVectorOffset !== 0) Node.addChildrenIds(builder, childrenVectorOffset);
      return Node.endNode(builder);
    });

    const allNodesVectorOffset = AIBrainTemplate.createNodesVector(builder, nodeOffsets);

    let allEdgesVectorOffset = 0;
    if (resolvedBrainType === BrainType.FiniteStateMachine) {
      const edgeOffsets = reactEdges.map(e => {
        const condOffsets = (e.data?.conditions || []).map(c => {
          const varNameOff = builder.createString(c.variable || '');
          let strValOff = 0;
          if (c.compare_type === 'String' && c.string_val) strValOff = builder.createString(c.string_val);

          Condition.startCondition(builder);
          Condition.addVariable(builder, varNameOff);
          Condition.addOp(builder, MapOperator[c.op] ?? Operator.Equal);
          Condition.addCompareType(builder, MapDataType[c.compare_type] ?? DataType.Int);

          if (c.compare_type === 'Int') Condition.addIntVal(builder, Number(c.int_val) || 0);
          if (c.compare_type === 'Float') Condition.addFloatVal(builder, Number(c.float_val) || 0.0);
          if (c.compare_type === 'Bool') Condition.addBoolVal(builder, c.bool_val === true || c.bool_val === 'true');
          if (strValOff !== 0) Condition.addStringVal(builder, strValOff);

          return Condition.endCondition(builder);
        });
        const condOffset = condOffsets.length > 0 ? condOffsets[0] : 0;

        Edge.startEdge(builder);
        Edge.addSourceId(builder, idMap.get(e.source) || 0);
        Edge.addTargetId(builder, idMap.get(e.target) || 0);
        if (condOffset !== 0) Edge.addCondition(builder, condOffset);
        return Edge.endEdge(builder);
      });
      allEdgesVectorOffset = AIBrainTemplate.createEdgesVector(builder, edgeOffsets);
    } else {
      allEdgesVectorOffset = AIBrainTemplate.createEdgesVector(builder, []);
    }

    AIBrainTemplate.startAIBrainTemplate(builder);
    AIBrainTemplate.addType(builder, resolvedBrainType);
    AIBrainTemplate.addRootNodeId(builder, idMap.get(rootNodeIdString) || 0);
    AIBrainTemplate.addNodes(builder, allNodesVectorOffset);
    AIBrainTemplate.addEdges(builder, allEdgesVectorOffset);
    const templateOffset = AIBrainTemplate.endAIBrainTemplate(builder);

    builder.finish(templateOffset);

    downloadBin(builder.asUint8Array(), `${brainName}.bin`);

    const { actions, variables } = extractActionsAndVariables(reactNodes, reactEdges);
    const csCode = generateCSharpScaffolding(resolvedBrainType, actions, variables, brainName);
    downloadText(csCode, `AIBrainBase_${brainName}.cs`);

  } catch (error) {
    console.error("Error while exporting the brain:", error);
    alert("An error occurred while exporting the brain: " + error.message);
  }
};

const guardarEsquemaJSON = (nodes, edges, activeEditor) => {
    try {

      const projecte = {
        nodes: nodes,
        edges: edges,
        tipus: activeEditor
      };

      const jsonString = JSON.stringify(projecte, null, 2);

      const blob = new Blob([jsonString], { type: "application/json" });
      const url = URL.createObjectURL(blob);

      const link = document.createElement("a");
      link.href = url;
      link.download = `AIBrain_${activeEditor}_Project.json`;
      document.body.appendChild(link);
      link.click();

      document.body.removeChild(link);
      URL.revokeObjectURL(url);

    } catch (error) {
      console.error("Error while saving the project:", error);
      alert("Error while saving the project: " + error.message);
    }
};

const carregarEsquemaJSON = (event, setNodes, setEdges, activeEditor) => {
  const fitxer = event.target.files[0];
  if (!fitxer) return;

  const lector = new FileReader();
  lector.onload = (e) => {
    try {
      const projecte = JSON.parse(e.target.result);

      if (projecte.nodes && projecte.edges) {

        if (projecte.tipus && projecte.tipus !== activeEditor) {
          alert(`Error: This file is a ${projecte.tipus} project and the ${activeEditor} editor is open.\nPlease switch to the ${projecte.tipus} tab before loading it.`);
          return;
        }

        setNodes(projecte.nodes);
        setEdges(projecte.edges);
        alert("Project loaded successfully!");

      } else {
        alert("The file does not have the correct format for the AI Logic Editor.");
      }
    } catch {
      alert("Error while reading the JSON file.");
    }
  };
  lector.readAsText(fitxer);
  event.target.value = null;
};

const downloadEngineFiles = async () => {
  try {
    const fitxers = [
      { url: '/engine/AIEnginePlugin.dll', nom: 'AIEnginePlugin.dll' },
      { url: '/engine/AIEngineTypes.cs',   nom: 'AIEngineTypes.cs' },
      { url: '/engine/README.txt',     nom: 'README.txt' },
      { url: '/engine/README.ca.txt',  nom: 'README.ca.txt' },
    ];

    const zip = new JSZip();

    for (const fitxer of fitxers) {
      const resposta = await fetch(fitxer.url);
      if (!resposta.ok) {
        throw new Error(`Could not get ${fitxer.nom} (${resposta.status})`);
      }
      zip.file(fitxer.nom, await resposta.blob());
    }

    const contingut = await zip.generateAsync({ type: 'blob' });

    const url = URL.createObjectURL(contingut);
    const enllac = document.createElement('a');
    enllac.href = url;
    enllac.download = 'AIEngine_Setup.zip';
    document.body.appendChild(enllac);
    enllac.click();
    document.body.removeChild(enllac);
    URL.revokeObjectURL(url);
  } catch (error) {
    console.error("Error while preparing the engine files:", error);
    alert("The engine files could not be prepared: " + error.message);
  }
};

function StateNode({ id, data, selected }) {
  const { setNodes, setEdges } = useReactFlow();
  const [hovered, setHovered] = useState(false);

  const deleteNode = () => {
    setNodes((nds) => nds.filter((n) => n.id !== id));
    setEdges((eds) => eds.filter((e) => e.source !== id && e.target !== id));
  };

  const handleStyle = { opacity: hovered || selected ? 1 : 0, width: '12px', height: '12px', background: '#007bff', border: '2px solid white', zIndex: 10 };

  return (
    <div
      style={{
        position: 'relative', width: '120px', height: '120px', borderRadius: '50%', backgroundColor: 'white',
        border: selected ? '3px solid #007bff' : (data.is_initial_state ? '3px solid #28a745' : '2px solid #333'),
        display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center',
        boxShadow: selected ? '0 0 10px rgba(0, 123, 255, 0.5)' : '0 4px 6px rgba(0,0,0,0.1)'
      }}
      onMouseEnter={() => setHovered(true)} onMouseLeave={() => setHovered(false)}
    >
      {selected && <button onClick={deleteNode} className="nodrag" style={{ position: 'absolute', top: 0, right: 0, background: '#ff4d4d', color: 'white', border: 'none', borderRadius: '50%', width: '24px', height: '24px', cursor: 'pointer', zIndex: 20 }}>✕</button>}

      {data.is_initial_state && (
        <div style={{ position: 'absolute', top: -10, left: -10, background: '#28a745', color: 'white', borderRadius: '4px', padding: '2px 6px', fontSize: '10px', fontWeight: 'bold', zIndex: 20, boxShadow: '0 2px 4px rgba(0,0,0,0.2)' }} title="Initial State (Root)">
          ROOT
        </div>
      )}

      <Handle type="target" position={Position.Top} id="top-target" style={handleStyle} />
      <Handle type="source" position={Position.Top} id="top-source" style={handleStyle} />
      <Handle type="target" position={Position.Bottom} id="bot-target" style={handleStyle} />
      <Handle type="source" position={Position.Bottom} id="bot-source" style={handleStyle} />
      <Handle type="target" position={Position.Left} id="left-target" style={handleStyle} />
      <Handle type="source" position={Position.Left} id="left-source" style={handleStyle} />
      <Handle type="target" position={Position.Right} id="right-target" style={handleStyle} />
      <Handle type="source" position={Position.Right} id="right-source" style={handleStyle} />

      <div style={{ width: '90%', textAlign: 'center', pointerEvents: 'none' }}>
        <div style={{ fontWeight: 'bold', fontSize: '14px', color: '#333', wordWrap: 'break-word' }}>{data.label || 'Unnamed'}</div>
        {data.action_name && <div style={{ fontSize: '10px', color: '#007bff', marginTop: '4px', fontStyle: 'italic' }}>{data.action_name}</div>}
      </div>
    </div>
  );
}

function ConditionEdge({ id, source, target, sourceHandleId, targetHandleId, sourceX, sourceY, targetX, targetY, sourcePosition, targetPosition, markerEnd, selected }) {
  const { getEdges } = useReactFlow();
  const reverseArrow = getEdges().some(e => e.source === target && e.target === source && e.sourceHandle === targetHandleId && e.targetHandle === sourceHandleId);

  let offsetX = 0, offsetY = 0;
  if (reverseArrow) {
    const sep = source > target ? 1 : -1;
    if (sourcePosition === 'left' || sourcePosition === 'right') offsetY = 15 * sep;
    else offsetX = 15 * sep;
  }

  const [edgePath] = getSmoothStepPath({
    sourceX: sourceX + offsetX, sourceY: sourceY + offsetY, sourcePosition,
    targetX: targetX + offsetX, targetY: targetY + offsetY, targetPosition, borderRadius: 15
  });

  return (
    <>
      <BaseEdge path={edgePath} id={`${id}-bg`} style={{ strokeWidth: selected ? 7 : 6, stroke: '#ffffff' }} />
      <BaseEdge path={edgePath} markerEnd={markerEnd} id={id} style={{ strokeWidth: selected ? 3 : 2, stroke: selected ? '#28a745' : '#aaa' }} />
    </>
  );
}

function SidebarFSM({ nodes, setNodes, edges, setEdges }) {
  const selectedNode = nodes.find(n => n.selected);
  const selectedEdge = edges.find(e => e.selected);

  const updateNodeData = (id, field, value) => {
    setNodes(nds => nds.map(n => n.id === id ? { ...n, data: { ...n.data, [field]: value } } : n));
  };

  const toggleInitialState = (id) => {
    setNodes(nds => nds.map(n => {
      if (n.id === id) {
        return { ...n, data: { ...n.data, is_initial_state: !n.data.is_initial_state } };
      } else {
        return { ...n, data: { ...n.data, is_initial_state: false } };
      }
    }));
  };

  const addActionParam = (nodeId) => {
    setNodes(nds => nds.map(n => {
      if (n.id === nodeId) {
        const params = n.data?.action_params || [];
        return { ...n, data: { ...n.data, action_params: [...params, { name: "NewParam", type: "Int", int_val: 0 }] } };
      }
      return n;
    }));
  };

  const updateActionParam = (nodeId, index, field, value) => {
    setNodes(nds => nds.map(n => {
      if (n.id === nodeId) {
        const params = [...(n.data?.action_params || [])];
        params[index] = { ...params[index], [field]: value };
        return { ...n, data: { ...n.data, action_params: params } };
      }
      return n;
    }));
  };

  const deleteActionParam = (nodeId, index) => {
    setNodes(nds => nds.map(n => {
      if (n.id === nodeId) {
        const params = (n.data?.action_params || []).filter((_, i) => i !== index);
        return { ...n, data: { ...n.data, action_params: params } };
      }
      return n;
    }));
  };

  const addCondition = (edgeId) => {
    setEdges(eds => eds.map(e => {
      if (e.id === edgeId) {
        const conds = e.data?.conditions || [];
        return { ...e, data: { ...e.data, conditions: [...conds, { variable: "NewVariable", op: "Equal", compare_type: "Int", int_val: 0 }] } };
      }
      return e;
    }));
  };

  const updateCondition = (edgeId, index, field, value) => {
    setEdges(eds => eds.map(e => {
      if (e.id === edgeId) {
        const conds = [...(e.data?.conditions || [])];
        conds[index] = { ...conds[index], [field]: value };
        return { ...e, data: { ...e.data, conditions: conds } };
      }
      return e;
    }));
  };

  const deleteCondition = (edgeId, index) => {
    setEdges(eds => eds.map(e => {
      if (e.id === edgeId) {
        const conds = (e.data?.conditions || []).filter((_, i) => i !== index);
        return { ...e, data: { ...e.data, conditions: conds } };
      }
      return e;
    }));
  };

  return (
    <div style={{ width: '350px', backgroundColor: '#f8f9fa', borderLeft: '2px solid #ccc', padding: '20px', display: 'flex', flexDirection: 'column', gap: '15px', overflowY: 'auto' }}>
      <h2 style={{ margin: 0, fontSize: '18px', borderBottom: '2px solid #ddd', paddingBottom: '10px' }}>FSM Inspector</h2>

      {selectedNode && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <h3 style={{ color: '#007bff', margin: '10px 0 5px 0' }}>State Properties</h3>

          <label style={{...labelStyle, display: 'flex', alignItems: 'center', gap: '8px', cursor: 'pointer', backgroundColor: '#e9ecef', padding: '8px', borderRadius: '4px'}}>
            <input
              type="checkbox"
              checked={selectedNode.data?.is_initial_state || false}
              onChange={() => toggleInitialState(selectedNode.id)}
            />
            Set as Initial State (Root)
          </label>

          <label style={labelStyle}>State Name:</label>
          <input value={selectedNode.data?.label || ''} onChange={(e) => updateNodeData(selectedNode.id, 'label', e.target.value)} style={inputStyle} />

          <label style={labelStyle}>Action (action_name):</label>
          <input value={selectedNode.data?.action_name || ''} onChange={(e) => updateNodeData(selectedNode.id, 'action_name', e.target.value)} placeholder="E.g. Patrol" style={inputStyle} />

          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: '10px' }}>
            <span style={{ fontWeight: 'bold', fontSize: '14px' }}>Action Parameters:</span>
            <button onClick={() => addActionParam(selectedNode.id)} style={{ padding: '4px 8px', background: '#007bff', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontSize: '12px' }}>+ Add Param</button>
          </div>

          {(selectedNode.data?.action_params || []).map((param, index) => (
            <div key={index} style={{ border: '1px solid #b8daff', padding: '10px', borderRadius: '6px', backgroundColor: '#e2eef9', display: 'flex', flexDirection: 'column', gap: '8px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                <span style={{ fontSize: '12px', fontWeight: 'bold', color: '#0056b3' }}>Param {index + 1}</span>
                <button onClick={() => deleteActionParam(selectedNode.id, index)} style={{ color: 'red', background: 'none', border: 'none', cursor: 'pointer', fontSize: '14px' }}>✕</button>
              </div>

              <input placeholder="Param Name (e.g. speed)" value={param.name} onChange={(e) => updateActionParam(selectedNode.id, index, 'name', e.target.value)} style={inputStyle} />

              <select value={param.type} onChange={(e) => updateActionParam(selectedNode.id, index, 'type', e.target.value)} style={inputStyle}>
                <option value="Int">Int</option>
                <option value="Float">Float</option>
                <option value="Bool">Bool</option>
                <option value="String">String</option>
              </select>

              {param.type === 'Int' && <input type="number" placeholder="Value (Int)" value={param.int_val ?? 0} onChange={(e) => updateActionParam(selectedNode.id, index, 'int_val', parseInt(e.target.value))} style={inputStyle} />}
              {param.type === 'Float' && <input type="number" step="0.1" placeholder="Value (Float)" value={param.float_val ?? 0} onChange={(e) => updateActionParam(selectedNode.id, index, 'float_val', parseFloat(e.target.value))} style={inputStyle} />}
              {param.type === 'Bool' && (
                <select value={param.bool_val ? 'true' : 'false'} onChange={(e) => updateActionParam(selectedNode.id, index, 'bool_val', e.target.value === 'true')} style={inputStyle}>
                  <option value="true">True</option>
                  <option value="false">False</option>
                </select>
              )}
              {param.type === 'String' && <input type="text" placeholder="Value (String)" value={param.string_val || ''} onChange={(e) => updateActionParam(selectedNode.id, index, 'string_val', e.target.value)} style={inputStyle} />}
            </div>
          ))}
        </div>
      )}

      {selectedEdge && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <h3 style={{ color: '#28a745', margin: '10px 0 5px 0' }}>Transition Properties</h3>

          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: '10px' }}>
            <span style={{ fontWeight: 'bold', fontSize: '14px' }}>Condition:</span>
            {(!selectedEdge.data?.conditions || selectedEdge.data.conditions.length === 0) && (
              <button onClick={() => addCondition(selectedEdge.id)} style={{ padding: '4px 8px', background: '#28a745', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontSize: '12px' }}>+ Add Condition</button>
            )}
          </div>

          {(selectedEdge.data?.conditions || []).map((cond, index) => (
            <div key={index} style={{ border: '1px solid #ddd', padding: '10px', borderRadius: '6px', backgroundColor: 'white', display: 'flex', flexDirection: 'column', gap: '8px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                <span style={{ fontSize: '12px', fontWeight: 'bold', color: '#555' }}>Rule {index + 1}</span>
                <button onClick={() => deleteCondition(selectedEdge.id, index)} style={{ color: 'red', background: 'none', border: 'none', cursor: 'pointer', fontSize: '14px' }}>✕</button>
              </div>

              <input placeholder="Variable (e.g. Health)" value={cond.variable} onChange={(e) => updateCondition(selectedEdge.id, index, 'variable', e.target.value)} style={inputStyle} />

              <div style={{ display: 'flex', gap: '5px' }}>
                <select value={cond.op} onChange={(e) => updateCondition(selectedEdge.id, index, 'op', e.target.value)} style={{...inputStyle, flex: 1}}>
                  <option value="Equal">== Equal</option>
                  <option value="NotEqual">!= Not Equal</option>
                  <option value="Greater">&gt; Greater</option>
                  <option value="Less">&lt; Less</option>
                  <option value="GreaterOrEqual">&gt;= Greater/Equal</option>
                  <option value="LessOrEqual">&lt;= Less/Equal</option>
                </select>

                <select value={cond.compare_type} onChange={(e) => updateCondition(selectedEdge.id, index, 'compare_type', e.target.value)} style={{...inputStyle, flex: 1}}>
                  <option value="Int">Int</option>
                  <option value="Float">Float</option>
                  <option value="Bool">Bool</option>
                  <option value="String">String</option>
                </select>
              </div>

              {cond.compare_type === 'Int' && <input type="number" placeholder="Value (Int)" value={cond.int_val ?? 0} onChange={(e) => updateCondition(selectedEdge.id, index, 'int_val', parseInt(e.target.value))} style={inputStyle} />}
              {cond.compare_type === 'Float' && <input type="number" step="0.1" placeholder="Value (Float)" value={cond.float_val ?? 0} onChange={(e) => updateCondition(selectedEdge.id, index, 'float_val', parseFloat(e.target.value))} style={inputStyle} />}
              {cond.compare_type === 'Bool' && (
                <select value={cond.bool_val ? 'true' : 'false'} onChange={(e) => updateCondition(selectedEdge.id, index, 'bool_val', e.target.value === 'true')} style={inputStyle}>
                  <option value="true">True</option>
                  <option value="false">False</option>
                </select>
              )}
              {cond.compare_type === 'String' && <input type="text" placeholder="Value (String)" value={cond.string_val || ''} onChange={(e) => updateCondition(selectedEdge.id, index, 'string_val', e.target.value)} style={inputStyle} />}
            </div>
          ))}

          {(!selectedEdge.data?.conditions || selectedEdge.data?.conditions.length === 0) && (
            <div style={{ fontSize: '12px', color: '#999', fontStyle: 'italic', textAlign: 'center', padding: '10px 0' }}>Free transition (no conditions).</div>
          )}
        </div>
      )}
    </div>
  );
}

const btColors = {
  Selector: '#ff9900',
  Sequence: '#28a745',
  Parallel: '#6f42c1',
  Decorator: '#e83e8c',
  Action: '#007bff',
  Condition: '#17a2b8'
};

function BTNode({ id, data, selected }) {
  const { setNodes, setEdges } = useReactFlow();
  const deleteNode = () => {
    setNodes((nds) => nds.filter((n) => n.id !== id));
    setEdges((eds) => eds.filter((e) => e.source !== id && e.target !== id));
  };

  const bgColor = btColors[data.btType] || '#ccc';

  const hasChildren = ['Selector', 'Sequence', 'Parallel', 'Decorator'].includes(data.btType);

  return (
    <div style={{
      position: 'relative', width: '160px', padding: '10px', borderRadius: '8px',
      backgroundColor: 'white', border: `3px solid ${selected ? '#000' : bgColor}`,
      boxShadow: selected ? '0 4px 10px rgba(0,0,0,0.3)' : '0 2px 4px rgba(0,0,0,0.1)',
      textAlign: 'center'
    }}>
      {selected && <button onClick={deleteNode} className="nodrag" style={{ position: 'absolute', top: -10, right: -10, background: '#ff4d4d', color: 'white', border: 'none', borderRadius: '50%', width: '20px', height: '20px', cursor: 'pointer' }}>✕</button>}

      <Handle type="target" position={Position.Top} style={{ width: '10px', height: '10px', background: '#555' }} />

      <div style={{ fontSize: '10px', fontWeight: 'bold', color: bgColor, textTransform: 'uppercase', marginBottom: '5px' }}>{data.btType}</div>
      <div style={{ fontSize: '14px', color: '#333', fontWeight: 'bold' }}>{data.label || 'Node'}</div>

      <Handle
        type="source"
        position={Position.Bottom}
        isConnectable={hasChildren}
        style={{
          width: '10px',
          height: '10px',
          background: '#555',
          opacity: hasChildren ? 1 : 0,
          pointerEvents: hasChildren ? 'all' : 'none'
        }}
      />
    </div>
  );
}

function SidebarBT({ nodes, setNodes, setEdges }) {
  const selectedNode = nodes.find(n => n.selected);

  const updateNodeData = (id, field, value) => {
    setNodes(nds => nds.map(n => n.id === id ? { ...n, data: { ...n.data, [field]: value } } : n));
  };

const handleTypeChange = (e) => {
    const newType = e.target.value;
    const canHaveChildren = ['Selector', 'Sequence', 'Parallel', 'Decorator'].includes(newType);

    setNodes((nds) =>
      nds.map((n) => {
        if (n.id !== selectedNode.id) return n;

        const newData = { ...n.data, btType: newType };

        const VALID_DECORATORS = ['Inverter', 'ForceSuccess', 'ForceFailure',
          'SuccessIsFailure', 'Repeater', 'RetryUntilSuccess', 'Cooldown',
          'Timeout', 'ConditionCheck'];

        if (newType === 'Decorator' && !VALID_DECORATORS.includes(newData.action_name)) {
          newData.action_name = 'Inverter';
          newData.action_params = [];
        }

        return { ...n, data: newData };
      })
    );

    if (!canHaveChildren) {
      setEdges((eds) => eds.filter((edge) => edge.source !== selectedNode.id));
    }
  };

  const handleDecoratorChange = (nodeId, newActionName) => {
    let defaultParams = [];
    if (newActionName === 'Repeater') {
      defaultParams = [{ name: 'count', type: 'Int', int_val: 3 }];
    } else if (newActionName === 'Cooldown' || newActionName === 'Timeout') {
      defaultParams = [{ name: 'time', type: 'Float', float_val: 1.0 }];
    }

    setNodes(nds => nds.map(n => n.id === nodeId ? {
      ...n,
      data: { ...n.data, action_name: newActionName, action_params: defaultParams }
    } : n));
  };

  const updateSingleParam = (nodeId, field, value) => {
    setNodes(nds => nds.map(n => {
      if (n.id === nodeId) {
        const params = [...(n.data?.action_params || [])];
        if (params.length > 0) params[0] = { ...params[0], [field]: value };
        return { ...n, data: { ...n.data, action_params: params } };
      }
      return n;
    }));
  };

  const addActionParam = (nodeId) => setNodes(nds => nds.map(n => n.id === nodeId ? { ...n, data: { ...n.data, action_params: [...(n.data?.action_params || []), { name: "NewParam", type: "Int", int_val: 0 }] } } : n));
  const updateActionParam = (nodeId, index, field, value) => setNodes(nds => nds.map(n => { if (n.id === nodeId) { const p = [...n.data.action_params]; p[index] = { ...p[index], [field]: value }; return { ...n, data: { ...n.data, action_params: p } }; } return n; }));
  const deleteActionParam = (nodeId, index) => setNodes(nds => nds.map(n => n.id === nodeId ? { ...n, data: { ...n.data, action_params: n.data.action_params.filter((_, i) => i !== index) } } : n));

  const addCondition = (nodeId) => setNodes(nds => nds.map(n => n.id === nodeId ? { ...n, data: { ...n.data, conditions: [...(n.data?.conditions || []), { variable: "Var", op: "Equal", compare_type: "Int", int_val: 0 }] } } : n));
  const updateCondition = (nodeId, index, field, value) => setNodes(nds => nds.map(n => { if (n.id === nodeId) { const c = [...n.data.conditions]; c[index] = { ...c[index], [field]: value }; return { ...n, data: { ...n.data, conditions: c } }; } return n; }));
  const deleteCondition = (nodeId, index) => setNodes(nds => nds.map(n => n.id === nodeId ? { ...n, data: { ...n.data, conditions: n.data.conditions.filter((_, i) => i !== index) } } : n));

  if (!selectedNode) return null;

  const btType = selectedNode.data?.btType || 'Action';
  const actionName = selectedNode.data?.action_name || '';

  const showConditionsPanel = btType === 'Condition' || (btType === 'Decorator' && actionName === 'ConditionCheck');

  return (
    <div style={{ width: '350px', background: '#f8f9fa', borderLeft: '2px solid #ccc', padding: '20px', display: 'flex', flexDirection: 'column', gap: '15px', overflowY: 'auto' }}>
      <h2 style={{ margin: 0, fontSize: '18px', borderBottom: '2px solid #ddd', paddingBottom: '10px' }}>BT Inspector</h2>

      <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
        <label style={labelStyle}>Node Type:</label>
        <select value={btType} onChange={handleTypeChange} style={inputStyle}>
          <option value="Selector">Selector (?)</option>
          <option value="Sequence">Sequence (→)</option>
          <option value="Parallel">Parallel (⇉)</option>
          <option value="Decorator">Decorator (δ)</option>
          <option value="Action">Action</option>
          <option value="Condition">ConditionNode</option>
        </select>
        <label style={labelStyle}>Node Label (UI Only):</label>
        <input value={selectedNode.data?.label || ''} onChange={(e) => updateNodeData(selectedNode.id, 'label', e.target.value)} style={inputStyle} />
      </div>

      {btType === 'Decorator' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', marginTop: '10px' }}>
          <h3 style={{ color: '#007bff', margin: '10px 0 5px 0' }}>Decorator Settings</h3>
          <label style={labelStyle}>Type:</label>
          <select value={actionName} onChange={(e) => handleDecoratorChange(selectedNode.id, e.target.value)} style={inputStyle}>
            <option value="">-- Select a Type --</option>
            <option value="ConditionCheck">Condition Check</option>
            <option value="Inverter">Inverter</option>
            <option value="ForceSuccess">Force Success</option>
            <option value="ForceFailure">Force Failure</option>
            <option value="SuccessIsFailure">Success is Failure</option>
            <option value="Repeater">Repeater (Loop)</option>
            <option value="RetryUntilSuccess">Retry Until Success</option>
            <option value="Cooldown">Cooldown Timer</option>
            <option value="Timeout">Timeout</option>
          </select>

          {actionName === 'Repeater' && (
            <div>
              <label style={labelStyle}>Iterations (Int):</label>
              <input type="number" min="1" value={selectedNode.data?.action_params?.[0]?.int_val ?? 3} onChange={(e) => updateSingleParam(selectedNode.id, 'int_val', parseInt(e.target.value))} style={inputStyle} />
            </div>
          )}

          {(actionName === 'Cooldown' || actionName === 'Timeout') && (
            <div>
              <label style={labelStyle}>Time in Seconds (Float):</label>
              <input type="number" step="0.1" min="0" value={selectedNode.data?.action_params?.[0]?.float_val ?? 1.0} onChange={(e) => updateSingleParam(selectedNode.id, 'float_val', parseFloat(e.target.value))} style={inputStyle} />
            </div>
          )}
        </div>
      )}

      {btType === 'Action' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', marginTop: '10px' }}>
          <h3 style={{ color: '#007bff', margin: '10px 0 5px 0' }}>Action Settings</h3>
          <label style={labelStyle}>Action Function Name:</label>
          <input value={selectedNode.data?.action_name || ''} onChange={(e) => updateNodeData(selectedNode.id, 'action_name', e.target.value)} placeholder="E.g. MoveTo" style={inputStyle} />

          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 'bold', fontSize: '14px' }}>Parameters:</span>
            <button onClick={() => addActionParam(selectedNode.id)} style={{ padding: '4px 8px', background: '#007bff', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontSize: '12px' }}>+ Add</button>
          </div>
          {(selectedNode.data?.action_params || []).map((param, index) => (
             <div key={index} style={{ border: '1px solid #b8daff', padding: '10px', borderRadius: '6px', backgroundColor: '#e2eef9' }}>
               <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '5px' }}>
                 <span style={{ fontSize: '12px', fontWeight: 'bold', color: '#0056b3' }}>Param {index + 1}</span>
                 <button onClick={() => deleteActionParam(selectedNode.id, index)} style={{ color: 'red', background: 'none', border: 'none', cursor: 'pointer' }}>✕</button>
               </div>
               <input placeholder="Name" value={param.name} onChange={(e) => updateActionParam(selectedNode.id, index, 'name', e.target.value)} style={{...inputStyle, marginBottom: '5px'}} />
               <select value={param.type} onChange={(e) => updateActionParam(selectedNode.id, index, 'type', e.target.value)} style={{...inputStyle, marginBottom: '5px'}}>
                 <option value="Int">Int</option><option value="Float">Float</option><option value="Bool">Bool</option><option value="String">String</option>
               </select>
               {param.type === 'Int' && <input type="number" value={param.int_val ?? 0} onChange={(e) => updateActionParam(selectedNode.id, index, 'int_val', parseInt(e.target.value))} style={inputStyle} />}
               {param.type === 'Float' && <input type="number" step="0.1" value={param.float_val ?? 0} onChange={(e) => updateActionParam(selectedNode.id, index, 'float_val', parseFloat(e.target.value))} style={inputStyle} />}
               {param.type === 'Bool' && <select value={param.bool_val ? 'true' : 'false'} onChange={(e) => updateActionParam(selectedNode.id, index, 'bool_val', e.target.value === 'true')} style={inputStyle}><option value="true">True</option><option value="false">False</option></select>}
               {param.type === 'String' && <input type="text" value={param.string_val || ''} onChange={(e) => updateActionParam(selectedNode.id, index, 'string_val', e.target.value)} style={inputStyle} />}
             </div>
          ))}
        </div>
      )}

      {showConditionsPanel && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', marginTop: '10px' }}>
          <h3 style={{ color: '#28a745', margin: '10px 0 5px 0' }}>Condition Settings</h3>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span style={{ fontWeight: 'bold', fontSize: '14px' }}>Condition:</span>

            {(!selectedNode.data?.conditions || selectedNode.data.conditions.length === 0) && (
              <button onClick={() => addCondition(selectedNode.id)} style={{ padding: '4px 8px', background: '#28a745', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontSize: '12px' }}>+ Add</button>
            )}
          </div>
          {(selectedNode.data?.conditions || []).map((cond, index) => (
             <div key={index} style={{ border: '1px solid #ddd', padding: '10px', borderRadius: '6px', backgroundColor: 'white' }}>
               <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '5px' }}>
                 <span style={{ fontSize: '12px', fontWeight: 'bold', color: '#555' }}>Rule {index + 1}</span>
                 <button onClick={() => deleteCondition(selectedNode.id, index)} style={{ color: 'red', background: 'none', border: 'none', cursor: 'pointer' }}>✕</button>
               </div>
               <input placeholder="Variable" value={cond.variable} onChange={(e) => updateCondition(selectedNode.id, index, 'variable', e.target.value)} style={{...inputStyle, marginBottom: '5px'}} />
               <div style={{ display: 'flex', gap: '5px', marginBottom: '5px' }}>
                 <select value={cond.op} onChange={(e) => updateCondition(selectedNode.id, index, 'op', e.target.value)} style={{...inputStyle, flex: 1}}>
                   <option value="Equal">== Equal</option><option value="NotEqual">!= Not Equal</option><option value="Greater">&gt; Greater</option><option value="Less">&lt; Less</option><option value="GreaterOrEqual">&gt;= Greater/Equal</option><option value="LessOrEqual">&lt;= Less/Equal</option>
                 </select>
                 <select value={cond.compare_type} onChange={(e) => updateCondition(selectedNode.id, index, 'compare_type', e.target.value)} style={{...inputStyle, flex: 1}}>
                   <option value="Int">Int</option><option value="Float">Float</option><option value="Bool">Bool</option><option value="String">String</option>
                 </select>
               </div>
               {cond.compare_type === 'Int' && <input type="number" value={cond.int_val ?? 0} onChange={(e) => updateCondition(selectedNode.id, index, 'int_val', parseInt(e.target.value))} style={inputStyle} />}
               {cond.compare_type === 'Float' && <input type="number" step="0.1" value={cond.float_val ?? 0} onChange={(e) => updateCondition(selectedNode.id, index, 'float_val', parseFloat(e.target.value))} style={inputStyle} />}
               {cond.compare_type === 'Bool' && <select value={cond.bool_val ? 'true' : 'false'} onChange={(e) => updateCondition(selectedNode.id, index, 'bool_val', e.target.value === 'true')} style={inputStyle}><option value="true">True</option><option value="false">False</option></select>}
               {cond.compare_type === 'String' && <input type="text" value={cond.string_val || ''} onChange={(e) => updateCondition(selectedNode.id, index, 'string_val', e.target.value)} style={inputStyle} />}
             </div>
          ))}
        </div>
      )}
    </div>
  );
}

const fsmNodeTypes = { stateNode: StateNode };
const fsmEdgeTypes = { conditionEdge: ConditionEdge };

function FSMEditor() {
const initialNodes = [
    {
      id: 'fsm_node_1',
      type: 'stateNode',
      position: { x: 250, y: 150 },
      data: {
        label: 'Initial State',
        is_initial_state: true,
        action_name: '',
        action_params: []
      }
    }
  ];

  const [nodes, setNodes, onNodesChange] = useNodesState(initialNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState([]);
  const somethingSelected = nodes.some(n => n.selected) || edges.some(e => e.selected);

  const onConnect = useCallback((params) => {
    setEdges((eds) => addEdge({ ...params, type: 'conditionEdge', data: { conditions: [] }, markerEnd: { type: MarkerType.ArrowClosed, color: '#28a745' } }, eds));
  }, [setEdges]);

  const addNode = () => setNodes(nds => [...nds, { id: `fsm_${Date.now()}`, type: 'stateNode', position: { x: 100, y: 100 }, data: { label: `New State`, action_name: '' } }]);

  const handleExport = () => {
    const rootNode = nodes.find(n => n.data.is_initial_state);
    if (!rootNode && nodes.length > 0) {
      alert("Please mark one state as the Initial State (ROOT).");
      return;
    }

    exportToFlatBuffer(BrainType.FiniteStateMachine, nodes, edges, rootNode?.id);
  };

  return (
    <div style={{ display: 'flex', width: '100%', height: '100%' }}>
      <div style={{ flex: 1, position: 'relative', cursor: 'default' }}>
        <ReactFlow nodes={nodes} edges={edges} onNodesChange={onNodesChange} onEdgesChange={onEdgesChange} onConnect={onConnect} nodeTypes={fsmNodeTypes} edgeTypes={fsmEdgeTypes} fitView deleteKeyCode={['Backspace', 'Delete']}>
          <Background />
          <Controls />

          <Panel position="top-right" style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
            <button onClick={addNode} style={toolbarButton('primary')}>+ Add State</button>

            <div style={toolbarDivider} />

            <button onClick={() => guardarEsquemaJSON(nodes, edges, 'FSM')} style={toolbarButton('secondary')}>
              Save FSM Project (.json)
            </button>
            {/* Un label fa de botó per amagar l'input de tipus file */}
            <label style={toolbarButton('secondary')}>
              Load FSM Project
              <input
                type="file"
                accept=".json"
                onChange={(e) => carregarEsquemaJSON(e, setNodes, setEdges, 'FSM')}
                style={{ display: 'none' }}
              />
            </label>

            <div style={toolbarDivider} />

            <button onClick={handleExport} style={toolbarButton('primaryOutline')}>Export FSM (.bin)</button>
            <button
              onClick={downloadEngineFiles}
              style={toolbarButton('neutral')}
              title="Native plugin, shared types and instructions. Needed only once per Unity project."
            >
              Engine Setup (.zip)
            </button>
          </Panel>
        </ReactFlow>
      </div>
      {somethingSelected && (
        <SidebarFSM nodes={nodes} setNodes={setNodes} edges={edges} setEdges={setEdges} />
      )}
    </div>
  );
}

const btNodeTypes = { btNode: BTNode };

function BTEditor() {
  const [nodes, setNodes, onNodesChange] = useNodesState([{ id: 'root', type: 'btNode', position: { x: 250, y: 50 }, data: { label: 'Root', btType: 'Selector' } }]);
  const [edges, setEdges, onEdgesChange] = useEdgesState([]);

  const onConnect = useCallback((params) => {
    setEdges((eds) => {
      const alreadyHasParent = eds.some((edge) => edge.target === params.target);

      if (alreadyHasParent) {
        alert("Error: A node in a Behavior Tree can only have one parent.");
        return eds;
      }

      return addEdge(params, eds);
    });
  }, [setEdges]);

  const addNode = () => setNodes(nds => [...nds, { id: `bt_${Date.now()}`, type: 'btNode', position: { x: 250, y: 200 }, data: { label: `New Node`, btType: 'Action' } }]);

  const handleExport = () => {
    if (nodes.length === 0) {
      alert("Error: The tree is empty.");
      return;
    }

    const rootNodes = nodes.filter(n => !edges.some(e => e.target === n.id));

    if (rootNodes.length === 0) {
      alert("Error: No root node found. The entire tree is a closed loop.");
      return;
    }

    if (rootNodes.length > 1) {
      alert(`Error: You have ${rootNodes.length} root or disconnected nodes. There must be exactly 1 starting root node.`);
      return;
    }

    const rootId = rootNodes[0].id;

    const visited = new Set();
    let hasCycle = false;

    const checkTree = (currentNodeId, currentPath) => {
      if (currentPath.has(currentNodeId)) {
        hasCycle = true;
        return;
      }

      visited.add(currentNodeId);
      currentPath.add(currentNodeId);

      const childrenIds = edges.filter(e => e.source === currentNodeId).map(e => e.target);
      for (const childId of childrenIds) {
        checkTree(childId, new Set(currentPath));
      }
    };

    checkTree(rootId, new Set());

    if (hasCycle) {
      alert("Error: An internal loop was detected. A Behavior Tree must always flow downwards and cannot connect backwards.");
      return;
    }

    if (visited.size !== nodes.length) {
      alert("Error: There are nodes disconnected from the main tree (possibly forming an isolated loop). All nodes must be connected directly or indirectly to the root.");
      return;
    }

    const decoratorsWithoutType = nodes.filter(n =>
      n.data?.btType === 'Decorator' &&
      (!n.data?.action_name || n.data.action_name.trim() === '')
    );

    if (decoratorsWithoutType.length > 0) {
      alert(`Error: ${decoratorsWithoutType.length} decorator node(s) have no type assigned. ` +
            `Select a type in the side panel before exporting.`);
      return;
    }

    exportToFlatBuffer(BrainType.BehaviorTree, nodes, edges, rootId);
  };

  return (
    <div style={{ display: 'flex', width: '100%', height: '100%' }}>
      <div style={{ flex: 1, position: 'relative', cursor: 'default' }}>
        <ReactFlow nodes={nodes} edges={edges} onNodesChange={onNodesChange} onEdgesChange={onEdgesChange} onConnect={onConnect} nodeTypes={btNodeTypes} fitView deleteKeyCode={['Backspace', 'Delete']}>
          <Background />
          <Controls />

          <Panel position="top-right" style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
            <button onClick={addNode} style={toolbarButton('primary')}>+ Add BT Node</button>

            <div style={toolbarDivider} />

            <button onClick={() => guardarEsquemaJSON(nodes, edges, 'BT')} style={toolbarButton('secondary')}>
              Save BT Project (.json)
            </button>
            {/* Un label fa de botó per amagar l'input de tipus file */}
            <label style={toolbarButton('secondary')}>
              Load BT Project
              <input
                type="file"
                accept=".json"
                onChange={(e) => carregarEsquemaJSON(e, setNodes, setEdges, 'BT')}
                style={{ display: 'none' }}
              />
            </label>

            <div style={toolbarDivider} />

            <button onClick={handleExport} style={toolbarButton('primaryOutline')}>Export BT (.bin)</button>
            <button
              onClick={downloadEngineFiles}
              style={toolbarButton('neutral')}
              title="Native plugin, shared types and instructions. Needed only once per Unity project."
            >
              Engine Setup (.zip)
            </button>
          </Panel>
        </ReactFlow>
      </div>
      <SidebarBT nodes={nodes} setNodes={setNodes} setEdges={setEdges} />
    </div>
  );
}

export default function App() {
  const [activeEditor, setActiveEditor] = useState('FSM');

  return (
    <div style={{ display: 'flex', flexDirection: 'column', width: '100vw', height: '100vh', overflow: 'hidden' }}>

      <div style={{ flexShrink: 0, height: '50px', backgroundColor: '#222', display: 'flex', alignItems: 'center', padding: '0 20px', gap: '10px' }}>
        <h1 style={{ color: 'white', fontSize: '18px', margin: 0, marginRight: '20px' }}>AI Logic Editor</h1>

        <button onClick={() => setActiveEditor('FSM')} style={{ ...buttonStyle(activeEditor === 'FSM' ? '#aa3bff' : '#444'), borderRadius: '20px' }}>
          Finite State Machine
        </button>

        <button onClick={() => setActiveEditor('BT')} style={{ ...buttonStyle(activeEditor === 'BT' ? '#aa3bff' : '#444'), borderRadius: '20px' }}>
          Behavior Tree
        </button>
      </div>

      <div style={{ flex: 1, position: 'relative', overflow: 'hidden' }}>
        {activeEditor === 'FSM' ? (
          <ReactFlowProvider key="fsm-editor">
            <FSMEditor />
          </ReactFlowProvider>
        ) : (
          <ReactFlowProvider key="bt-editor">
            <BTEditor />
          </ReactFlowProvider>
        )}
      </div>

    </div>
  );
}