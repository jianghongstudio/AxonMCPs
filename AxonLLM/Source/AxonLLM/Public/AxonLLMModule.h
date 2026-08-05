#pragma once

#include "Modules/ModuleManager.h"

class FAxonLLMModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
