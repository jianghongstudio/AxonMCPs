#pragma once

#include "Modules/ModuleManager.h"

class FAxonEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
