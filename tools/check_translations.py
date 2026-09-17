#!/usr/bin/env python3
"""Checks every translations/<language>.txt against english.txt.

    python tools/check_translations.py

Reports, per language: string count, missing keys, extra keys, empty values, values still identical
to english, and format problems (encoding, duplicated keys, stray % conversions that a translator
could smuggle into a text call). Exit code 0 = clean.
"""
import pathlib
import re
import sys

SPEC = re.compile(r"%[-+ #0-9.]*[a-zA-Z%]")

def read(path: pathlib.Path) -> tuple[dict[str, str], list[str]]:
    problems: list[str] = []
    raw = path.read_bytes()
    if not raw.startswith(b"\xff\xfe"):
        problems.append("not UTF-16 LE with BOM")
        return {}, problems
    text = raw[2:].decode("utf-16-le", errors="replace")
    if "\r\n" not in text:
        problems.append("no CRLF line endings")

    entries: dict[str, str] = {}
    for number, line in enumerate(text.splitlines(), start=1):
        line = line.rstrip("\r")
        if not line:
            continue
        if not line.startswith("$"):
            problems.append(f"line {number}: does not start with $ ({line[:40]!r})")
            continue
        key, tab, value = line.partition("\t")
        if not tab:
            problems.append(f"line {number}: no TAB between key and text")
            continue
        if key in entries:
            problems.append(f"line {number}: duplicated key {key}")
        entries[key] = value
    return entries, problems

def spec_count(text: str) -> int:
    return len(SPEC.findall(text))

def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent / "translations"
    files = sorted(root.glob("*.txt"))
    if not files:
        raise SystemExit(f"no translation files in {root}")

    english, english_problems = read(root / "english.txt")
    failures = len(english_problems)

    print(f"reference: english.txt, {len(english)} key(s)")
    for problem in english_problems:
        print(f"  english.txt: {problem}")

    header = f"{'language':<14}{'strings':>8}{'missing':>9}{'extra':>7}{'empty':>7}{'english':>9}{'spec':>6}"
    print(header)
    print("-" * len(header))

    for path in files:
        language = path.stem
        entries, problems = read(path)
        missing = [key for key in english if key not in entries]
        extra = [key for key in entries if key not in english]
        empty = [key for key, value in entries.items() if not value.strip()]
        unchanged = [] if language == "english" else [
            key for key, value in entries.items() if english.get(key) == value
        ]
        spec = [key for key, value in entries.items()
                if key in english and spec_count(value) != spec_count(english[key])]

        print(f"{language:<14}{len(entries):>8}{len(missing):>9}{len(extra):>7}{len(empty):>7}"
              f"{len(unchanged):>9}{len(spec):>6}")

        for label, items in (("missing", missing), ("extra", extra), ("empty", empty),
                             ("format", problems), ("% specifier mismatch", spec)):
            if items:
                failures += 1
                shown = ", ".join(items[:6])
                more = "" if len(items) <= 6 else f" (+{len(items) - 6} more)"
                print(f"  {language}: {label}: {shown}{more}")

    print()
    if failures:
        print(f"FAILED: {failures} problem group(s)")
        return 1
    print("OK: formats valid, no missing or extra keys, no empty values, % conversions match english")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())