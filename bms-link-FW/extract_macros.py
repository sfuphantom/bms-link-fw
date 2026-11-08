#!/usr/bin/env python3
"""
extract_macros.py

Clean, debuggable C/C++ macro and enum extractor powered by PyCLibrary.

Requirements:
  python3 -m pip install pyclibrary

Features:
- Searches two predefined directories (and their subdirectories) for the target .h/.c file.
- Extracts object-like macros (evaluating chained macros, function-like macro calls,
  signed/unsigned hex casts, and 'f' floats to concrete numbers).
- Ignores empty #define directives (such as header include guards).
- Stores a clean key: value pair when a macro is defined once, and only creates a
  conditional branch object if the same macro is defined multiple times across
  #ifdef / #elif / #else blocks.
- Extracts C enums and groups them under an "ENUMS" key by enum name.
- Updates ./macros.json in place and prints [ADDED], [UPDATED], and [UNCHANGED] actions.

Examples:
  python3 extract_macros.py -f config.h --all
  python3 extract_macros.py -f config.h -m TOTAL_PACKET_SIZE
  python3 extract_macros.py -f config.h -e BmsState_e

Created On: Oct 25 2026
Author: Jordan Ko
"""

from pyclibrary import CParser
import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Any

# ==============================================================================
# Configuration
# ==============================================================================
PREDEFINED_DIRS = [Path("./Phantom_Hardware"), Path("./Phantom_drivers")]
JSON_OUTPUT = Path("./macros.json")

# C integer typecast bit-widths and signedness: (bits, is_signed)
C_CAST_TYPES: dict[str, tuple[int, bool]] = {
    "int8_t": (8, True), "signed char": (8, True), "s8": (8, True),
    "uint8_t": (8, False), "unsigned char": (8, False), "u8": (8, False), "char": (8, False),
    "int16_t": (16, True), "short": (16, True), "signed short": (16, True), "s16": (16, True),
    "uint16_t": (16, False), "unsigned short": (16, False), "u16": (16, False),
    "int32_t": (32, True), "int": (32, True), "signed int": (32, True), "long": (32, True), "s32": (32, True),
    "uint32_t": (32, False), "unsigned int": (32, False), "unsigned": (32, False), "unsigned long": (32, False), "u32": (32, False),
    "int64_t": (64, True), "long long": (64, True), "s64": (64, True),
    "uint64_t": (64, False), "unsigned long long": (64, False), "size_t": (64, False), "u64": (64, False),
}

# Single-pass numeric token normalizer (prevents .00 in 0.00f from matching as octal)
TOKEN_RE = re.compile(
    r'(?P<str>"(?:\\.|[^"\\])*")'
    r"|(?P<id>[A-Za-z_]\w*)"
    r"|(?P<hex>0[xX][0-9a-fA-F]+)[uUlL]*"
    r"|(?P<bin>0[bB][01]+)[uUlL]*"
    r"|(?P<flt>(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?|\d+[eE][+-]?\d+)[fFlL]?"
    r"|(?P<iflt>\d+)[fF]\b"
    r"|(?P<oct>0[0-7]+)[uUlL]*\b"
    r"|(?P<dec>\d+)[uUlL]*\b"
)


# ==============================================================================
# 1. File Discovery & Comment Stripping
# ==============================================================================
def locate_file(filename: str, search_dirs: list[Path]) -> Path:
    """Finds `filename` across `search_dirs` (including subdirectories)."""
    direct = Path(filename)
    if direct.is_file():
        return direct.resolve()

    req_parts = Path(filename).parts
    matches: list[Path] = []

    for d in search_dirs:
        if not d.is_dir():
            continue
        for root, _, files in os.walk(d):
            if Path(filename).name in files:
                candidate = (Path(root) / Path(filename).name).resolve()
                if candidate not in matches and candidate.parts[-len(req_parts):] == req_parts:
                    matches.append(candidate)

    if not matches:
        sys.exit(f"[ERROR] File '{filename}' not found in {[str(d) for d in search_dirs]}.")
    if len(matches) > 1:
        msg = f"[ERROR] Multiple files match '{filename}':\n" + "\n".join(f"  - {m}" for m in matches)
        sys.exit(msg)
    return matches[0]


