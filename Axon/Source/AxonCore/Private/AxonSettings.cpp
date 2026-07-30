#include "AxonSettings.h"

UAxonSettings::UAxonSettings()
{
}

const UAxonSettings* UAxonSettings::Get()
{
	return GetDefault<UAxonSettings>();
}
