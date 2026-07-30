#pragma once

#include "Modules/ModuleManager.h"

class FAxonRewindDebuggerModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