def clean_c_source(text: str) -> str:
    """Strips /* ... */ and // comments and joins backslash-continued lines."""
    pattern = re.compile(r'("(?:\\.|[^"\\])*")|(/\*[\s\S]*?\*/)|(//[^\n]*)')
    no_comments = pattern.sub(lambda m: m.group(1) or "", text)
    return re.sub(r"\\\r?\n", " ", no_comments)


# ==============================================================================
# 2. Lightweight Expression & Function-Macro Evaluator
# ==============================================================================
def c_cast(val: float | int, bits: int, signed: bool) -> int:
    """Applies C two's complement signed/unsigned bit-width truncation."""
    ival = int(val) & ((1 << bits) - 1)
    return (ival - (1 << bits)) if (signed and (ival & (1 << (bits - 1)))) else ival


def expand_macros(
    expr: str,
    obj_macros: dict[str, str],
    fn_macros: dict[str, tuple[list[str], str]],
) -> str:
    """Expands function-like macros (e.g. add1(T)) and object-like macros."""
    curr = expr
    for _ in range(20):
        prev = curr
        # Expand function-like macro calls: NAME(arg1, arg2)
        for fn_name, (params, body) in fn_macros.items():
            call_re = re.compile(rf"\b{re.escape(fn_name)}\s*\(([^()]*)\)")
            def _repl_fn(m: re.Match) -> str:
                args = [a.strip() for a in m.group(1).split(",")] if m.group(1).strip() else []
                if len(args) != len(params):
                    return m.group(0)
                amap = {p: f"({a})" for p, a in zip(params, args)}
                sub = re.sub(r"\b([A-Za-z_]\w*)\b", lambda pm: amap.get(pm.group(1), pm.group(1)), body)
                return f"({sub})"
            curr = call_re.sub(_repl_fn, curr)

        # Expand object-like macro identifiers
        curr = re.sub(
            r"\b([A-Za-z_]\w*)\b",
            lambda m: f"({obj_macros[m.group(1)]})"
            if m.group(1) in obj_macros and obj_macros[m.group(1)] not in ("", m.group(1))
            else m.group(1),
            curr,
        )
        if curr == prev:
            break
    return curr


