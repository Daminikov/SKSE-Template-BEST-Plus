#!/usr/bin/env python3
"""Creates a new translation stub from english.txt (one file per language).

    python tools/new_language.py german
    python tools/new_language.py japanese

The stub keeps every english key with the english text, so the mod works immediately and a
translator just overwrites the values one by one. Write ONLY the part after the TAB: the key
(including the leading "$") is the contract with the code and must not change.
"""
import pathlib
import sys

LANGUAGES_HINT = ("english russian german french spanish italian polish czech japanese korean "
                  "chinese portuguese turkish ukrainian")

def read(path: pathlib.Path) -> list[tuple[str, str]]:
    raw = path.read_bytes()
    if not raw.startswith(b"\xff\xfe"):
        raise SystemExit(f"{path.name}: not UTF-16 LE with BOM - regenerate it with make_translations.py")
    text = raw[2:].decode("utf-16-le")
    entries: list[tuple[str, str]] = []
    for line in text.splitlines():
        line = line.rstrip("\r")
        if not line.startswith("$"):
            continue
        key, _, value = line.partition("\t")
        entries.append((key, value))
    return entries

def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        print(f"known Skyrim language names: {LANGUAGES_HINT}")
        return 2

    language = sys.argv[1].strip().lower()
    root = pathlib.Path(__file__).resolve().parent.parent / "translations"
    source = root / "english.txt"
    target = root / f"{language}.txt"

    if not source.exists():
        raise SystemExit(f"{source} is missing - run tools/make_translations.py first")
    if target.exists():
        raise SystemExit(f"{target} already exists - edit it instead of regenerating")

    entries = read(source)
    body = "".join(f"{key}\t{value}\r\n" for key, value in entries)
    target.write_bytes(b"\xff\xfe" + body.encode("utf-16-le"))

    print(f"created {target} with {len(entries)} key(s), values still english")
    print(f"translate it, then run:  python tools/check_translations.py")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())