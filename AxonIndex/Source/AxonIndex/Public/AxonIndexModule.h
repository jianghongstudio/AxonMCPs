#pragma once

#include "Modules/ModuleManager.h"

class FAxonIndexModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