def _find_matching_paren(text: str, open_idx: int) -> int:
    depth = 0
    for i in range(open_idx, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return -1


def _replace_c_casts(text: str) -> str:
    """Recursively replaces C casts like (uint16_t)(...) or (float)x with Python calls."""
    atom_re = re.compile(r"^\s*([+-~]?\s*(?:0[xX][0-9a-fA-F]+|0[bB][01]+|0o[0-7]+|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?|\.\d+(?:[eE][+-]?\d+)?|[A-Za-z_]\w*))")

    def _pass(s: str) -> str:
        i, n, out = 0, len(s), []
        while i < n:
            if s[i] == "(":
                close_t = _find_matching_paren(s, i)
                if close_t != -1:
                    tname = re.sub(r"\b(?:const|volatile)\b", "", s[i + 1 : close_t]).strip()
                    tname = re.sub(r"\s+", " ", tname)
                    if tname in ("float", "double") or tname in C_CAST_TYPES:
                        j = close_t + 1
                        while j < n and s[j].isspace():
                            j += 1
                        if j < n:
                            op_str, next_i = None, i
                            if s[j] == "(":
                                close_op = _find_matching_paren(s, j)
                                if close_op != -1:
                                    op_str, next_i = _pass(s[j + 1 : close_op]), close_op + 1
                            else:
                                m_at = atom_re.match(s[j:])
                                if m_at:
                                    op_str, next_i = m_at.group(1).strip(), j + m_at.end()
                            if op_str is not None:
                                if tname in ("float", "double"):
                                    out.append(f"float({op_str})")
                                else:
                                    bits, signed = C_CAST_TYPES[tname]
                                    out.append(f"c_cast({op_str}, {bits}, {signed})")
                                i = next_i
                                continue
            out.append(s[i])
            i += 1
        return "".join(out)

    curr = text
    for _ in range(8):
        nxt = _pass(curr)
        if nxt == curr:
            break
        curr = nxt
    return curr


def evaluate_c_expr(expr: str) -> Any:
    """Evaluates C constant expressions, float 'f' suffixes, and signed/unsigned casts."""
    s = expr.strip()
    if not s:
        return ""
    if s.startswith('"') and s.endswith('"') and s.count('"') == 2:
        return s[1:-1]

    # 1. Normalize C numeric tokens in one pass
    def _tok(m: re.Match) -> str:
        if m.group("str") or m.group("id"):
            return m.group(0)
        if m.group("hex") or m.group("bin") or m.group("flt") or m.group("dec"):
            return m.group("hex") or m.group("bin") or m.group("flt") or m.group("dec")
        if m.group("iflt"):
            return f"{m.group('iflt')}.0"
        if m.group("oct"):
            return f"0o{m.group('oct')[1:]}"
        return m.group(0)

    norm = TOKEN_RE.sub(_tok, s)

    # 2. Replace C casts like (uint16_t) or (float) with Python callables
    py_expr = _replace_c_casts(norm)
    # Convert C ternary `a ? b : c` -> `((b) if (a) else (c))`
    py_expr = re.sub(r"([^?:()]+)\?([^?:()]+):([^?:()]+)", r"((\2) if (\1) else (\3))", py_expr)
    py_expr = py_expr.replace("&&", " and ").replace("||", " or ")

    try:
        # Evaluate safely with only c_cast and float exposed
        val = eval(py_expr, {"__builtins__": {}}, {"c_cast": c_cast, "float": float})
        if isinstance(val, float):
            return float(f"{val:.12g}")
        return val
    except Exception:
        return s


# ==============================================================================
# 3. Macro & Enum Extraction (PyCLibrary + Conditional Branch Grouping)
# ==============================================================================
def collect_workspace_macros(search_dirs: list[Path]) -> tuple[dict[str, str], dict[str, tuple[list[str], str]]]:
    """Scans all .h/.c files in `search_dirs` to build cross-file macro lookup tables."""
    obj_macros: dict[str, str] = {}
    fn_macros: dict[str, tuple[list[str], str]] = {}
    def_re = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)(\([^)]*\))?\s*(.*)$")

    for d in search_dirs:
        if not d.is_dir():
            continue
        for root, _, files in os.walk(d):
            for fname in sorted(files):
                if fname.endswith((".h", ".c")):
                    text = clean_c_source((Path(root) / fname).read_text(encoding="utf-8", errors="ignore"))
                    for line in text.splitlines():
                        m = def_re.match(line)
                        if not m:
                            continue
                        name, params, body = m.group(1), m.group(2), m.group(3).strip()
                        if params is not None:
                            p_list = [p.strip() for p in params[1:-1].split(",") if p.strip()]
                            fn_macros[name] = (p_list, body)
                        elif body and body.upper() not in ("TRUE", "FALSE"):
                            obj_macros[name] = body
    return obj_macros, fn_macros


