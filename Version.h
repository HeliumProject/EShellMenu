#pragma once

#include "resource.h"

#define LAUNCHER_VERSION_STRINGIFY(compatible, feature, patch) #compatible"."#feature"."#patch
#define LAUNCHER_VERSION_TOSTRING(compatible, feature, patch) LAUNCHER_VERSION_STRINGIFY(compatible,feature,patch)
#define LAUNCHER_VERSION_STRING LAUNCHER_VERSION_TOSTRING( LAUNCHER_VERSION_MAJOR, LAUNCHER_VERSION_MINOR, LAUNCHER_VERSION_PATCH)

#define RESOURCE_VERSION_STRINGIFY(compatible, feature, patch) #compatible", "#feature", "#patch", 0"
#define RESOURCE_VERSION_TOSTRING(compatible, feature, patch) RESOURCE_VERSION_STRINGIFY(compatible, feature, patch)
#define RESOURCE_VERSION_STRING RESOURCE_VERSION_TOSTRING(LAUNCHER_VERSION_MAJOR,LAUNCHER_VERSION_MINOR,LAUNCHER_VERSION_PATCH)
