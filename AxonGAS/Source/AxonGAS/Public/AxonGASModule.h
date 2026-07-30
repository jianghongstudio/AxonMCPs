#pragma once

#include "Modules/ModuleInterface.h"

class FAxonGASModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
