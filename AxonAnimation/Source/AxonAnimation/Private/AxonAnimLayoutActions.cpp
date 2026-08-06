#include "AxonAnimLayoutActions.h"
#include "AxonAssetUtils.h"
#include "AxonParamSchema.h"
#include "IAxonGraphFormatter.h"

#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimationGraph.h"
#include "AnimationStateMachineGraph.h"
#include "AnimStateNode.h"
#include "AnimationStateGraph.h"
#include "EdGraphNode_Comment.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void FAxonAnimLayoutActions::RegisterActions(FAxonToolRegistry& Registry)
{
	Registry.RegisterAction(TEXT("animation"), TEXT("auto_layout"),
		TEXT("Auto-layout nodes in an Animation Blueprint graph. With formatter='auto' (default) it uses "
			 "Blueprint Assist if available and otherwise falls back to a built-in dependency-aware layered "
			 "layout that needs no plugin (available in release builds). formatter='builtin' forces the "
			 "built-in layout. For partitioned/comment-heavy ABPs prefer layout_mode='selected' or "
			 "'new_only' after local edits; avoid layout_mode='all' unless rebuilding the whole graph. "
			 "Asset must be open in the editor."),
		FAxonActionHandler::CreateStatic(&HandleAutoLayout),
		FParamSchemaBuilder()
			.RequiredAssetPath(TEXT("asset_path"), TEXT("Animation Blueprint asset path"))
			.Optional(TEXT("graph_name"), TEXT("string"),
				TEXT("Graph to layout: 'AnimGraph' (default), state machine name, or 'all' for every graph"), TEXT("AnimGraph"))
			.Optional(TEXT("formatter"), TEXT("string"),
				TEXT("Formatter: 'auto' (default, Blueprint Assist if available else built-in), "
					 "'blueprint_assist' (BA or error), 'builtin' (built-in layered layout, no plugin needed), "
					 "'monolith' (alias for 'builtin')"),
				TEXT("auto"))
			.Optional(TEXT("layout_mode"), TEXT("string"),
				TEXT("Layout scope: 'all' (default, full graph), 'selected' (only node_ids), "
					 "'new_only' (nodes at default spawn positions 0,0 or 200,0). selected/new_only use built-in partial layout."),
				TEXT("all"))
			.Optional(TEXT("node_ids"), TEXT("array"), TEXT("Node UObject names to layout when layout_mode='selected'"))
			.Optional(TEXT("column_spacing"), TEXT("number"),
				TEXT("Built-in layout: horizontal spacing between dependency columns (default 320)"), TEXT("320"))
			.Optional(TEXT("row_spacing"), TEXT("number"),
				TEXT("Built-in layout: vertical spacing between nodes within a column (default 180)"), TEXT("180"))
			.Build());

	Registry.RegisterAction(TEXT("animation"), TEXT("get_anim_node_positions"),
		TEXT("Return compact {nodes:[{name,x,y,class}]} layout positions for AnimGraph nodes. "
			 "Token-cheap alternative to full get_nodes dumps. Omits comment nodes."),
		FAxonActionHandler::CreateStatic(&HandleGetAnimNodePositions),
		FParamSchemaBuilder()
			.RequiredAssetPath(TEXT("asset_path"), TEXT("Animation Blueprint asset path"))
			.Optional(TEXT("graph_name"), TEXT("string"),
				TEXT("Graph to query: 'AnimGraph' (default) or state machine name"), TEXT("AnimGraph"))
			.Optional(TEXT("node_names"), TEXT("array"), TEXT("Optional filter — only return these node UObject names"))
			.Build());

	Registry.RegisterAction(TEXT("animation"), TEXT("set_anim_node_position"),
		TEXT("Move one or more AnimGraph nodes by setting NodePosX/NodePosY. Marks the asset dirty."),
		FAxonActionHandler::CreateStatic(&HandleSetAnimNodePosition),
		FParamSchemaBuilder()
			.RequiredAssetPath(TEXT("asset_path"), TEXT("Animation Blueprint asset path"))
			.Optional(TEXT("graph_name"), TEXT("string"),
				TEXT("Graph containing the node(s): 'AnimGraph' (default) or state machine name"), TEXT("AnimGraph"))
			.Optional(TEXT("node_name"), TEXT("string"), TEXT("Single node UObject name (use with position_x/position_y)"))
			.Optional(TEXT("position_x"), TEXT("number"), TEXT("Single-node X position"))
			.Optional(TEXT("position_y"), TEXT("number"), TEXT("Single-node Y position"))
			.Optional(TEXT("nodes"), TEXT("array"), TEXT("Bulk move: [{name,x,y}, ...]"))
			.Build());
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{

/** Collect all formattable graphs from an ABP: the main AnimGraph, all state machine graphs, and state inner graphs. */
void CollectAllGraphs(UAnimBlueprint* ABP, TArray<TPair<FString, UEdGraph*>>& OutGraphs)
{
	for (UEdGraph* Graph : ABP->FunctionGraphs)
	{
		if (!Graph) continue;

		// Add the top-level graph (e.g. AnimGraph)
		OutGraphs.Add(TPair<FString, UEdGraph*>(Graph->GetName(), Graph));

		// Dig into state machine nodes
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
			if (!SMNode) continue;

			UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(SMNode->EditorStateMachineGraph);
			if (!SMGraph) continue;

			// The SM graph itself
			FString SMTitle = SMNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
			int32 NewlineIdx = INDEX_NONE;
			if (SMTitle.FindChar(TEXT('\n'), NewlineIdx))
			{
				SMTitle.LeftInline(NewlineIdx);
			}
			OutGraphs.Add(TPair<FString, UEdGraph*>(SMTitle, SMGraph));

			// Each state's inner graph
			for (UEdGraphNode* SMChild : SMGraph->Nodes)
			{
				UAnimStateNode* StateNode = Cast<UAnimStateNode>(SMChild);
				if (!StateNode || !StateNode->BoundGraph) continue;

				FString StateLabel = FString::Printf(TEXT("%s.%s"), *SMTitle, *StateNode->GetStateName());
				OutGraphs.Add(TPair<FString, UEdGraph*>(StateLabel, StateNode->BoundGraph));
			}
		}
	}
}

/** Find the main AnimGraph (first UAnimationGraph in FunctionGraphs). */
UEdGraph* FindAnimGraph(UAnimBlueprint* ABP)
{
	for (UEdGraph* Graph : ABP->FunctionGraphs)
	{
		if (UAnimationGraph* AG = Cast<UAnimationGraph>(Graph))
		{
			return AG;
		}
	}
	return nullptr;
}

/** Find a state machine graph by display title. */
UEdGraph* FindSMGraphByTitle(UAnimBlueprint* ABP, const FString& MachineName)
{
	for (UEdGraph* Graph : ABP->FunctionGraphs)
	{
		if (!Graph) continue;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
			if (!SMNode) continue;

			FString SMTitle = SMNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
			int32 NewlineIdx = INDEX_NONE;
			if (SMTitle.FindChar(TEXT('\n'), NewlineIdx))
			{
				SMTitle.LeftInline(NewlineIdx);
			}
			if (SMTitle == MachineName)
			{
				return Cast<UAnimationStateMachineGraph>(SMNode->EditorStateMachineGraph);
			}
		}
	}
	return nullptr;
}

