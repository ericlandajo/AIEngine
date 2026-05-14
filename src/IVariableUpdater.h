#pragma once
#include <string>

class IVariableUpdater {
public:
    virtual ~IVariableUpdater() = default;

    // Aquest mètode serà cridat quan el motor necessiti avaluar una condició.
    virtual bool GetCondition(const std::string& conditionName) = 0;
};