#!/usr/bin/env python3
"""
extract_dil.py

Extracts and evaluates HALCoGen `.dil` configuration constants ending with `.VALUE`
(case-insensitive), storing and updating them in a dedicated root-level JSON file.

Features:
- Searches the predefined directory (`./BMS_Master` by default, including nested
  subdirectories) for the requested `.dil` file.
- Stores each constant under the `.dil` filename using the full key path with the
  trailing `.VALUE` stripped (e.g., `"DRIVER.SYSTEM.VAR.CLKT_HCLK_FREQ": 160.0`).
- Supports querying a specific constant by:
  1. Full key with or without `.VALUE` (e.g., `DRIVER.SYSTEM.VAR.CLKT_HCLK_FREQ`)
  2. Partial suffix (e.g., `SYSTEM.VAR.DCC2_ENABLE`)
  3. Short variable name (e.g., `CLKT_HCLK_FREQ`)
  Throws a clear disambiguation error if a short variable name matches multiple
  constants across different modules (e.g., `DCC2_ENABLE`, `DCC1_ENABLE`, `MIBSPI5_ENABLE`).
- Converts integer, float (including `f` suffixes), and signed/unsigned hex values
  (including C typecasts like `(int8_t)0xFF`) to their decimal equivalents, while
  preserving strings and identifiers like `IRQ`, `256_KB`, or `phantomInterrupt`.
- Updates existing JSON entries or adds new ones when the `.dil` file changes,
  printing explicit status lines (`[ADDED]`, `[UPDATED]`, `[UNCHANGED]`, `[SUMMARY]`).

Usage examples:
  python3 extract_dil.py -f BMS_Master.dil --all
  python3 extract_dil.py -f BMS_Master.dil -c CLKT_HCLK_FREQ
  python3 extract_dil.py -f BMS_Master.dil -c DRIVER.SYSTEM.VAR.DCC2_ENABLE
"""

import argparse
import ast
import json
import operator
import os
import re
import sys
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple, Union

# ==============================================================================
# Predefined Configuration
# ==============================================================================
PREDEFINED_DIL_DIR: str = "./BMS_Master"
DEFAULT_JSON_OUTPUT: str = "./dil_constants.json"


# ==============================================================================
# Value Evaluator (Integers, Floats, Signed/Unsigned Hex, and String Preservation)
# ==============================================================================
_ALLOWED_UNARY_OPS = {
    ast.UAdd: operator.pos,
    ast.USub: operator.neg,
    ast.Invert: operator.invert,
}

_ALLOWED_BIN_OPS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.FloorDiv: operator.floordiv,
    ast.Mod: operator.mod,
    ast.LShift: operator.lshift,
    ast.RShift: operator.rshift,
    ast.BitOr: operator.or_,
    ast.BitXor: operator.xor,
    ast.BitAnd: operator.and_,
}


def _apply_c_cast(val: Union[int, float], bits: int, signed: bool) -> int:
    """Applies C two's complement signed or unsigned bit-width truncation."""
    ival = int(val)
    mask = (1 << bits) - 1
    ival &= mask
    if signed and (ival & (1 << (bits - 1))):
        ival -= 1 << bits
    return ival


_CAST_FUNCTIONS: Dict[str, Callable[[Union[int, float]], Union[int, float]]] = {
    "__cast_int8": lambda x: _apply_c_cast(x, 8, True),
    "__cast_uint8": lambda x: _apply_c_cast(x, 8, False),
    "__cast_int16": lambda x: _apply_c_cast(x, 16, True),
    "__cast_uint16": lambda x: _apply_c_cast(x, 16, False),
    "__cast_int32": lambda x: _apply_c_cast(x, 32, True),
    "__cast_uint32": lambda x: _apply_c_cast(x, 32, False),
    "__cast_int64": lambda x: _apply_c_cast(x, 64, True),
    "__cast_uint64": lambda x: _apply_c_cast(x, 64, False),
    "__cast_float": lambda x: float(x),
}

