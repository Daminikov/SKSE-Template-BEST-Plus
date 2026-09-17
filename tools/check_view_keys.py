#!/usr/bin/env python3
"""Checks that translation keys line up between code, the PrismaUI view and translations/.

    python tools/check_view_keys.py
    python tools/check_view_keys.py --strict    # also fail on keys that nothing uses

Checks:
  * every key used in src/ / include/ (Loc::Get) and in view/*.html (data-i18n, data-i18n-placeholder)
    exists in translations/english.txt  -> error (would show a raw $Key on screen)
  * every key follows the convention  $<Mod>_<Surface>_<Element>, Surface in Menu | View | Notice
  * keys that nothing uses (candidates for deletion) -> warning, error with --strict
"""
import pathlib
import re
import sys

SURFACES = ("Menu", "View", "Notice")
CODE_KEY = re.compile(r'Loc::Get\(\s*"(\$[A-Za-z0-9_]+)"')
HTML_KEY = re.compile(r'data-i18n(?:-placeholder)?="(\$[A-Za-z0-9_]+)"')
CONVENTION = re.compile(r"^\$[A-Za-z0-9]+_(Menu|View|Notice)_[A-Za-z0-9_]+$")

def defined_keys(root: pathlib.Path) -> set[str]:
    raw = (root / "translations" / "english.txt").read_bytes()
    if not raw.startswith(b"\xff\xfe"):
        raise SystemExit("translations/english.txt: not UTF-16 LE with BOM")
    keys = set()
    for line in raw[2:].decode("utf-16-le").splitlines():
        if line.startswith("$"):
            keys.add(line.split("\t", 1)[0])
    return keys

def used_keys(root: pathlib.Path) -> dict[str, set[str]]:
    used: dict[str, set[str]] = {}
    for pattern, folder, suffixes in (("code", "src", (".cpp",)), ("headers", "include", (".h",)),
                                      ("view", "view", (".html",))):
        found: set[str] = set()
        for path in sorted((root / folder).rglob("*")):
            if path.suffix not in suffixes:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            regex = HTML_KEY if path.suffix == ".html" else CODE_KEY
            found |= set(regex.findall(text))
        if found:
            used[pattern] = found
    return used

def main() -> int:
    strict = "--strict" in sys.argv
    root = pathlib.Path(__file__).resolve().parent.parent

    defined = defined_keys(root)
    used = used_keys(root)
    all_used = set().union(*used.values()) if used else set()

    failures: list[str] = []
    warnings: list[str] = []

    for where, keys in sorted(used.items()):
        unknown = sorted(keys - defined)
        if unknown:
            failures.append(f"{where}: key(s) not defined in english.txt: {', '.join(unknown)}")

    for key in sorted(defined):
        if not CONVENTION.match(key):
            failures.append(f"{key}: does not follow $<Mod>_<Surface>_<Element> (Menu | View | Notice)")
    unused = sorted(defined - all_used)
    if unused:
        warnings.append(f"defined but unused: {', '.join(unused)}")

    print(f"translations: {len(defined)} key(s) - {len(all_used)} used "
          f"({', '.join(f'{k}: {len(v)}' for k, v in sorted(used.items()))})")

    for warning in warnings:
        print(f"WARNING  {warning}")
    for failure in failures:
        print(f"ERROR    {failure}")

    if failures or (strict and warnings):
        print("FAILED")
        return 1
    print("OK: every used key is defined, naming convention satisfied")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())