// SPDX-License-Identifier: MIT
// FAxonDryRunGuard — RAII-ish helper used inside existing write actions
// to add `dry_run: true` support with minimal boilerplate. Phase 0 primitive.

#pragma once

#include "CoreMinimal.h"
#include "AxonBulkFillTypes.h"

struct FAxonActionResult;

/**
 * Helper used inside existing write actions to add `dry_run: true` support
 * with minimal boilerplate.
 *
 * Usage:
 *   FAxonActionResult FMyActions::HandleFoo(const TSharedPtr<FJsonObject>& Params)
 *   {
 *       FAxonDryRunGuard Guard(Params);
 *       // ... do all validation ...
 *       if (Guard.IsDryRun()) { return Guard.MakeDryRunResponse(MyReport); }
 *       // ... commit ...
 *   }
 */
class AXONCORE_API FAxonDryRunGuard
{
public:
	explicit FAxonDryRunGuard(const TSharedPtr<FJsonObject>& Params);

	bool IsDryRun() const { return bDryRun; }
	bool IsStrict() const { return bStrict; }

	/** Build a success-shaped JSON-RPC response carrying the report payload. */
	FAxonActionResult MakeDryRunResponse(const FAxonDryRunReport& Report) const;

	/** Convert a report into a JSON object (extracted for unit-testability). */
	static TSharedPtr<FJsonObject> ReportToJson(const FAxonDryRunReport& Report);

private:
	bool bDryRun = false;
	bool bStrict = false;
};