_C_TYPE_TO_HELPER: List[Tuple[re.Pattern, str]] = [
    (re.compile(r"^(?:int8_t|signed\s+char|s8|__int8_t)$"), "__cast_int8"),
    (re.compile(r"^(?:uint8_t|unsigned\s+char|u8|char|__uint8_t|bool|_Bool)$"), "__cast_uint8"),
    (re.compile(r"^(?:int16_t|(?:signed\s+)?short(?:\s+int)?|s16|__int16_t)$"), "__cast_int16"),
    (re.compile(r"^(?:uint16_t|unsigned\s+short(?:\s+int)?|u16|__uint16_t)$"), "__cast_uint16"),
    (re.compile(r"^(?:int32_t|(?:signed\s+)?int|signed|(?:signed\s+)?long|s32|__int32_t)$"), "__cast_int32"),
    (re.compile(r"^(?:uint32_t|unsigned\s+int|unsigned|unsigned\s+long|u32|__uint32_t)$"), "__cast_uint32"),
    (
        re.compile(
            r"^(?:int64_t|(?:signed\s+)?long\s+long(?:\s+int)?|ssize_t|ptrdiff_t|intptr_t|intmax_t|s64|__int64_t)$"
        ),
        "__cast_int64",
    ),
    (
        re.compile(
            r"^(?:uint64_t|unsigned\s+long\s+long(?:\s+int)?|size_t|uintptr_t|uintmax_t|u64|__uint64_t)$"
        ),
        "__cast_uint64",
    ),
    (re.compile(r"^(?:float|double|long\s+double)$"), "__cast_float"),
]


def _match_c_cast_helper(type_str: str) -> Optional[str]:
    cleaned = re.sub(r"\b(?:const|volatile|register)\b", "", type_str).strip()
    cleaned = re.sub(r"\s+", " ", cleaned)
    for pat, helper_name in _C_TYPE_TO_HELPER:
        if pat.match(cleaned):
            return helper_name
    return None


def _find_matching_paren(text: str, open_idx: int) -> int:
    depth = 0
    in_str = False
    escape = False
    for i in range(open_idx, len(text)):
        ch = text[i]
        if escape:
            escape = False
            continue
        if ch == "\\":
            escape = True
            continue
        if ch == '"':
            in_str = not in_str
            continue
        if not in_str:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    return i
    return -1


# Single-pass C numeric token regex
_C_NUMERIC_TOKEN_RE = re.compile(
    r"""
    (?P<string>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')
    | (?P<ident>[a-zA-Z_][a-zA-Z0-9_]*)
    | (?P<hex>0[xX][0-9a-fA-F]+)(?P<hex_suf>[uUlL]*)
    | (?P<bin>0[bB][01]+)(?P<bin_suf>[uUlL]*)
    | (?P<float>(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?|\d+[eE][+-]?\d+)(?P<float_suf>[fFlL]?)
    | (?P<int_float>\d+)(?P<int_float_suf>[fF])\b
    | (?P<octal>0[0-7]+)(?P<oct_suf>[uUlL]*)\b
    | (?P<dec>\d+)(?P<dec_suf>[uUlL]*)\b
    """,
    re.VERBOSE,
)


def _normalize_c_numeric_literals(expr: str) -> str:
    if '"' in expr or "'" in expr:
        return expr

    def _replace_token(m: re.Match) -> str:
        if m.group("string") or m.group("ident"):
            return m.group(0)
        if m.group("hex"):
            return m.group("hex")
        if m.group("bin"):
            return m.group("bin")
        if m.group("float"):
            return m.group("float")
        if m.group("int_float"):
            return f"{m.group('int_float')}.0"
        if m.group("octal"):
            oct_digits = m.group("octal")[1:]
            return f"0o{oct_digits}" if oct_digits else "0"
        if m.group("dec"):
            return m.group("dec")
        return m.group(0)

    return _C_NUMERIC_TOKEN_RE.sub(_replace_token, expr)