def extract_macros_and_enums(
    target_file: Path,
    search_dirs: list[Path],
) -> tuple[dict[str, Any], dict[str, dict[str, int]]]:
    """
    Extracts object-like macros (with #ifdef/#else branch grouping when defined
    multiple times) and enums (grouped by enum name) from `target_file`.
    """
    obj_macros, fn_macros = collect_workspace_macros(search_dirs)
    text = clean_c_source(target_file.read_text(encoding="utf-8", errors="replace"))

    # Run PyCLibrary CParser on the target file to pre-populate values & enums
    parser = CParser([str(target_file)])
    pyclib_values: dict[str, Any] = dict(parser.defs.get("values", {}))
    enums: dict[str, dict[str, int]] = {}

    for enum_name, enum_dict in parser.defs.get("enums", {}).items():
        if isinstance(enum_dict, dict):
            clean_name = enum_name.replace("enum ", "").strip() or "anonymous_enum"
            enums[clean_name] = {k: int(v) for k, v in enum_dict.items()}

    # Lightweight conditional-aware macro scan on target_file
    cond_stack: list[str] = []
    raw_defs: dict[str, list[tuple[str | None, str]]] = {}
    dir_re = re.compile(r"^\s*#\s*([a-zA-Z]+)\s*(.*)$")
    def_re = re.compile(r"^([A-Za-z_]\w*)(\([^)]*\))?\s*(.*)$")

    for line in text.splitlines():
        m = dir_re.match(line)
        if not m:
            continue
        cmd, rest = m.group(1), m.group(2).strip()
        if cmd in ("ifdef", "ifndef", "if"):
            # Ignore outer file-level #ifndef include guards
            if cmd == "ifndef" and not cond_stack and not raw_defs:
                continue
            cond_stack.append(f"#{cmd} {rest}")
        elif cmd == "elif" and cond_stack:
            cond_stack[-1] = f"{cond_stack[-1]} -> #elif {rest}"
        elif cmd == "else" and cond_stack:
            cond_stack[-1] = f"#else (after {cond_stack[-1]})"
        elif cmd == "endif":
            if cond_stack:
                cond_stack.pop()
        elif cmd == "define":
            dm = def_re.match(rest)
            if not dm or dm.group(2) is not None:
                continue  # Skip function-like macros in output
            name, body = dm.group(1), dm.group(3).strip()
            if not body:
                continue  # Ignore any #define with no value (such as header guards)
            cond = " && ".join(cond_stack) if cond_stack else None
            raw_defs.setdefault(name, []).append((cond, body))

    macros_out: dict[str, Any] = {}
    for name, branches in raw_defs.items():
        # Clean key: value pair when defined once; branch object only when defined multiple times
        if len(branches) == 1:
            _, body = branches[0]
            if body.upper() in ("TRUE", "FALSE"):
                macros_out[name] = body
            else:
                expanded = expand_macros(body, obj_macros, fn_macros)
                val = evaluate_c_expr(expanded)
                if isinstance(val, str) and name in pyclib_values and pyclib_values[name] is not None:
                    val = pyclib_values[name]
                macros_out[name] = val
        else:
            branch_map: dict[str, Any] = {}
            for cond, body in branches:
                key = cond or "default"
                if body.upper() in ("TRUE", "FALSE"):
                    branch_map[key] = body
                else:
                    local_obj = {**obj_macros, name: body}
                    expanded = expand_macros(body, local_obj, fn_macros)
                    branch_map[key] = evaluate_c_expr(expanded)
            macros_out[name] = branch_map

    # Resolve typedef enum names (e.g. typedef enum { ... } BMSState_t;) and macro-based enum values
    enum_re = re.compile(
        r"(?:typedef\s+)?enum(?:\s+([A-Za-z_]\w*))?\s*\{([^}]*)\}\s*([A-Za-z_]\w*)?\s*;",
        re.DOTALL,
    )
    anon_counter = 1
    for em in enum_re.finditer(text):
        tag_name, body, typedef_name = em.group(1), em.group(2), em.group(3)
        enum_name = typedef_name or tag_name
        if not enum_name:
            enum_name = "anonymous_enum" if anon_counter == 1 else f"anonymous_enum_{anon_counter}"
            anon_counter += 1

        # Remove PyCLibrary's placeholder tag (e.g. '#anon_enum0') if typedef_name is used
        for k in list(enums.keys()):
            if k.startswith("#anon_enum"):
                del enums[k]

        members: dict[str, int] = {}
        next_val = 0
        for item in body.split(","):
            item = item.strip()
            if not item:
                continue
            if "=" in item:
                k, v_expr = item.split("=", 1)
                k = k.strip()
                expanded = expand_macros(v_expr.strip(), {**obj_macros, **{ek: str(ev) for ek, ev in members.items()}}, fn_macros)
                eval_v = evaluate_c_expr(expanded)
                next_val = int(eval_v) if isinstance(eval_v, (int, float)) else next_val
            else:
                k = item
            members[k] = next_val
            next_val += 1

        if members:
            enums[enum_name] = members

    return macros_out, enums


