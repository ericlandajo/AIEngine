#pragma once
#include <string>

class IVariableUpdater {
public:
    virtual ~IVariableUpdater() = default;

    virtual bool GetBoolVariable(const std::string& varName) = 0;
    virtual float GetFloatVariable(const std::string& varName) = 0;
    virtual int GetIntVariable(const std::string& varName) = 0;
    virtual std::string GetStringVariable(const std::string& varName) = 0;
};