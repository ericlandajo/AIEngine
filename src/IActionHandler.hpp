#pragma once
#include <string>

class IActionHandler {
public:
    virtual ~IActionHandler() = default;

    // Aquest mètode serà cridat pel motor quan un node Action s'executi.
    // Retorna 'true' si l'acció s'està executant/ha tingut èxit, 'false' si ha fallat.
    virtual bool ExecuteAction(const std::string& actionName) = 0;
};