/** Format a single graph via IAxonGraphFormatter. Returns a JSON object with results. */
TSharedPtr<FJsonObject> FormatSingleGraph(const FString& GraphLabel, UEdGraph* Graph, bool bExplicitBA, FString& OutError)
{
	bool bBAAvailable = IAxonGraphFormatter::IsAvailable()
		&& IAxonGraphFormatter::Get().SupportsGraph(Graph);

	if (!bBAAvailable)
	{
		if (bExplicitBA)
		{
			OutError = FString::Printf(
				TEXT("Blueprint Assist formatter not available or does not support graph '%s'. "
					 "Ensure Blueprint Assist plugin is installed and the asset is open in the editor."),
				*GraphLabel);
		}
		else
		{
			OutError = FString::Printf(
				TEXT("No formatter available for graph '%s'. Install Blueprint Assist plugin and ensure the asset is open in the editor."),
				*GraphLabel);
		}
		return nullptr;
	}

	int32 NodesFormatted = 0;
	FString FormatError;
	bool bSuccess = IAxonGraphFormatter::Get().FormatGraph(Graph, NodesFormatted, FormatError);

	if (!bSuccess)
	{
		OutError = FString::Printf(TEXT("Formatter failed on graph '%s': %s"), *GraphLabel, *FormatError);
		return nullptr;
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph"), GraphLabel);
	ResultObj->SetNumberField(TEXT("nodes_formatted"), NodesFormatted);
	ResultObj->SetStringField(TEXT("formatter_used"), TEXT("blueprint_assist"));
	return ResultObj;
}

// ---------------------------------------------------------------------------
// Built-in layered layout (no Blueprint Assist dependency).
//
// Pure UEdGraph traversal: builds the connection DAG from each node's input
// pins, assigns a column = max(upstream column) + 1 via longest-path layering
// (Kahn-style relaxation that tolerates cycles), then writes NodePosX/NodePosY
// directly. Pose flows left->right (sources at column 0, the Output Pose / sink
// at the highest column, matching the editor's reading order). This path
// references no Blueprint Assist symbol and compiles with WITH_BLUEPRINT_ASSIST=0.
// ---------------------------------------------------------------------------

/**
 * Layered layout for a single graph. Returns a result JSON object on success,
 * or nullptr with OutError set. Writes NodePosX/NodePosY on every node.
 */
TSharedPtr<FJsonObject> BuiltinLayoutSingleGraph(
	const FString& GraphLabel, UEdGraph* Graph, float ColumnSpacing, float RowSpacing, FString& OutError)
{
	if (!Graph)
	{
		OutError = FString::Printf(TEXT("Graph '%s' is null"), *GraphLabel);
		return nullptr;
	}

	// Stable node set (skip nulls). Index by pointer for the relaxation pass.
	TArray<UEdGraphNode*> Nodes;
	Nodes.Reserve(Graph->Nodes.Num());
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			Nodes.Add(Node);
		}
	}

	if (Nodes.Num() == 0)
	{
		// Empty graph is a no-op success, not an error.
		TSharedPtr<FJsonObject> EmptyObj = MakeShared<FJsonObject>();
		EmptyObj->SetStringField(TEXT("graph"), GraphLabel);
		EmptyObj->SetNumberField(TEXT("nodes_formatted"), 0);
		EmptyObj->SetStringField(TEXT("formatter_used"), TEXT("builtin"));
		return EmptyObj;
	}

	TMap<UEdGraphNode*, int32> IndexOf;
	IndexOf.Reserve(Nodes.Num());
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		IndexOf.Add(Nodes[i], i);
	}

	// Upstream adjacency: for each node, the set of node-indices feeding ANY of
	// its input pins (a node's column must exceed all of these). Mirrors the
	// engine's own pin walk (EdGraphSchema_K2.cpp: Pin->Direction / Pin->LinkedTo
	// / LinkedPin->GetOwningNode()).
	TArray<TSet<int32>> Upstream;
	Upstream.SetNum(Nodes.Num());
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		for (UEdGraphPin* Pin : Nodes[i]->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input)
			{
				continue;
			}
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!LinkedPin)
				{
					continue;
				}
				UEdGraphNode* OwningNode = LinkedPin->GetOwningNode();
				if (const int32* SrcIdx = IndexOf.Find(OwningNode))
				{
					if (*SrcIdx != i) // ignore self-links
					{
						Upstream[i].Add(*SrcIdx);
					}
				}
			}
		}
	}

	// Longest-path layering by iterative relaxation. Column[i] = max upstream
	// column + 1. Bounded by Nodes.Num() iterations so cyclic graphs (which a
	// valid anim graph should not contain) terminate instead of looping forever.
	TArray<int32> Column;
	Column.Init(0, Nodes.Num());
	const int32 MaxIterations = Nodes.Num();
	for (int32 Iter = 0; Iter < MaxIterations; ++Iter)
	{
		bool bChanged = false;
		for (int32 i = 0; i < Nodes.Num(); ++i)
		{
			int32 Desired = 0;
			for (int32 Src : Upstream[i])
			{
				Desired = FMath::Max(Desired, Column[Src] + 1);
			}
			if (Desired > Column[i])
			{
				Column[i] = Desired;
				bChanged = true;
			}
		}
		if (!bChanged)
		{
			break;
		}
	}

	// Group nodes by column, preserving original graph order within a column to
	// keep the result deterministic and avoid overlap.
	int32 MaxColumn = 0;
	for (int32 c : Column)
	{
		MaxColumn = FMath::Max(MaxColumn, c);
	}

	TArray<TArray<int32>> ColumnBuckets;
	ColumnBuckets.SetNum(MaxColumn + 1);
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		ColumnBuckets[Column[i]].Add(i);
	}

	// Assign positions. X increases with column (left-to-right pose flow). Within
	// a column, stack rows top-to-bottom with fixed spacing so no two nodes share
	// a position.
	const int32 SafeColumnSpacing = FMath::Max(1, FMath::RoundToInt(ColumnSpacing));
	const int32 SafeRowSpacing = FMath::Max(1, FMath::RoundToInt(RowSpacing));

	for (int32 c = 0; c <= MaxColumn; ++c)
	{
		const TArray<int32>& Bucket = ColumnBuckets[c];
		const int32 PosX = c * SafeColumnSpacing;
		for (int32 Row = 0; Row < Bucket.Num(); ++Row)
		{
			UEdGraphNode* Node = Nodes[Bucket[Row]];
			Node->NodePosX = PosX;
			Node->NodePosY = Row * SafeRowSpacing;
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph"), GraphLabel);
	ResultObj->SetNumberField(TEXT("nodes_formatted"), Nodes.Num());
	ResultObj->SetNumberField(TEXT("columns"), MaxColumn + 1);
	ResultObj->SetStringField(TEXT("formatter_used"), TEXT("builtin"));
	return ResultObj;
}

/**
 * Per-graph formatter dispatch. Chooses Blueprint Assist or the built-in layered
 * layout based on the requested formatter and BA availability:
 *   - bForceBuiltin: always built-in (formatter 'builtin'/'monolith').
 *   - bExplicitBA: BA only; errors via FormatSingleGraph if BA is unavailable.
 *   - 'auto': BA when available + supports the graph, otherwise built-in fallback.
 * Returns a result JSON object on success, or nullptr with OutError set.
 */
TSharedPtr<FJsonObject> LayoutSingleGraphDispatch(
	const FString& GraphLabel, UEdGraph* Graph,
	bool bExplicitBA, bool bForceBuiltin,
	float ColumnSpacing, float RowSpacing, FString& OutError)
{
	if (bForceBuiltin)
	{
		return BuiltinLayoutSingleGraph(GraphLabel, Graph, ColumnSpacing, RowSpacing, OutError);
	}

	const bool bBAAvailable = IAxonGraphFormatter::IsAvailable()
		&& IAxonGraphFormatter::Get().SupportsGraph(Graph);

	if (bExplicitBA)
	{
		// BA explicitly requested: use BA or surface BA's own error (no fallback).
		return FormatSingleGraph(GraphLabel, Graph, /*bExplicitBA=*/true, OutError);
	}

	// 'auto': prefer BA, fall back to built-in when BA is absent/unsupported.
	if (bBAAvailable)
	{
		return FormatSingleGraph(GraphLabel, Graph, /*bExplicitBA=*/false, OutError);
	}
	return BuiltinLayoutSingleGraph(GraphLabel, Graph, ColumnSpacing, RowSpacing, OutError);
}

/** True when a node is at Axon add_anim_graph_node default spawn coordinates. */
bool IsDefaultSpawnPosition(const UEdGraphNode* Node)
{
	if (!Node) return false;
	return (Node->NodePosX == 0 && Node->NodePosY == 0)
		|| (Node->NodePosX == 200 && Node->NodePosY == 0);
}

/**
 * Partial built-in layout: stack matching nodes vertically at min X/Y anchor,
 * leaving all other nodes untouched.
 */
TSharedPtr<FJsonObject> BuiltinLayoutPartialNodes(
	const FString& GraphLabel, UEdGraph* Graph,
	const TArray<UEdGraphNode*>& NodesToLayout, float RowSpacing, FString& OutError)
{
	if (!Graph)
	{
		OutError = FString::Printf(TEXT("Graph '%s' is null"), *GraphLabel);
		return nullptr;
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph"), GraphLabel);
	ResultObj->SetStringField(TEXT("formatter_used"), TEXT("builtin"));

	if (NodesToLayout.Num() == 0)
	{
		ResultObj->SetNumberField(TEXT("nodes_formatted"), 0);
		return ResultObj;
	}

	int32 MinX = INT32_MAX;
	int32 MinY = INT32_MAX;
	for (UEdGraphNode* Node : NodesToLayout)
	{
		if (!Node) continue;
		MinX = FMath::Min(MinX, Node->NodePosX);
		MinY = FMath::Min(MinY, Node->NodePosY);
	}
	if (MinX == INT32_MAX)
	{
		MinX = 0;
		MinY = 0;
	}

	const int32 SafeRowSpacing = FMath::Max(1, FMath::RoundToInt(RowSpacing));
	for (int32 Row = 0; Row < NodesToLayout.Num(); ++Row)
	{
		UEdGraphNode* Node = NodesToLayout[Row];
		if (!Node) continue;
		Node->NodePosX = MinX;
		Node->NodePosY = MinY + Row * SafeRowSpacing;
	}

	ResultObj->SetNumberField(TEXT("nodes_formatted"), NodesToLayout.Num());
	return ResultObj;
}

/** Resolve a single layout target graph (AnimGraph or state machine graph by title). */
UEdGraph* ResolveLayoutGraph(UAnimBlueprint* ABP, const FString& GraphName, FString& OutLabel, FString& OutError)
{
	if (GraphName.Equals(TEXT("AnimGraph"), ESearchCase::IgnoreCase) || GraphName.IsEmpty())
	{
		UEdGraph* AG = FindAnimGraph(ABP);
		OutLabel = TEXT("AnimGraph");
		if (!AG)
		{
			OutError = TEXT("No AnimGraph found in this Animation Blueprint");
		}
		return AG;
	}

	UEdGraph* SMGraph = FindSMGraphByTitle(ABP, GraphName);
	OutLabel = GraphName;
	if (!SMGraph)
	{
		OutError = FString::Printf(
			TEXT("Graph '%s' not found. Use 'AnimGraph' for the main graph, a state machine name, or 'all'."),
			*GraphName);
	}
	return SMGraph;
}

/** Collect non-comment nodes to partially layout for selected/new_only modes. */
void CollectPartialLayoutNodes(
	UEdGraph* Graph, const FString& LayoutMode, const TSet<FString>& SelectedNodeIds,
	TArray<UEdGraphNode*>& OutNodes)
{
	if (!Graph) return;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node || Cast<UEdGraphNode_Comment>(Node)) continue;

		if (LayoutMode == TEXT("new_only"))
		{
			if (IsDefaultSpawnPosition(Node))
			{
				OutNodes.Add(Node);
			}
		}
		else if (LayoutMode == TEXT("selected"))
		{
			if (SelectedNodeIds.Contains(Node->GetName()))
			{
				OutNodes.Add(Node);
			}
		}
	}
}

