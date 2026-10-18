"""Prepare a rendered A/B capture project without editing source assets.

Run with ordinary Python: stage_arena_visual_pass.py prepare | sync-binaries | verify
Run the two captures in separate D3D12 editor processes; see the capture script.
Content directories are junctions treated as read-only by the capture workflow.
Neither script saves an Unreal package. Source asset hashes are checked after use.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Saved/Automation/ArenaVisualPass"
STAGE = OUTPUT / "Stage_20261018_01"
MAP = "Content/MothEffect/Maps/L_Arena_SoulCity.umap"


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def inventory(base, relative_to):
    return {p.relative_to(relative_to).as_posix(): sha(p)
            for p in sorted(base.rglob("*")) if p.is_file()}


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def prepare():
    if STAGE.exists():
        raise RuntimeError("Stage already exists; reuse it explicitly rather than replacing it")
    for folder in ("Config", "Binaries", "Content/MothEffect"):
        if not (ROOT / folder).is_dir():
            raise RuntimeError("Missing source directory: " + folder)
    if not (ROOT / MAP).is_file():
        raise RuntimeError("Missing saved production map: " + MAP)
    protected = inventory(ROOT / "Content/MothEffect", ROOT)
    write(OUTPUT / "OwnedAssetsBefore.json", protected)
    backup = OUTPUT / "Backup/L_Arena_SoulCity.umap"
    backup.parent.mkdir(parents=True, exist_ok=True)
    if backup.exists() and sha(backup) != protected[MAP]:
        raise RuntimeError("A different original-map backup already exists")
    if not backup.exists():
        shutil.copy2(ROOT / MAP, backup)
    STAGE.mkdir()
    for folder in ("Config", "Binaries"):
        shutil.copytree(ROOT / folder, STAGE / folder)
    descriptor = json.loads((ROOT / "MothEffect.uproject").read_text(encoding="utf-8-sig"))
    plugins = descriptor.setdefault("Plugins", [])
    for name in ("PythonScriptPlugin", "EditorScriptingUtilities"):
        existing = next((p for p in plugins if p["Name"] == name), None)
        if existing:
            existing["Enabled"] = True
        else:
            plugins.append(dict(Name=name, Enabled=True))
    write(STAGE / "MothEffect.uproject", descriptor)
    with (STAGE / "Config/DefaultEngine.ini").open("a", encoding="utf-8") as stream:
        stream.write("\n[/Script/EngineSettings.GameMapsSettings]\n"
                     "EditorStartupMap=/Engine/Maps/Entry\n")
    with (STAGE / "Config/DefaultEditorPerProjectUserSettings.ini").open("a", encoding="utf-8") as stream:
        stream.write("\n[/Script/UnrealEd.EditorLoadingSavingSettings]\n"
                     "bAutoSaveEnable=False\nbAutoSaveMaps=False\nbAutoSaveContent=False\n")
    content = STAGE / "Content"
    content.mkdir()
    for source in sorted((ROOT / "Content").iterdir()):
        destination = content / source.name
        if source.is_dir():
            # Use native PowerShell throughout. Never remove, move, or mutate junction targets.
            def literal(path):
                return "'" + str(path).replace("'", "''") + "'"
            subprocess.run(["powershell", "-NoProfile", "-Command",
                            "New-Item -ItemType Junction -Path " + literal(destination)
                            + " -Target " + literal(source) + " | Out-Null"], check=True)
        elif source.is_file():
            shutil.copy2(source, destination)
    write(OUTPUT / "StageInfo.json", dict(
        source_root=str(ROOT), stage=str(STAGE), project=str(STAGE / "MothEffect.uproject"),
        map=MAP, map_sha256=protected[MAP], protected_files=len(protected),
        content_access="Read-only workflow via junctions; package saving is prohibited",
        baseline_config=inventory(STAGE / "Config", STAGE),
        binaries=inventory(STAGE / "Binaries", STAGE)))
    print(json.dumps(dict(stage=str(STAGE), project=str(STAGE / "MothEffect.uproject"),
                          protected_files=len(protected), map_sha256=protected[MAP])))


def validate_stage():
    info = json.loads((OUTPUT / "StageInfo.json").read_text(encoding="utf-8"))
    if Path(info["source_root"]).resolve() != ROOT or Path(info["stage"]).resolve() != STAGE:
        raise RuntimeError("Stage marker does not match this source project")
    return info


def verify():
    validate_stage()
    before = json.loads((OUTPUT / "OwnedAssetsBefore.json").read_text(encoding="utf-8"))
    after = inventory(ROOT / "Content/MothEffect", ROOT)
    changed = sorted(path for path in before.keys() | after.keys() if before.get(path) != after.get(path))
    report = dict(success=not changed, changed_owned_assets=changed, protected_files=len(before),
                  map_sha256_before=before[MAP], map_sha256_after=after.get(MAP),
                  original_map_backup_sha256=sha(OUTPUT / "Backup/L_Arena_SoulCity.umap"))
    write(OUTPUT / "AssetVerification.json", report)
    print(json.dumps(report))
    if changed:
        raise RuntimeError("Source content changed: " + ", ".join(changed))


def sync_binaries():
    info = validate_stage()
    shutil.copytree(ROOT / "Binaries", STAGE / "Binaries", dirs_exist_ok=True)
    current = inventory(ROOT / "Binaries", ROOT)
    if any(not (STAGE / path).is_file() or sha(STAGE / path) != digest
           for path, digest in current.items()):
        raise RuntimeError("Staged binaries differ from source")
    info["binaries"] = current
    write(OUTPUT / "StageInfo.json", info)
    print("Synchronized binaries to " + str(STAGE))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "sync-binaries", "verify"))
    globals()[parser.parse_args().action.replace("-", "_")]()