def _transform_c_type_casts(expr: str) -> str:
    if '"' in expr or "'" in expr or "(" not in expr:
        return expr

    atom_re = re.compile(
        r"^\s*([+-~\s]*(?:0[xX][0-9a-fA-F]+|0[bB][01]+|0o[0-7]+|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?|\.\d+(?:[eE][+-]?\d+)?|[a-zA-Z_][a-zA-Z0-9_]*(?:\s*\([^()]*\))?))"
    )

    def _pass(s: str) -> str:
        i = 0
        out: List[str] = []
        n = len(s)
        while i < n:
            if s[i] == "(":
                close_type = _find_matching_paren(s, i)
                if close_type != -1:
                    inner_type = s[i + 1 : close_type].strip()
                    helper = _match_c_cast_helper(inner_type)
                    if helper:
                        j = close_type + 1
                        while j < n and s[j].isspace():
                            j += 1
                        if j < n:
                            if s[j] == "(":
                                close_op = _find_matching_paren(s, j)
                                if close_op != -1:
                                    op_inner = _pass(s[j + 1 : close_op])
                                    out.append(f"{helper}({op_inner})")
                                    i = close_op + 1
                                    continue
                            else:
                                m_atom = atom_re.match(s[j:])
                                if m_atom:
                                    operand = _pass(m_atom.group(1).strip())
                                    out.append(f"{helper}({operand})")
                                    i = j + m_atom.end()
                                    continue
            out.append(s[i])
            i += 1
        return "".join(out)

    curr = expr
    for _ in range(10):
        nxt = _pass(curr)
        if nxt == curr:
            break
        curr = nxt
    return curr


def _eval_ast_node(node: ast.AST) -> Union[int, float]:
    if isinstance(node, ast.Expression):
        return _eval_ast_node(node.body)
    if isinstance(node, ast.Constant):
        if isinstance(node.value, (int, float)) and not isinstance(node.value, bool):
            return node.value
        raise ValueError(f"Unsupported constant type: {type(node.value)}")
    if isinstance(node, ast.UnaryOp) and type(node.op) in _ALLOWED_UNARY_OPS:
        operand = _eval_ast_node(node.operand)
        return _ALLOWED_UNARY_OPS[type(node.op)](operand)
    if isinstance(node, ast.BinOp):
        op_type = type(node.op)
        if op_type is ast.Div:
            left = _eval_ast_node(node.left)
            right = _eval_ast_node(node.right)
            if right == 0:
                raise ZeroDivisionError("Division by zero")
            if isinstance(left, int) and isinstance(right, int):
                return left // right
            return float(left) / float(right)
        if op_type in _ALLOWED_BIN_OPS:
            left = _eval_ast_node(node.left)
            right = _eval_ast_node(node.right)
            return _ALLOWED_BIN_OPS[op_type](left, right)
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
        fn_name = node.func.id
        if fn_name in _CAST_FUNCTIONS:
            args = [_eval_ast_node(a) for a in node.args]
            return _CAST_FUNCTIONS[fn_name](*args)
    raise ValueError(f"Unsupported AST node: {type(node).__name__}")


