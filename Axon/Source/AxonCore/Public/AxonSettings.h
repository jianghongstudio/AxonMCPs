#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AxonSettings.generated.h"

UENUM()
enum class EAxonLogVerbosity : uint8
{
	Quiet,
	Normal,
	Verbose,
	VeryVerbose
};

UCLASS(config=Axon, defaultconfig, meta=(DisplayName="Axon"))
class AXONCORE_API UAxonSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAxonSettings();

	/** Master enable for the Axon MCP HTTP server. Takes effect on next editor restart. */
	UPROPERTY(config, EditAnywhere, Category="MCP Server")
	bool bMcpServerEnabled = true;

	/** Port for the embedded MCP HTTP server */
	UPROPERTY(config, EditAnywhere, Category="MCP Server", meta=(ClampMin="1024", ClampMax="65535"))
	int32 ServerPort = 9320;

	/** Log verbosity for Axon systems */
	UPROPERTY(config, EditAnywhere, Category="Logging")
	EAxonLogVerbosity LogVerbosity = EAxonLogVerbosity::Normal;

	static const UAxonSettings* Get();

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};
