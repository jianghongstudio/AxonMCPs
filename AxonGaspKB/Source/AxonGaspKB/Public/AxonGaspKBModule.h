#pragma once

#include "Modules/ModuleManager.h"

class FAxonGaspKBModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