TSharedPtr<FJsonObject> LayoutGraphWithMode(
	UAnimBlueprint* ABP, const FString& GraphLabel, UEdGraph* Graph,
	const FString& LayoutMode, const TSet<FString>& SelectedNodeIds,
	bool bExplicitBA, bool bForceBuiltin,
	float ColumnSpacing, float RowSpacing, FString& OutError)
{
	if (LayoutMode == TEXT("selected") || LayoutMode == TEXT("new_only"))
	{
		TArray<UEdGraphNode*> NodesToLayout;
		CollectPartialLayoutNodes(Graph, LayoutMode, SelectedNodeIds, NodesToLayout);
		TSharedPtr<FJsonObject> Result = BuiltinLayoutPartialNodes(GraphLabel, Graph, NodesToLayout, RowSpacing, OutError);
		if (Result)
		{
			ABP->MarkPackageDirty();
		}
		return Result;
	}

	return LayoutSingleGraphDispatch(
		GraphLabel, Graph, bExplicitBA, bForceBuiltin, ColumnSpacing, RowSpacing, OutError);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Action: auto_layout
// ---------------------------------------------------------------------------

FAxonActionResult FAxonAnimLayoutActions::HandleAutoLayout(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	FString GraphName = Params->HasField(TEXT("graph_name")) ? Params->GetStringField(TEXT("graph_name")) : TEXT("AnimGraph");
	FString Formatter = Params->HasField(TEXT("formatter")) ? Params->GetStringField(TEXT("formatter")) : TEXT("auto");
	FString LayoutMode = Params->HasField(TEXT("layout_mode")) ? Params->GetStringField(TEXT("layout_mode")) : TEXT("all");

	if (LayoutMode != TEXT("all") && LayoutMode != TEXT("selected") && LayoutMode != TEXT("new_only"))
	{
		return FAxonActionResult::Error(FString::Printf(
			TEXT("Invalid layout_mode '%s'. Must be 'all', 'selected', or 'new_only'."), *LayoutMode));
	}

	TSet<FString> SelectedNodeIds;
	if (LayoutMode == TEXT("selected"))
	{
		const TArray<TSharedPtr<FJsonValue>>* NodeIdsArr = nullptr;
		if (Params->TryGetArrayField(TEXT("node_ids"), NodeIdsArr) && NodeIdsArr)
		{
			for (const TSharedPtr<FJsonValue>& V : *NodeIdsArr)
			{
				if (V.IsValid())
				{
					const FString Id = V->AsString();
					if (!Id.IsEmpty())
					{
						SelectedNodeIds.Add(Id);
					}
				}
			}
		}
		if (SelectedNodeIds.Num() == 0)
		{
			return FAxonActionResult::Error(TEXT("layout_mode='selected' requires a non-empty 'node_ids' array."));
		}
	}

	// Validate formatter param. 'monolith' is accepted as an alias for 'builtin'
	// (the built-in layered layout IS the Axon-native formatter now).
	if (Formatter != TEXT("auto") && Formatter != TEXT("blueprint_assist")
		&& Formatter != TEXT("builtin") && Formatter != TEXT("monolith"))
	{
		return FAxonActionResult::Error(FString::Printf(
			TEXT("Unknown formatter '%s'. Supported: 'auto', 'blueprint_assist', 'builtin', 'monolith'"), *Formatter));
	}

	// Built-in layout spacing (only used by the builtin path).
	const float ColumnSpacing = Params->HasField(TEXT("column_spacing"))
		? static_cast<float>(Params->GetNumberField(TEXT("column_spacing"))) : 320.0f;
	const float RowSpacing = Params->HasField(TEXT("row_spacing"))
		? static_cast<float>(Params->GetNumberField(TEXT("row_spacing"))) : 180.0f;

	// Load the AnimBlueprint
	UAnimBlueprint* ABP = FAxonAssetUtils::LoadAssetByPath<UAnimBlueprint>(AssetPath);
	if (!ABP)
	{
		return FAxonActionResult::Error(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AssetPath));
	}

	const bool bExplicitBA = (Formatter == TEXT("blueprint_assist"));
	bool bForceBuiltin = (Formatter == TEXT("builtin") || Formatter == TEXT("monolith"));
	if (LayoutMode == TEXT("selected") || LayoutMode == TEXT("new_only"))
	{
		bForceBuiltin = true;
	}

	// --- "all" mode: format every graph ---
	if (GraphName.Equals(TEXT("all"), ESearchCase::IgnoreCase))
	{
		TArray<TPair<FString, UEdGraph*>> AllGraphs;
		CollectAllGraphs(ABP, AllGraphs);

		if (AllGraphs.Num() == 0)
		{
			return FAxonActionResult::Error(TEXT("No graphs found in this Animation Blueprint"));
		}

		TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(TEXT("asset_path"), AssetPath);
		Root->SetStringField(TEXT("mode"), TEXT("all"));
		Root->SetStringField(TEXT("layout_mode"), LayoutMode);

		TArray<TSharedPtr<FJsonValue>> ResultsArr;
		TArray<TSharedPtr<FJsonValue>> ErrorsArr;
		int32 TotalFormatted = 0;

		for (const auto& Pair : AllGraphs)
		{
			FString Error;
			TSharedPtr<FJsonObject> GraphResult = LayoutGraphWithMode(
				ABP, Pair.Key, Pair.Value, LayoutMode, SelectedNodeIds,
				bExplicitBA, bForceBuiltin, ColumnSpacing, RowSpacing, Error);
			if (GraphResult)
			{
				TotalFormatted++;
				ResultsArr.Add(MakeShared<FJsonValueObject>(GraphResult));
			}
			else
			{
				TSharedPtr<FJsonObject> ErrObj = MakeShared<FJsonObject>();
				ErrObj->SetStringField(TEXT("graph"), Pair.Key);
				ErrObj->SetStringField(TEXT("error"), Error);
				ErrorsArr.Add(MakeShared<FJsonValueObject>(ErrObj));
			}
		}

		Root->SetArrayField(TEXT("formatted"), ResultsArr);
		Root->SetNumberField(TEXT("graphs_formatted"), TotalFormatted);
		Root->SetNumberField(TEXT("graphs_total"), AllGraphs.Num());

		if (ErrorsArr.Num() > 0)
		{
			Root->SetArrayField(TEXT("errors"), ErrorsArr);
		}

		if (TotalFormatted == 0)
		{
			// All graphs failed — return error with details. With the built-in
			// fallback this only happens when BA is explicitly required and absent.
			return FAxonActionResult::Error(FString::Printf(
				TEXT("Failed to format any of %d graphs. Try formatter='builtin' (no plugin needed), "
					 "or install Blueprint Assist and ensure the asset is open in the editor."),
				AllGraphs.Num()));
		}

		return FAxonActionResult::Success(Root);
	}

	// --- Single graph mode ---
	UEdGraph* TargetGraph = nullptr;
	FString GraphLabel;
	FString GraphError;
	TargetGraph = ResolveLayoutGraph(ABP, GraphName, GraphLabel, GraphError);
	if (!TargetGraph)
	{
		return FAxonActionResult::Error(GraphError);
	}

	FString Error;
	TSharedPtr<FJsonObject> GraphResult = LayoutGraphWithMode(
		ABP, GraphLabel, TargetGraph, LayoutMode, SelectedNodeIds,
		bExplicitBA, bForceBuiltin, ColumnSpacing, RowSpacing, Error);
	if (!GraphResult)
	{
		return FAxonActionResult::Error(Error);
	}

	// Wrap in a top-level result
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("asset_path"), AssetPath);
	Root->SetStringField(TEXT("graph"), GraphLabel);
	Root->SetStringField(TEXT("layout_mode"), LayoutMode);
	Root->SetNumberField(TEXT("nodes_formatted"), GraphResult->GetNumberField(TEXT("nodes_formatted")));
	Root->SetStringField(TEXT("formatter_used"), GraphResult->GetStringField(TEXT("formatter_used")));

	return FAxonActionResult::Success(Root);
}

