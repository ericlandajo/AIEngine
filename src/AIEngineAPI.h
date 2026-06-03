#pragma once

#if defined(_MSC_VER)
    #define EXPORT_API __declspec(dllexport)
#else
    #define EXPORT_API __attribute__((visibility("default")))
#endif

extern "C" {
    typedef bool (*ActionCallback)(const char* actionName); 
    typedef float (*GetFloatVarCallback)(const char* varName);
    typedef bool (*GetBoolVarCallback)(const char* varName);
    typedef int (*GetIntVarCallback)(const char* varName);

    EXPORT_API void* AIEngine_Create(ActionCallback actionCb, 
                                     GetFloatVarCallback floatCb, 
                                     GetBoolVarCallback boolCb,
                                     GetIntVarCallback intCb);
                                     
    EXPORT_API void AIEngine_Destroy(void* enginePtr);
    
    EXPORT_API bool AIEngine_LoadBrain(void* enginePtr, const char* filePath);
    EXPORT_API void AIEngine_Update(void* enginePtr, float deltaTime);
}