#pragma once
#include <string>
#include <unordered_map>
#include <aitoolkit/behtree.hpp>

struct ActionParameter {
    enum class Type { Int, Float, Bool, String };
    Type type;
    
    int int_val = 0;
    float float_val = 0.0f;
    bool bool_val = false;
    std::string string_val = "";
};

using ActionParameters = std::unordered_map<std::string, ActionParameter>;

class IActionHandler {
public:
    virtual ~IActionHandler() = default;
    
    virtual aitoolkit::bt::execution_state ExecuteAction(const std::string& actionName, const ActionParameters& params) = 0;
};