def parse_dil_value(raw_val: str) -> Any:
    """
    Converts `.dil` values into their appropriate Python/JSON types:
    - Quoted strings (e.g. `"ldr pc,[pc,#-0x1b0]"`) -> stripped string
    - Identifier-like tokens with underscores or unit suffixes (e.g. `256_KB`,
      `12_BIT`, `IRQ`, `phantomInterrupt`) -> kept as string!
      (Note: Python's `int("256_000")` normally treats `_` as a digit separator,
      so we explicitly guard against `_` in numeric tokens so `256_KB` or `8_BIT`
      are never misinterpreted.)
    - Signed/unsigned hex values (e.g. `0x0300`, `0xfcffffff`, `-0x1A`,
      `(int8_t)0xFF`, `0xFFU`) -> decimal int
    - Integers (`0`, `1`, `-42`) -> int
    - Floats (`160.000`, `180.0`, `3.14f`) -> float
    """
    cleaned = raw_val.strip()
    if not cleaned:
        return ""

    # If enclosed in double quotes, strip outer quotes and keep as string
    if len(cleaned) >= 2 and cleaned.startswith('"') and cleaned.endswith('"'):
        return cleaned[1:-1]

    # Guard: if the token contains an underscore (e.g., `256_KB`, `12_BIT`, `PRIV_RW_USER_RW_NOEXEC`)
    # and does not start with a C typecast `(int8_t)...`, keep it strictly as a string.
    if "_" in cleaned and not cleaned.startswith("("):
        return cleaned

    # Direct fast-path for standard signed/unsigned integers, hex, and floats
    # 1. Pure integer: e.g. 0, 1, -15, 42U
    m_int = re.match(r"^([+-]?\d+)[uUlL]*$", cleaned)
    if m_int:
        return int(m_int.group(1), 10)

    # 2. Pure hex literal: e.g. 0x0300, 0xfcffffff, -0x10, 0xFFU
    m_hex = re.match(r"^([+-]?0[xX][0-9a-fA-F]+)[uUlL]*$", cleaned)
    if m_hex:
        return int(m_hex.group(1), 16)

    # 3. Pure float literal: e.g. 160.000, 180.0, 3.14f, 1e-6f
    m_flt = re.match(
        r"^([+-]?(?:(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?|\d+[eE][+-]?\d+))[fFlL]?$",
        cleaned,
    )
    if m_flt:
        val = float(m_flt.group(1))
        return float(f"{val:.12g}")

    # 4. General C expression / typecast evaluation (e.g. `(int8_t)0xFF`, `(int32_t)0xFFFFFFFF`)
    candidate = _normalize_c_numeric_literals(cleaned)
    candidate = _transform_c_type_casts(candidate)
    try:
        tree = ast.parse(candidate, mode="eval")
        val = _eval_ast_node(tree)
        if isinstance(val, float):
            return float(f"{val:.12g}")
        return val
    except Exception:
        return cleaned


# ==============================================================================
# .dil File Discovery & Parsing
# ==============================================================================
def locate_dil_file(file_arg: str, search_dir: Path) -> Path:
    """
    Locates `file_arg` either directly or inside `search_dir` (and its subdirectories).
    """
    direct_path = Path(file_arg)
    if direct_path.is_file():
        return direct_path.resolve()

    req_parts = Path(file_arg).parts
    matches: List[Path] = []
    seen_resolved = set()

    if search_dir.exists() and search_dir.is_dir():
        candidate = search_dir / file_arg
        if candidate.is_file():
            res = candidate.resolve()
            seen_resolved.add(res)
            matches.append(res)

        for root, _, files in os.walk(search_dir):
            if Path(file_arg).name in files:
                found = (Path(root) / Path(file_arg).name).resolve()
                if found not in seen_resolved:
                    if (
                        len(found.parts) >= len(req_parts)
                        and found.parts[-len(req_parts) :] == req_parts
                    ):
                        seen_resolved.add(found)
                        matches.append(found)

    if not matches:
        print(
            f"[ERROR] File '{file_arg}' was not found directly or inside '{search_dir}'.",
            file=sys.stderr,
        )
        sys.exit(1)

    if len(matches) > 1:
        print(
            f"[ERROR] Multiple files matching '{file_arg}' were found under '{search_dir}':",
            file=sys.stderr,
        )
        for m in matches:
            print(f"  - {m}", file=sys.stderr)
        print("Please provide a more specific relative path.", file=sys.stderr)
        sys.exit(1)

    return matches[0]


