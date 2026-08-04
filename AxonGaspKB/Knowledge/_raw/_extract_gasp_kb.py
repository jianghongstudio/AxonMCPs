"""One-shot GASP KB extraction via Axon MCP. Writes only under Knowledge/_raw/."""
import json
import re
import urllib.error
import urllib.request
from pathlib import Path

MCP_URL = "http://127.0.0.1:9320/mcp"
RAW = Path(__file__).resolve().parent
INI_PATH = Path(r"D:\GameAnimationSample\Config\DefaultEngine.ini")

_req_id = 0
failures: list[dict] = []
written: list[str] = []


def mcp_call(tool: str, arguments: dict) -> dict:
    global _req_id
    _req_id += 1
    payload = {
        "jsonrpc": "2.0",
        "id": _req_id,
        "method": "tools/call",
        "params": {"name": tool, "arguments": arguments},
    }
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        MCP_URL, data=data, headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            body = json.loads(resp.read().decode("utf-8"))
    except Exception as exc:
        return {"success": False, "error": str(exc), "tool": tool, "arguments": arguments}
    if "error" in body:
        return {"success": False, "error": body["error"], "tool": tool, "arguments": arguments}
    result = body.get("result", {})
    content = result.get("content", [])
    if content and isinstance(content, list) and content[0].get("type") == "text":
        text = content[0].get("text", "")
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return {"success": True, "raw_text": text}
    return result if result else body


def save_json(rel_path: str, data) -> Path:
    path = RAW / rel_path
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    written.append(str(path))
    return path


def safe_name(asset_path: str) -> str:
    return asset_path.replace("/Game/", "").replace("/", "__")


def is_failure(result: dict) -> bool:
    if not isinstance(result, dict):
        return False
    if result.get("success") is False and "error" in result:
        return True
    if "error" in result and "success" not in result:
        return True
    return False


def dump_action(
    tool: str,
    action: str,
    params: dict,
    rel_path: str,
    asset_path: str | None = None,
) -> dict:
    args = {"action": action, "params": params}
    result = mcp_call(tool, args)
    payload = {"tool": tool, "action": action, "params": params, "result": result}
    save_json(rel_path, payload)
    if is_failure(result):
        failures.append(
            {
                "asset_path": asset_path or params.get("asset_path"),
                "tool": tool,
                "action": action,
                "error": result.get("error", result),
            }
        )
    return result


def parse_ddcvars(ini_text: str) -> list[dict]:
    ddcvars: list[dict] = []
    for line in ini_text.splitlines():
        line = line.strip()
        if not line.startswith("+CVarsArray="):
            continue
        body = line[len("+CVarsArray=") :]
        entry: dict = {}
        for part in body.strip("()").split(","):
            if "=" not in part:
                continue
            key, value = part.split("=", 1)
            key = key.strip()
            value = value.strip().strip('"')
            if key == "Type":
                entry["type"] = value
            elif key == "Name":
                entry["name"] = value
            elif key == "ToolTip":
                entry["tooltip"] = value
            elif key == "DefaultValueFloat":
                entry["default_float"] = float(value)
            elif key == "DefaultValueInt":
                entry["default_int"] = int(value)
            elif key == "DefaultValueBool":
                entry["default_bool"] = value.lower() == "true"
        name = entry.get("name", "")
        if name.lower().startswith("ddcvar"):
            ddcvars.append(entry)
    return ddcvars


