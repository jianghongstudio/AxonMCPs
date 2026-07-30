#pragma once

#include "Modules/ModuleManager.h"

class FAxonAnimationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
