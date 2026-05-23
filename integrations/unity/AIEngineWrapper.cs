using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class AIEngineWrapper : MonoBehaviour
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool ActionCallback(string actionName);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate float GetFloatVarCallback(string varName);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool GetBoolVarCallback(string varName);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetIntVarCallback(string varName);

    private const string DllName = "AIEnginePlugin";

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr AIEngine_Create(ActionCallback actionCb, GetFloatVarCallback floatCb, GetBoolVarCallback boolCb, GetIntVarCallback intCb);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void AIEngine_Destroy(IntPtr enginePtr);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern bool AIEngine_LoadBrain(IntPtr enginePtr, string filePath);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void AIEngine_Update(IntPtr enginePtr);

    private IntPtr _enginePtr = IntPtr.Zero;
    
    public float enemyDistance = 20.0f;

    private ActionCallback _actionCb;
    private GetFloatVarCallback _floatCb;
    private GetBoolVarCallback _boolCb;
    private GetIntVarCallback _intCb;

    void Start()
    {
        _actionCb = OnExecuteAction;
        _floatCb = OnGetFloatVariable;
        _boolCb = (name) => false; 
        _intCb = (name) => 0;      

        _enginePtr = AIEngine_Create(_actionCb, _floatCb, _boolCb, _intCb);

        string brainPath = Application.dataPath + "/Plugins/fsm_test.bin"; 
        
        if (AIEngine_LoadBrain(_enginePtr, brainPath))
        {
            Debug.Log("[Unity] Cervell de IA carregat correctament.");
        }
        else
        {
            Debug.LogError("[Unity] Error en carregar el cervell de IA.");
        }
    }

    void Update()
    {
        if (_enginePtr != IntPtr.Zero)
        {
            enemyDistance -= Time.deltaTime * 4.0f;
            
            AIEngine_Update(_enginePtr);
        }
    }

    void OnDestroy()
    {
        if (_enginePtr != IntPtr.Zero)
        {
            AIEngine_Destroy(_enginePtr);
            _enginePtr = IntPtr.Zero;
        }
    }
    
    private bool OnExecuteAction(string actionName)
    {
        Debug.Log($"[JOC] Executant acció des de C++: {actionName}");
        return true;
    }

    private float OnGetFloatVariable(string varName)
    {
        if (varName == "EnemyDistance")
        {
            return enemyDistance;
        }
        return 0.0f;
    }
}