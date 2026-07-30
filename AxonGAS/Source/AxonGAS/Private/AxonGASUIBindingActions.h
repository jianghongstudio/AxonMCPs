// AxonGASUIBindingActions.h
// Action handlers for the gas/ui attribute-binding feature (Phase H1).
// Canonical registration in `gas` namespace; thin alias in `ui` namespace.

#pragma once

#include "AxonGASInternal.h"

class FAxonGASUIBindingActions
{
public:
    /** Registers actions in BOTH `gas` (canonical) and `ui` (alias) namespaces. */
    static void RegisterActions(FAxonToolRegistry& Registry);

    static FAxonActionResult HandleBindWidgetToAttribute(const TSharedPtr<FJsonObject>& Params);
    static FAxonActionResult HandleUnbindWidgetAttribute(const TSharedPtr<FJsonObject>& Params);
    static FAxonActionResult HandleListAttributeBindings(const TSharedPtr<FJsonObject>& Params);
    static FAxonActionResult HandleClearWidgetAttributeBindings(const TSharedPtr<FJsonObject>& Params);
};