// ---------------------------------------------------------------------------
// Action: get_anim_node_positions
// ---------------------------------------------------------------------------

FAxonActionResult FAxonAnimLayoutActions::HandleGetAnimNodePositions(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	FString GraphName = Params->HasField(TEXT("graph_name")) ? Params->GetStringField(TEXT("graph_name")) : TEXT("AnimGraph");

	UAnimBlueprint* ABP = FAxonAssetUtils::LoadAssetByPath<UAnimBlueprint>(AssetPath);
	if (!ABP)
	{
		return FAxonActionResult::Error(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AssetPath));
	}

	FString GraphLabel;
	FString GraphError;
	UEdGraph* Graph = ResolveLayoutGraph(ABP, GraphName, GraphLabel, GraphError);
	if (!Graph)
	{
		return FAxonActionResult::Error(GraphError);
	}

	TSet<FString> NameFilter;
	const TArray<TSharedPtr<FJsonValue>>* NodeNamesArr = nullptr;
	if (Params->TryGetArrayField(TEXT("node_names"), NodeNamesArr) && NodeNamesArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *NodeNamesArr)
		{
			if (V.IsValid())
			{
				const FString Name = V->AsString();
				if (!Name.IsEmpty())
				{
					NameFilter.Add(Name);
				}
			}
		}
	}

	TArray<TSharedPtr<FJsonValue>> NodesArr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node || Cast<UEdGraphNode_Comment>(Node)) continue;
		if (NameFilter.Num() > 0 && !NameFilter.Contains(Node->GetName())) continue;

		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("name"), Node->GetName());
		NodeObj->SetNumberField(TEXT("x"), Node->NodePosX);
		NodeObj->SetNumberField(TEXT("y"), Node->NodePosY);
		NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
		NodesArr.Add(MakeShared<FJsonValueObject>(NodeObj));
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("asset_path"), AssetPath);
	Root->SetStringField(TEXT("graph"), GraphLabel);
	Root->SetArrayField(TEXT("nodes"), NodesArr);
	return FAxonActionResult::Success(Root);
}

