#!/usr/bin/env python3
"""
extract_dil.py

Reads '.VALUE' constants from a HALCoGen .dil file and writes/updates them
as C #define macros in ./dil_constants.h.

Examples:
  python3 extract_dil.py -f BMS_Master.dil --all
  python3 extract_dil.py -f BMS_Master.dil -c CLKT_HCLK_FREQ
  python3 extract_dil.py -f BMS_Master.dil -c DRIVER.SYSTEM.VAR.DCC2_ENABLE

Created On: Nov 07 2026
Author: Jordan Ko
"""

import argparse
import re
import sys
from pathlib import Path

# ==============================================================================
# Configuration
# ==============================================================================
DIL_DIR = Path("./BMS_Master")
HEADER_FILE = Path("./dil_constants.h")

# Matches integers (0, -42), hex (0x0300), or floats (160.000, 1e-6f)
NUMBER_RE = re.compile(
    r"^[+-]?(?:0[xX][0-9a-fA-F]+|\d+(?:\.\d+)?|\.\d+)(?:[eE][+-]?\d+)?[uUlLfF]*$"
)


def format_macro_value(raw_val: str) -> str:
    """Keep numbers/hex/floats as-is; wrap everything else in double quotes."""
    val = raw_val.strip()
    if val.startswith('"') and val.endswith('"'):
        return val
    if NUMBER_RE.match(val):
        return val
    return f'"{val}"'


def parse_dil_file(dil_path: Path) -> dict[str, str]:
    """
    Returns {MACRO_NAME: formatted_value} for all .VALUE lines in the .dil file.
    Example:
      DRIVER.SYSTEM.VAR.CLKT_HCLK_FREQ.VALUE=160.000
      -> {"DRIVER_SYSTEM_VAR_CLKT_HCLK_FREQ": "160.000"}
    """
    macros: dict[str, str] = {}

    for line in dil_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue

        key, raw_val = line.split("=", 1)
        key = key.strip()

        if not key.upper().endswith(".VALUE"):
            continue

        # Drop '.VALUE' (last 6 chars) and replace '.' with '_'
        macro_name = key[:-6].replace(".", "_")
        macros[macro_name] = format_macro_value(raw_val)

    return macros


def find_single_macro(query: str, all_macros: dict[str, str]) -> dict[str, str]:
    """Finds a specific macro by full name or short suffix."""
    clean = re.sub(r"\.value$", "", query.strip(), flags=re.IGNORECASE)
    clean = clean.replace(".", "_")

    # 1. Exact match
    if clean in all_macros:
        return {clean: all_macros[clean]}

    # 2. Short variable name match (e.g. CLKT_HCLK_FREQ)
    suffix = "_" + clean
    matches = [k for k in all_macros if k.endswith(suffix)]

    if not matches:
        sys.exit(f"[ERROR] Constant '{query}' not found.")

    if len(matches) > 1:
        msg = f"[ERROR] Ambiguous name '{query}' matches multiple constants:\n"
        msg += "\n".join(f"  - {m}" for m in matches)
        sys.exit(msg)

    name = matches[0]
    return {name: all_macros[name]}


def update_header_file(header_path: Path, selected_macros: dict[str, str]) -> None:
    """
    Reads existing dil_constants.h (if present), updates existing #define lines
    or appends new ones, and writes the file back with header guards.
    """
    existing: dict[str, str] = {}
    define_re = re.compile(r"^#define\s+([A-Za-z0-9_]+)\s+\((.*)\)\s*$")

    if header_path.exists():
        for line in header_path.read_text(encoding="utf-8").splitlines():
            m = define_re.match(line.strip())
            if m:
                existing[m.group(1)] = m.group(2)

    added, updated, unchanged = 0, 0, 0

    for name, val in selected_macros.items():
        if name not in existing:
            existing[name] = val
            added += 1
            print(f"[ADDED]     #define {name} ({val})")
        elif existing[name] != val:
            old_val = existing[name]
            existing[name] = val
            updated += 1
            print(f"[UPDATED]   #define {name}: ({old_val}) -> ({val})")
        else:
            unchanged += 1
            print(f"[UNCHANGED] #define {name} ({val})")

    # Write clean header file
    lines = [
        "#ifndef DIL_CONSTANTS_H",
        "#define DIL_CONSTANTS_H",
        "",
    ]
    for name, val in existing.items():
        lines.append(f"#define {name} ({val})")
    lines.extend(["", "#endif /* DIL_CONSTANTS_H */", ""])

    header_path.write_text("\n".join(lines), encoding="utf-8")
    print(
        f"[SUMMARY] Saved '{header_path}' | "
        f"Added: {added}, Updated: {updated}, Unchanged: {unchanged}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Convert .dil .VALUE lines to a C header.")
    parser.add_argument("-f", "--file", required=True, help=".dil filename (e.g. BMS_Master.dil)")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-a", "--all", action="store_true", help="Grab all .VALUE constants")
    group.add_argument("-c", "--constant", help="Grab a specific constant by name")

    args = parser.parse_args()

    # Locate the .dil file directly or inside ./BMS_Master
    dil_path = Path(args.file)
    if not dil_path.is_file():
        dil_path = DIL_DIR / args.file
    if not dil_path.is_file():
        sys.exit(f"[ERROR] Could not find '{args.file}' directly or in '{DIL_DIR}'.")

    all_macros = parse_dil_file(dil_path)
    selected = all_macros if args.all else find_single_macro(args.constant, all_macros)

    update_header_file(HEADER_FILE, selected)


if __name__ == "__main__":
    main()