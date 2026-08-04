#include "AxonSettings.h"

UAxonSettings::UAxonSettings()
{
}

const UAxonSettings* UAxonSettings::Get()
{
	return GetDefault<UAxonSettings>();
}

TArray<FName> UAxonSettings::GetIndexedContentPaths()
{
	TArray<FName> Paths;
	Paths.Add(FName(TEXT("/Game")));

	if (const UAxonSettings* Settings = Get())
	{
		for (const FString& Path : Settings->AdditionalContentPaths)
		{
			if (!Path.IsEmpty())
			{
				Paths.AddUnique(FName(*Path));
			}
		}
	}

	return Paths;
}

bool UAxonSettings::IsIndexedContentPath(const FString& PackagePath)
{
	if (PackagePath.StartsWith(TEXT("/Game/")))
	{
		return true;
	}

	if (const UAxonSettings* Settings = Get())
	{
		for (const FString& ContentPath : Settings->AdditionalContentPaths)
		{
			if (!ContentPath.IsEmpty() && PackagePath.StartsWith(ContentPath + TEXT("/")))
			{
				return true;
			}
		}
	}

	return false;
}
