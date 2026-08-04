"""Supplement GASP KB dumps for soft-failure cases."""
import json
import urllib.request
from pathlib import Path

MCP_URL = "http://127.0.0.1:9320/mcp"
RAW = Path(__file__).resolve().parent
_req_id = 0
soft_failures = []
written = []


def mcp_call(tool, arguments):
    global _req_id
    _req_id += 1
    payload = {
        "jsonrpc": "2.0",
        "id": _req_id,
        "method": "tools/call",
        "params": {"name": tool, "arguments": arguments},
    }
    req = urllib.request.Request(
        MCP_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=120) as resp:
        body = json.loads(resp.read().decode("utf-8"))
    result = body.get("result", {})
    content = result.get("content", [])
    if content and content[0].get("type") == "text":
        text = content[0]["text"]
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return {"success": True, "raw_text": text}
    return result


def save(rel, data):
    path = RAW / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    written.append(str(path))


def safe_name(p):
    return p.replace("/Game/", "").replace("/", "__")


def dump(tool, action, params, rel, note=None):
    result = mcp_call(tool, {"action": action, "params": params})
    save(rel, {"tool": tool, "action": action, "params": params, "result": result})
    text = json.dumps(result)
    if "not found" in text.lower() or (isinstance(result, dict) and result.get("success") is False):
        soft_failures.append({"note": note, "params": params, "result": result})
    return result


structs = [
    "/Game/Blueprints/Data/S_CharacterPropertiesForAnimation",
    "/Game/Blueprints/Data/S_CharacterPropertiesForCamera",
    "/Game/Blueprints/Data/S_CharacterPropertiesForTraversal",
    "/Game/Blueprints/Data/S_ChooserOutputs",
    "/Game/Blueprints/Data/S_TraversalChooserInputs",
    "/Game/Blueprints/Data/S_TraversalChooserOutputs",
    "/Game/Blueprints/Data/S_PlayerInputState",
    "/Game/Blueprints/Data/S_MovementDirectionThresholds",
]
for p in structs:
    dump(
        "blueprint_query",
        "describe_cdo_schema",
        {"asset_path": p},
        f"structs/{safe_name(p)}__describe_schema.json",
        p,
    )

# Non-blueprint assets: asset details + export snippets
for p in [
    "/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter",
    "/Game/Blueprints/Cameras/CHT_CameraRig",
    "/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalAnims_PoseMatch",
]:
    dump(
        "project_query",
        "get_asset_details",
        {"asset_path": p},
        f"asset_details/{safe_name(p)}.json",
        p,
    )

# Redirect target chooser
dump(
    "chooser_query",
    "inspect_chooser",
    {
        "asset_path": "/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC",
        "include_cells": True,
        "recursive": True,
    },
    "choosers/Characters__UEFN_Mannequin__Animations__Traversal__CHT_TraversalAnims_PoseMatch__redirect_target.json",
    "redirect from CHT_TraversalAnims_PoseMatch",
)

manifest_path = RAW / "_manifest.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
manifest["written_files"] = sorted(set(manifest.get("written_files", []) + written))
manifest["written_count"] = len(manifest["written_files"])
manifest["soft_failures"] = soft_failures
manifest["soft_failure_count"] = len(soft_failures)
manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
print(json.dumps({"added": len(written), "soft_failures": len(soft_failures)}, indent=2))