def main() -> None:
    searches = {
        "pose_search_database": "PoseSearchDatabase",
        "pose_search_schema": "PoseSearchSchema",
        "cht_pose_search": "CHT_PoseSearch",
        "cht_traversal": "CHT_Traversal",
        "camera": "CameraAsset OR CameraDirector OR CHT_CameraRig",
        "movement_modes": "MovementModes",
    }
    asset_index = {}
    for key, query in searches.items():
        asset_index[key] = mcp_call(
            "project_query", {"action": "search", "params": {"query": query, "limit": 200}}
        )
    save_json("asset_index/search_results.json", asset_index)

    pss_mm = [
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Default",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Idle",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Stop",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Jump",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Traversal",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Relaxed_Loops",
    ]
    pss_sm = [
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSS_SM_CMC_Idles",
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSS_SM_CMC_LocoLoops",
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSS_SM_Mover_Loops",
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSS_SM_Mover_Transitions",
    ]
    psd_mm = [
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/Dense/PSD_Dense_Stand_Walk_Loops",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/Sparse/PSD_Sparse_Stand_Walk_Loops",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/PSD_Traversal",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/Relaxed/PSD_Relaxed_Stand_Idles",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/Dense/PSD_Dense_Stand_Sprint_Loops",
    ]
    psd_sm = [
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSD_SM_CMC_Idles",
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSD_SM_CMC_Loops",
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSD_SM_CMC_Transitions",
        "/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSD_SM_Mover_Loops",
    ]

    for path in pss_mm + pss_sm:
        dump_action(
            "animation_query",
            "get_pose_search_schema",
            {"asset_path": path},
            f"pose_search/schemas/{safe_name(path)}.json",
            path,
        )
    for path in psd_mm + psd_sm:
        dump_action(
            "animation_query",
            "get_pose_search_database",
            {"asset_path": path},
            f"pose_search/databases/{safe_name(path)}.json",
            path,
        )
        dump_action(
            "animation_query",
            "get_database_stats",
            {"asset_path": path},
            f"pose_search/stats/{safe_name(path)}.json",
            path,
        )

    blueprint_assets = [
        "/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter",
        "/Game/Blueprints/Cameras/CameraDirector_SandboxCharacter",
        "/Game/Blueprints/Cameras/CHT_CameraRig",
        "/Game/Blueprints/MovementModes/BP_MovementMode_Walking",
        "/Game/Blueprints/MovementModes/BP_MovementMode_Falling",
        "/Game/Blueprints/MovementModes/BP_MovementMode_Slide",
        "/Game/Blueprints/MovementModes/BP_MovementTransition_ToSlide",
        "/Game/Blueprints/MovementModes/BP_MovementTransition_FromSlide",
    ]
    for path in blueprint_assets:
        dump_action(
            "blueprint_query",
            "get_blueprint_info",
            {"asset_path": path},
            f"blueprints/{safe_name(path)}__info.json",
            path,
        )
        dump_action(
            "blueprint_query",
            "get_cdo_properties",
            {"asset_path": path},
            f"blueprints/{safe_name(path)}__cdo.json",
            path,
        )

    data_structs = [
        "/Game/Blueprints/Data/S_CharacterPropertiesForAnimation",
        "/Game/Blueprints/Data/S_CharacterPropertiesForCamera",
        "/Game/Blueprints/Data/S_CharacterPropertiesForTraversal",
        "/Game/Blueprints/Data/S_ChooserOutputs",
        "/Game/Blueprints/Data/S_TraversalChooserInputs",
        "/Game/Blueprints/Data/S_TraversalChooserOutputs",
        "/Game/Blueprints/Data/S_PlayerInputState",
        "/Game/Blueprints/Data/S_MovementDirectionThresholds",
    ]
    for path in data_structs:
        dump_action(
            "describe_query",
            "schema",
            {"target_namespace": "describe", "target": path},
            f"structs/{safe_name(path)}__schema.json",
            path,
        )
        dump_action(
            "blueprint_query",
            "get_cdo_properties",
            {"asset_path": path},
            f"structs/{safe_name(path)}__cdo.json",
            path,
        )

    choosers = [
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Dense",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Sparse",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Relaxed",
        "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_ExtremeSparse",
        "/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC",
        "/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_Mover",
        "/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalAnims_PoseMatch",
        "/Game/Blueprints/Cameras/CHT_CameraRig",
    ]
    for path in choosers:
        dump_action(
            "chooser_query",
            "inspect_chooser",
            {"asset_path": path, "include_cells": True, "recursive": True},
            f"choosers/{safe_name(path)}.json",
            path,
        )

    abp = "/Game/Blueprints/SandboxCharacter_CMC_ABP"
    for fn in [
        "Update_CVarDrivenVariables",
        "Update_States",
        "Update_Logic",
        "ShouldTurnInPlace",
        "IsPivoting",
        "IsMoving",
    ]:
        dump_action(
            "blueprint_query",
            "get_graph_summary",
            {"asset_path": abp, "graph_name": fn},
            f"abp/cmc/{fn}.json",
            f"{abp}:{fn}",
        )

    ini_text = INI_PATH.read_text(encoding="utf-8")
    ddcvars = parse_ddcvars(ini_text)
    save_json(
        "config/ddcvars.json",
        {"source": str(INI_PATH), "count": len(ddcvars), "cvars": ddcvars},
    )

    save_json(
        "_manifest.json",
        {
            "written_files": sorted(written),
            "written_count": len(written),
            "failures": failures,
            "failure_count": len(failures),
        },
    )

    print(json.dumps({"written": len(written), "failures": len(failures)}, indent=2))
    for item in failures:
        print("FAIL:", item.get("asset_path"), item.get("action"), item.get("error"))


if __name__ == "__main__":
    main()
