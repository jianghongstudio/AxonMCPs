#pragma once

#include "Modules/ModuleManager.h"

class FAxonBlueprintModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