# ==============================================================================
# 4. JSON Store Update Engine
# ==============================================================================
def update_json_file(
    json_path: Path,
    file_key: str,
    selected_macros: dict[str, Any],
    selected_enums: dict[str, dict[str, int]],
) -> None:
    store: dict[str, Any] = {}
    if json_path.exists():
        try:
            store = json.loads(json_path.read_text(encoding="utf-8") or "{}")
        except Exception as exc:
            sys.exit(f"[ERROR] Invalid JSON in '{json_path}': {exc}")

    bucket = store.setdefault(file_key, {})
    added, updated, unchanged = 0, 0, 0

    # 1. Update Macros
    for k, val in selected_macros.items():
        if k not in bucket:
            bucket[k] = val
            added += 1
            print(f"[ADDED]     [{file_key}] Macro '{k}' = {json.dumps(val)}")
        elif bucket[k] != val or type(bucket[k]) is not type(val):
            old = bucket[k]
            bucket[k] = val
            updated += 1
            print(f"[UPDATED]   [{file_key}] Macro '{k}': {json.dumps(old)} -> {json.dumps(val)}")
        else:
            unchanged += 1
            print(f"[UNCHANGED] [{file_key}] Macro '{k}' = {json.dumps(val)}")

    # 2. Update Enums under "ENUMS" key
    if selected_enums:
        enum_bucket = bucket.setdefault("ENUMS", {})
        for ename, emembers in selected_enums.items():
            if ename not in enum_bucket:
                enum_bucket[ename] = emembers
                added += 1
                print(f"[ADDED]     [{file_key}][ENUMS] '{ename}' = {json.dumps(emembers)}")
            elif enum_bucket[ename] != emembers:
                old = enum_bucket[ename]
                enum_bucket[ename] = emembers
                updated += 1
                print(f"[UPDATED]   [{file_key}][ENUMS] '{ename}': {json.dumps(old)} -> {json.dumps(emembers)}")
            else:
                unchanged += 1
                print(f"[UNCHANGED] [{file_key}][ENUMS] '{ename}' = {json.dumps(emembers)}")

    json_path.write_text(json.dumps(store, indent=2) + "\n", encoding="utf-8")
    print(
        f"[SUMMARY] Saved '{json_path}' | File: '{file_key}' | "
        f"Added: {added}, Updated: {updated}, Unchanged: {unchanged}"
    )


# ==============================================================================
# 5. CLI Entry Point
# ==============================================================================
def main() -> None:
    parser = argparse.ArgumentParser(description="Extract C macros and enums into JSON.")
    parser.add_argument("-f", "--file", required=True, help="Target .h or .c file")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-a", "--all", action="store_true", help="Grab all macros and enums")
    group.add_argument("-m", "--macro", help="Grab a specific macro by name")
    group.add_argument("-e", "--enum", help="Grab a specific enum by name")

    parser.add_argument("-d", "--dirs", nargs=2, default=[str(d) for d in PREDEFINED_DIRS], help="Two search directories")
    parser.add_argument("-o", "--output", default=str(JSON_OUTPUT), help="Output JSON path")
    args = parser.parse_args()

    search_dirs = [Path(d) for d in args.dirs]
    target_file = locate_file(args.file, search_dirs)
    file_key = target_file.name

    macros, enums = extract_macros_and_enums(target_file, search_dirs)

    if args.macro:
        if args.macro not in macros:
            sys.exit(f"[ERROR] Macro '{args.macro}' not found in '{file_key}'.")
        update_json_file(Path(args.output), file_key, {args.macro: macros[args.macro]}, {})
    elif args.enum:
        if args.enum not in enums:
            sys.exit(f"[ERROR] Enum '{args.enum}' not found in '{file_key}'.")
        update_json_file(Path(args.output), file_key, {}, {args.enum: enums[args.enum]})
    else:
        update_json_file(Path(args.output), file_key, macros, enums)


if __name__ == "__main__":
    main()