def parse_dil_file(filepath: Path) -> Dict[str, Any]:
    """
    Parses a `.dil` file and extracts all entries whose key ends with `.VALUE`
    (case-insensitive). Strips the trailing `.VALUE` from the stored key name
    and converts the value to int, float, or string.
    """
    try:
        lines = filepath.read_text(encoding="utf-8", errors="replace").splitlines()
    except Exception as exc:
        print(f"[ERROR] Failed to read '{filepath}': {exc}", file=sys.stderr)
        sys.exit(1)

    constants: Dict[str, Any] = {}
    suffix_re = re.compile(r"\.value$", re.IGNORECASE)

    for raw_line in lines:
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue

        key_part, val_part = line.split("=", 1)
        key_part = key_part.strip()

        # Only capture keys ending in `.VALUE` / `.value`
        if not suffix_re.search(key_part):
            continue

        # Cut `.VALUE` from the end of the key
        clean_key = suffix_re.sub("", key_part)
        constants[clean_key] = parse_dil_value(val_part)

    return constants


def resolve_requested_constant(
    query: str, all_constants: Dict[str, Any], file_key: str
) -> Tuple[str, Any]:
    """
    Resolves a user-requested constant name (`query`) against `all_constants`:
    1. If `query` (with optional `.VALUE` stripped) matches a full key exactly
       (case-insensitively), returns that constant.
    2. Otherwise checks if `query` matches the short variable name (the segment
       after the last `.`) or a dot-separated suffix (e.g. `SYSTEM.VAR.DCC2_ENABLE`).
    3. If multiple constants share the same short variable name, prints all
       matching full keys and exits with an error.
    """
    clean_query = re.sub(r"\.value$", "", query.strip(), flags=re.IGNORECASE)

    # 1. Exact full-key match (case-sensitive first, then case-insensitive)
    if clean_query in all_constants:
        return clean_query, all_constants[clean_query]

    lower_query = clean_query.lower()
    for k, v in all_constants.items():
        if k.lower() == lower_query:
            return k, v

    # 2. Short variable name or dot-suffix match
    suffix_target = "." + lower_query
    matches: List[str] = [
        k for k in all_constants if k.lower().endswith(suffix_target)
    ]

    if not matches:
        print(
            f"[ERROR] Constant '{query}' was not found in '{file_key}'.",
            file=sys.stderr,
        )
        sys.exit(1)

    if len(matches) > 1:
        print(
            f"[ERROR] Ambiguous short constant name '{query}' matches {len(matches)} "
            f"different constants in '{file_key}':",
            file=sys.stderr,
        )
        for m in matches:
            print(f"  - {m} (value: {json.dumps(all_constants[m])})", file=sys.stderr)
        print(
            "Please pass the full constant key (e.g., "
            f"'-c {matches[0]}') to disambiguate.",
            file=sys.stderr,
        )
        sys.exit(1)

    matched_key = matches[0]
    return matched_key, all_constants[matched_key]


# ==============================================================================
# JSON Storage & Update Engine
# ==============================================================================
def load_json_store(json_path: Path) -> Dict[str, Dict[str, Any]]:
    """Loads existing JSON file or returns an empty dictionary."""
    if not json_path.exists():
        print(
            f"[INFO] JSON file '{json_path}' does not exist yet. A new file will be created."
        )
        return {}
    try:
        content = json_path.read_text(encoding="utf-8").strip()
        if not content:
            return {}
        data = json.loads(content)
        if not isinstance(data, dict):
            print(
                f"[WARN] Root of '{json_path}' is not a JSON object. Resetting to empty object.",
                file=sys.stderr,
            )
            return {}
        return data
    except Exception as exc:
        print(
            f"[ERROR] Failed to parse existing JSON file '{json_path}': {exc}",
            file=sys.stderr,
        )
        sys.exit(1)


