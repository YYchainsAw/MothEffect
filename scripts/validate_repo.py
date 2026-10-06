"""Static repository checks; does not compile or run Unreal Engine."""
import json
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
tracked = subprocess.check_output(
    ["git", "ls-files", "-z"], cwd=root
).decode("utf-8").split("\0")
tracked = [name for name in tracked if name]
errors = []


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate key: {key}")
        result[key] = value
    return result


def invalid_constant(value):
    raise ValueError(f"invalid JSON constant: {value}")


for name in tracked:
    path = root / name
    if path.suffix in {".json", ".uproject", ".uplugin"}:
        try:
            json.loads(path.read_text(encoding="utf-8-sig"),
                       object_pairs_hook=unique_keys,
                       parse_constant=invalid_constant)
        except (ValueError, OSError) as error:
            errors.append(f"{name}: {error}")
    if path.suffix == ".md" and (name == "README.md" or name.startswith("Docs/")):
        text = path.read_text(encoding="utf-8-sig")
        # Check inline links/images in prose; skip fenced examples and inline code.
        text = re.sub(r"^([ \t]*)(`{3,}|~{3,})[^\n]*\n.*?^\1\2[ \t]*$",
                      "", text, flags=re.M | re.S)
        text = re.sub(r"`[^`\n]+`", "", text)
        for match in re.finditer(r"!?\[[^\]\n]*\]\(([^\s)]+)(?:\s+\"[^\"]*\")?\)", text):
            target = match.group(1).strip("<>")
            parts = urlsplit(target)
            if parts.scheme or parts.netloc or not parts.path:
                continue
            destination = ((root if parts.path.startswith("/") else path.parent)
                           / unquote(parts.path).lstrip("/")).resolve()
            if not destination.exists():
                errors.append(f"{name}: missing relative-link target: {target}")

assets = [name for name in tracked if Path(name).suffix in {".uasset", ".umap"}]
if assets:
    raw = subprocess.check_output(
        ["git", "check-attr", "-z", "--stdin", "filter", "diff", "merge", "text"],
        input=("\0".join(assets) + "\0").encode(), cwd=root
    ).decode().split("\0")
    expected = {"filter": "lfs", "diff": "lfs", "merge": "lfs", "text": "unset"}
    for index in range(0, len(raw) - 1, 3):
        name, attribute, value = raw[index:index + 3]
        if value != expected[attribute]:
            errors.append(f"{name}: {attribute}={value}; expected {expected[attribute]}")

if errors:
    print("\n".join(errors))
    sys.exit(1)
print(f"Static checks passed for {len(tracked)} tracked files and {len(assets)} UE assets.")
print("Checked JSON, inline relative-link targets, and LFS attributes; no UE build or gameplay tests.")
