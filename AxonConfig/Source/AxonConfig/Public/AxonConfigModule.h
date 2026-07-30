#pragma once

#include "Modules/ModuleManager.h"

class FAxonConfigModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