def update_json_constants(
    store: Dict[str, Dict[str, Any]],
    file_key: str,
    extracted_constants: Dict[str, Any],
    verbose_each: bool = True,
) -> Tuple[int, int, int]:
    """
    Inserts or updates constants under `store[file_key]` and prints explicit
    status messages describing what happened.
    """
    if file_key not in store or not isinstance(store[file_key], dict):
        store[file_key] = {}
        print(f"[INFO] Created new file section '{file_key}' in JSON store.")

    file_bucket = store[file_key]
    added_count = 0
    updated_count = 0
    unchanged_count = 0

    for const_key, new_val in extracted_constants.items():
        if const_key not in file_bucket:
            file_bucket[const_key] = new_val
            added_count += 1
            if verbose_each:
                print(
                    f"[ADDED]     [{file_key}] Constant '{const_key}' added with value: "
                    f"{json.dumps(new_val)}"
                )
        else:
            old_val = file_bucket[const_key]
            if old_val != new_val or type(old_val) is not type(new_val):
                file_bucket[const_key] = new_val
                updated_count += 1
                print(
                    f"[UPDATED]   [{file_key}] Constant '{const_key}' updated from "
                    f"{json.dumps(old_val)} -> {json.dumps(new_val)}"
                )
            else:
                unchanged_count += 1
                if verbose_each:
                    print(
                        f"[UNCHANGED] [{file_key}] Constant '{const_key}' is already up to date "
                        f"({json.dumps(new_val)})"
                    )

    return added_count, updated_count, unchanged_count


# ==============================================================================
# CLI Entry Point
# ==============================================================================
def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Extract and evaluate '.VALUE' constants from a .dil file and "
            "store/update them in a dedicated root JSON file."
        )
    )
    parser.add_argument(
        "-f",
        "--file",
        required=True,
        help="Name (or relative path) of the .dil file to inspect (e.g., BMS_Master.dil).",
    )

    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="Grab all '.VALUE' constants from the specified .dil file.",
    )
    mode_group.add_argument(
        "-c",
        "-m",
        "--constant",
        dest="constant",
        metavar="CONSTANT_KEY_OR_SHORT_NAME",
        help=(
            "Grab a specific constant by full key (with or without .VALUE) "
            "or by short variable name (if unique)."
        ),
    )

    parser.add_argument(
        "-d",
        "--dir",
        default=PREDEFINED_DIL_DIR,
        help=f"Override the predefined search directory (default: {PREDEFINED_DIL_DIR}).",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=DEFAULT_JSON_OUTPUT,
        help=f"Path to the output JSON file (default: {DEFAULT_JSON_OUTPUT}).",
    )
    parser.add_argument(
        "-q",
        "--quiet-unchanged",
        action="store_true",
        help="When running with --all, only print individual lines for ADDED/UPDATED constants.",
    )
    return parser


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    search_dir = Path(args.dir)
    json_path = Path(args.output)

    print(f"[INFO] Searching for '{args.file}' in directory: '{search_dir}'...")
    target_file = locate_dil_file(args.file, search_dir)
    file_key = target_file.name
    print(f"[INFO] Located target file: '{target_file}' (JSON key: '{file_key}')")

    all_constants = parse_dil_file(target_file)
    if not all_constants:
        print(f"[INFO] No '.VALUE' constants found in '{target_file}'.")
        sys.exit(0)

    if args.constant:
        resolved_key, resolved_val = resolve_requested_constant(
            args.constant, all_constants, file_key
        )
        selected_constants = {resolved_key: resolved_val}
        print(
            f"[INFO] Extracting specific constant '{resolved_key}' "
            f"(requested as '{args.constant}') from '{file_key}'..."
        )
        verbose_each = True
    else:
        selected_constants = all_constants
        print(
            f"[INFO] Extracting all ({len(selected_constants)}) '.VALUE' constants "
            f"from '{file_key}'..."
        )
        verbose_each = not args.quiet_unchanged

    store = load_json_store(json_path)
    added, updated, unchanged = update_json_constants(
        store, file_key, selected_constants, verbose_each=verbose_each
    )

    json_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(store, indent=2) + "\n", encoding="utf-8")

    print(
        f"[SUMMARY] Saved '{json_path}' | File: '{file_key}' | "
        f"Added: {added}, Updated: {updated}, Unchanged: {unchanged}"
    )


if __name__ == "__main__":
    main()