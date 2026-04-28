#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNeptuneEqicMcpModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    bool bInitialized = false;
};