// ---------------------------------------------------------------------------
// Action: set_anim_node_position
// ---------------------------------------------------------------------------

FAxonActionResult FAxonAnimLayoutActions::HandleSetAnimNodePosition(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	FString GraphName = Params->HasField(TEXT("graph_name")) ? Params->GetStringField(TEXT("graph_name")) : TEXT("AnimGraph");

	UAnimBlueprint* ABP = FAxonAssetUtils::LoadAssetByPath<UAnimBlueprint>(AssetPath);
	if (!ABP)
	{
		return FAxonActionResult::Error(FString::Printf(TEXT("AnimBlueprint not found: %s"), *AssetPath));
	}

	FString GraphLabel;
	FString GraphError;
	UEdGraph* Graph = ResolveLayoutGraph(ABP, GraphName, GraphLabel, GraphError);
	if (!Graph)
	{
		return FAxonActionResult::Error(GraphError);
	}

	struct FNodePositionSpec
	{
		FString Name;
		int32 X = 0;
		int32 Y = 0;
	};
	TArray<FNodePositionSpec> Specs;

	const TArray<TSharedPtr<FJsonValue>>* BulkArr = nullptr;
	if (Params->TryGetArrayField(TEXT("nodes"), BulkArr) && BulkArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *BulkArr)
		{
			const TSharedPtr<FJsonObject> Entry = V.IsValid() ? V->AsObject() : nullptr;
			if (!Entry.IsValid()) continue;

			FNodePositionSpec Spec;
			Spec.Name = Entry->GetStringField(TEXT("name"));
			if (Spec.Name.IsEmpty()) continue;
			Spec.X = Entry->HasField(TEXT("x")) ? static_cast<int32>(Entry->GetNumberField(TEXT("x"))) : 0;
			Spec.Y = Entry->HasField(TEXT("y")) ? static_cast<int32>(Entry->GetNumberField(TEXT("y"))) : 0;
			Specs.Add(Spec);
		}
	}
	else
	{
		FString NodeName = Params->GetStringField(TEXT("node_name"));
		if (NodeName.IsEmpty())
		{
			return FAxonActionResult::Error(
				TEXT("Provide either 'nodes' [{name,x,y},...] or 'node_name' with position_x/position_y."));
		}
		FNodePositionSpec Spec;
		Spec.Name = NodeName;
		Spec.X = Params->HasField(TEXT("position_x")) ? static_cast<int32>(Params->GetNumberField(TEXT("position_x"))) : 0;
		Spec.Y = Params->HasField(TEXT("position_y")) ? static_cast<int32>(Params->GetNumberField(TEXT("position_y"))) : 0;
		Specs.Add(Spec);
	}

	if (Specs.Num() == 0)
	{
		return FAxonActionResult::Error(TEXT("No node position updates specified."));
	}

	TMap<FString, UEdGraphNode*> NodeByName;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			NodeByName.Add(Node->GetName(), Node);
		}
	}

	TArray<TSharedPtr<FJsonValue>> UpdatedArr;
	TArray<TSharedPtr<FJsonValue>> MissingArr;

	Graph->Modify();
	for (const FNodePositionSpec& Spec : Specs)
	{
		if (UEdGraphNode* const* Found = NodeByName.Find(Spec.Name))
		{
			(*Found)->NodePosX = Spec.X;
			(*Found)->NodePosY = Spec.Y;

			TSharedPtr<FJsonObject> UpdatedObj = MakeShared<FJsonObject>();
			UpdatedObj->SetStringField(TEXT("name"), Spec.Name);
			UpdatedObj->SetNumberField(TEXT("x"), Spec.X);
			UpdatedObj->SetNumberField(TEXT("y"), Spec.Y);
			UpdatedArr.Add(MakeShared<FJsonValueObject>(UpdatedObj));
		}
		else
		{
			MissingArr.Add(MakeShared<FJsonValueString>(Spec.Name));
		}
	}

	ABP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("asset_path"), AssetPath);
	Root->SetStringField(TEXT("graph"), GraphLabel);
	Root->SetArrayField(TEXT("updated"), UpdatedArr);
	if (MissingArr.Num() > 0)
	{
		Root->SetArrayField(TEXT("missing"), MissingArr);
	}
	return FAxonActionResult::Success(Root);
}
