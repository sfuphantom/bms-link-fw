#!/usr/bin/env python3
"""
extract_macros.py

Extracts and evaluates C/C++ object-like macros from .c and .h files located in
two predefined directories (including all nested subdirectories), storing and
updating them in a root-level JSON file.

Features:
- Recursively searches all subdirectories within the two predefined directories
  for both the target file and any cross-file macro definitions.
- Single-pass C numeric lexical scanner that normalizes C float literals (`3.14f`,
  `0.00f`, `1e-6f`, `.5f`), hex literals (`0xFFU`), binary literals (`0b1010`),
  and octal literals (`0755`) as atomic tokens so fractional parts like `.00` in
  `0.00f` are never misidentified as octal numbers.
- Accurately evaluates signed vs. unsigned hex values and nested C typecasts using
  two's complement wrap-around (e.g., `(int8_t)0xFF` -> `-1`, `0xFFU` -> `255`,
  `(uint16_t)(((float)0.5f)/((float)(100*1e-6f)))` -> `5000`).
- Evaluates macros that depend on function-like macros (cross-file or local,
  such as `#define add1(x) ((x) + 1)` and `#define T_PLUS_1 add1(T)` -> `6`)
  as well as actual C functions/`sizeof` via a background `gcc`/`clang` compile-and-run
  probe when needed, while keeping function-like macro definitions themselves out
  of the output JSON.
- Explicitly preserves conditional macro definitions (`#ifdef`, `#ifndef`, `#if`, `#elif`, `#else`)
  as a structured dictionary showing each condition branch and its evaluated value.
- Updates existing JSON entries and prints clear status messages for every macro action.

Usage examples:
  python3 extract_macros.py -f example.h --all
  python3 extract_macros.py -f example.h -m SAMPLE_MACRO

Created On: Oct 25 2026
Author: Jordan Ko
"""

import argparse
import ast
import json
import math
import operator
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple, Union

# ==============================================================================
# Predefined Configuration (Customize your two directory paths here)
# ==============================================================================
PREDEFINED_DIRS: List[str] = [
    "./Phantom_drivers",
    "./Phantom_Hardware",
]

DEFAULT_JSON_OUTPUT: str = "./macros.json"


# ==============================================================================
# Safe Constant Expression Evaluator (with Float `f`, Signed Casts & Ternaries)
# ==============================================================================
_ALLOWED_UNARY_OPS = {
    ast.UAdd: operator.pos,
    ast.USub: operator.neg,
    ast.Invert: operator.invert,
    ast.Not: lambda x: int(not bool(x)),
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

_ALLOWED_CMP_OPS = {
    ast.Eq: lambda a, b: int(a == b),
    ast.NotEq: lambda a, b: int(a != b),
    ast.Lt: lambda a, b: int(a < b),
    ast.LtE: lambda a, b: int(a <= b),
    ast.Gt: lambda a, b: int(a > b),
    ast.GtE: lambda a, b: int(a >= b),
}


def _apply_c_cast(val: Union[int, float], bits: int, signed: bool) -> int:
    """Applies C two's complement signed or unsigned bit-width truncation."""
    ival = int(val)
    mask = (1 << bits) - 1
    ival &= mask
    if signed and (ival & (1 << (bits - 1))):
        ival -= 1 << bits
    return ival


# Map C typecast names to helper callables in AST evaluation
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
    # Common C math functions if invoked inside constant macros
    "abs": abs,
    "fabs": lambda x: float(abs(x)),
    "fabsf": lambda x: float(abs(x)),
    "sqrt": math.sqrt,
    "sqrtf": math.sqrt,
    "sin": math.sin,
    "cos": math.cos,
    "tan": math.tan,
    "floor": math.floor,
    "ceil": math.ceil,
    "round": round,
}

# Supports standard C types and <stdint.h> types (uint8_t, int16_t, uint32_t, etc.)
_C_TYPE_TO_HELPER: List[Tuple[re.Pattern, str]] = [
    (
        re.compile(r"^(?:int8_t|signed\s+char|s8|__int8_t)$"),
        "__cast_int8",
    ),
    (
        re.compile(r"^(?:uint8_t|unsigned\s+char|u8|char|__uint8_t|bool|_Bool)$"),
        "__cast_uint8",
    ),
    (
        re.compile(r"^(?:int16_t|(?:signed\s+)?short(?:\s+int)?|s16|__int16_t)$"),
        "__cast_int16",
    ),
    (
        re.compile(r"^(?:uint16_t|unsigned\s+short(?:\s+int)?|u16|__uint16_t)$"),
        "__cast_uint16",
    ),
    (
        re.compile(r"^(?:int32_t|(?:signed\s+)?int|signed|(?:signed\s+)?long|s32|__int32_t)$"),
        "__cast_int32",
    ),
    (
        re.compile(r"^(?:uint32_t|unsigned\s+int|unsigned|unsigned\s+long|u32|__uint32_t)$"),
        "__cast_uint32",
    ),
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
    (
        re.compile(r"^(?:float|double|long\s+double)$"),
        "__cast_float",
    ),
]


def _match_c_cast_helper(type_str: str) -> Optional[str]:
    """Returns the `_CAST_FUNCTIONS` helper name if `type_str` is a C numeric type."""
    cleaned = re.sub(r"\b(?:const|volatile|register)\b", "", type_str).strip()
    cleaned = re.sub(r"\s+", " ", cleaned)
    for pat, helper_name in _C_TYPE_TO_HELPER:
        if pat.match(cleaned):
            return helper_name
    return None


def _find_matching_paren(text: str, open_idx: int) -> int:
    """Finds the index of the matching `)` for `(` at `open_idx`, or -1."""
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


# Single-pass C numeric token regex: matches strings, identifiers, hex, binary,
# floats (with decimal point and/or exponent), shorthand floats (`2f`), octals, and decimals
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
    """
    Single-pass C numeric lexical normalizer:
    - Scans the expression left-to-right so each literal (string, identifier,
      hex, binary, floating-point, octal, or decimal) is matched once as an
      atomic token.
    - Prevents multi-pass regex collisions (such as the `.00` in `0.00f` being
      mistaken for an octal literal `0o0`).
    """
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
    """
    Balanced-parenthesis parser that transforms arbitrary nested C type casts like
    `(uint16_t)(((uint16_t)(0.5f)) / ((float)(100 * 1e-6f)))` or `(int8_t)0xFF`
    into AST helper calls (`__cast_uint16(...)`, `__cast_int8(0xFF)`).
    Handles deeply nested parenthesized operands that regexes cannot match.
    """
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


def _convert_c_ternary_to_python(expr: str) -> str:
    """
    Converts C ternary expressions `cond ? true_val : false_val` into Python
    `((true_val) if (cond) else (false_val))` for AST evaluation.
    """
    if "?" not in expr or ":" not in expr or '"' in expr:
        return expr

    ternary_re = re.compile(r"([^?:()]+)\?([^?:()]+):([^?:()]+)")
    prev = None
    curr = expr
    for _ in range(8):
        if "?" not in curr or curr == prev:
            break
        prev = curr
        curr = ternary_re.sub(
            lambda m: f"(({m.group(2).strip()}) if ({m.group(1).strip()}) else ({m.group(3).strip()}))",
            curr,
        )
    return curr


def _eval_ast_node(node: ast.AST) -> Union[int, float]:
    """Recursively evaluates an AST node restricted to safe numeric and cast operations."""
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
                raise ZeroDivisionError("Division by zero in macro expression")
            if isinstance(left, int) and isinstance(right, int):
                return left // right
            return float(left) / float(right)
        if op_type in _ALLOWED_BIN_OPS:
            left = _eval_ast_node(node.left)
            right = _eval_ast_node(node.right)
            return _ALLOWED_BIN_OPS[op_type](left, right)
    if isinstance(node, ast.BoolOp):
        values = [_eval_ast_node(v) for v in node.values]
        if isinstance(node.op, ast.And):
            return int(all(bool(v) for v in values))
        if isinstance(node.op, ast.Or):
            return int(any(bool(v) for v in values))
    if isinstance(node, ast.Compare) and len(node.ops) == 1 and len(node.comparators) == 1:
        op_type = type(node.ops[0])
        if op_type in _ALLOWED_CMP_OPS:
            left = _eval_ast_node(node.left)
            right = _eval_ast_node(node.comparators[0])
            return _ALLOWED_CMP_OPS[op_type](left, right)
    if isinstance(node, ast.IfExp):
        cond = _eval_ast_node(node.test)
        return _eval_ast_node(node.body) if bool(cond) else _eval_ast_node(node.orelse)
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
        fn_name = node.func.id
        if fn_name in _CAST_FUNCTIONS:
            args = [_eval_ast_node(a) for a in node.args]
            return _CAST_FUNCTIONS[fn_name](*args)
    raise ValueError(f"Unsupported AST node: {type(node).__name__}")


def try_evaluate_expression(expr: str) -> Any:
    """
    Attempts to evaluate a C expression to a concrete int, float, or clean string.
    Falls back to the cleaned literal string if the expression cannot be reduced.
    """
    cleaned = expr.strip()
    if not cleaned:
        return ""

    str_literals = re.findall(r'"((?:\\.|[^"\\])*)"', cleaned)
    if str_literals:
        remainder = re.sub(r'"(?:\\.|[^"\\])*"', "", cleaned).strip("() \t")
        if not remainder:
            return "".join(str_literals)

    # 1. Single-pass normalization of C floats ('f'/'F'), hex, binary, octal, and integer suffixes
    candidate = _normalize_c_numeric_literals(cleaned)
    # 2. Convert C signed/unsigned/float typecasts (including nested casts) into AST helper calls
    candidate = _transform_c_type_casts(candidate)
    # 3. Convert C ternary `? :` into Python `if else`
    candidate = _convert_c_ternary_to_python(candidate)
    # 4. Convert C logical operators (`&&`, `||`, `!`)
    candidate_py = re.sub(r"&&", " and ", candidate)
    candidate_py = re.sub(r"\|\|", " or ", candidate_py)
    candidate_py = re.sub(r"!(?!=)", " not ", candidate_py)

    try:
        tree = ast.parse(candidate_py, mode="eval")
        val = _eval_ast_node(tree)
        if isinstance(val, float):
            return float(f"{val:.12g}")
        return val
    except Exception:
        return cleaned


# ==============================================================================
# C Source Parsing & Macro Extraction (Object-Like + Function-Like Helpers)
# ==============================================================================
def strip_c_comments(text: str) -> str:
    """
    Removes C block comments (/* ... */) and line comments (// ...) while
    preserving string literals and newlines.
    """
    pattern = re.compile(
        r'("(?:\\.|[^"\\])*")|(\'(?:\\.|[^\'\\])*\')|(/\*[\s\S]*?\*/)|(//[^\n]*)'
    )

    def _replacer(match: re.Match) -> str:
        if match.group(1) is not None:
            return match.group(1)
        if match.group(2) is not None:
            return match.group(2)
        if match.group(3) is not None:
            return "\n" * match.group(3).count("\n")
        return ""

    return pattern.sub(_replacer, text)


def join_line_continuations(text: str) -> str:
    """Joins lines ending with a backslash (`\\`) continuation."""
    return re.sub(r"\\\r?\n", " ", text)


class MacroDefinition:
    """Represents a `#define` occurrence (either object-like or function-like)."""

    def __init__(
        self,
        name: str,
        raw_value: str,
        condition: Optional[str],
        source_file: Path,
        is_func_like: bool = False,
        params: Optional[List[str]] = None,
    ):
        self.name = name
        self.raw_value = raw_value.strip()
        self.condition = condition
        self.source_file = source_file
        self.is_func_like = is_func_like
        self.params = params or []


def parse_macros_from_file(
    filepath: Path, include_func_macros: bool = False
) -> List[MacroDefinition]:
    """
    Parses a .c or .h file to extract `#define` macros, tracking any enclosing
    `#if` / `#ifdef` / `#ifndef` / `#elif` / `#else` conditional compilation blocks.
    When `include_func_macros=True`, also captures function-like macros so they can
    be used in the background to expand object-like macros that call them.
    """
    try:
        raw_text = filepath.read_text(encoding="utf-8", errors="replace")
    except Exception as exc:
        print(f"[WARN] Could not read '{filepath}': {exc}", file=sys.stderr)
        return []

    clean_text = strip_c_comments(raw_text)
    logical_text = join_line_continuations(clean_text)
    lines = logical_text.splitlines()

    cond_stack: List[str] = []
    definitions: List[MacroDefinition] = []

    directive_re = re.compile(r"^\s*#\s*([a-zA-Z_]+)(.*)$")
    define_re = re.compile(r"^([a-zA-Z_][a-zA-Z0-9_]*)(\([^)]*\))?(.*)$")

    potential_guard: Optional[str] = None

    for line in lines:
        m_dir = directive_re.match(line)
        if not m_dir:
            continue

        directive = m_dir.group(1)
        rest = m_dir.group(2).strip()

        if directive == "ifdef":
            cond_stack.append(f"#ifdef {rest}")
        elif directive == "ifndef":
            if not cond_stack and not definitions and rest.endswith("_H"):
                potential_guard = rest
            cond_stack.append(f"#ifndef {rest}")
        elif directive == "if":
            cond_stack.append(f"#if {rest}")
        elif directive == "elif":
            if cond_stack:
                prev = cond_stack.pop()
                cond_stack.append(f"{prev} -> #elif {rest}")
            else:
                cond_stack.append(f"#elif {rest}")
        elif directive == "else":
            if cond_stack:
                prev = cond_stack.pop()
                cond_stack.append(f"#else (after {prev})")
            else:
                cond_stack.append("#else")
        elif directive == "endif":
            if cond_stack:
                cond_stack.pop()
        elif directive == "define":
            m_def = define_re.match(rest)
            if not m_def:
                continue
            macro_name = m_def.group(1)
            param_group = m_def.group(2)
            macro_body = m_def.group(3).strip()
            is_func_like = param_group is not None

            if is_func_like and not include_func_macros:
                continue

            params: List[str] = []
            if is_func_like and param_group:
                inner = param_group[1:-1].strip()
                if inner:
                    params = [p.strip() for p in inner.split(",") if p.strip()]

            if (
                not is_func_like
                and macro_body == ""
                and (
                    macro_name == potential_guard
                    or (cond_stack and cond_stack[0] == f"#ifndef {macro_name}")
                )
            ):
                continue

            active_conds = [
                c
                for c in cond_stack
                if not (potential_guard and c == f"#ifndef {potential_guard}")
                and not (
                    c.startswith("#ifndef ")
                    and c.split()[-1].upper().endswith(("_H", "_H_", "_HPP", "_INCLUDED"))
                )
            ]
            cond_str = " && ".join(active_conds) if active_conds else None

            definitions.append(
                MacroDefinition(
                    name=macro_name,
                    raw_value=macro_body,
                    condition=cond_str,
                    source_file=filepath,
                    is_func_like=is_func_like,
                    params=params,
                )
            )

    return definitions


# ==============================================================================
# Cross-File, Function-Like Macro & C Function Runtime Evaluation
# ==============================================================================
def find_compiler() -> Optional[str]:
    """Finds `gcc` or `clang` on the system PATH."""
    for compiler in ("gcc", "clang", "cc"):
        path = shutil.which(compiler)
        if path:
            return path
    return None


def collect_all_include_dirs(search_dirs: List[Path], target_file: Path) -> List[Path]:
    """
    Collects the target file's parent directory, the predefined root directories,
    and all nested subdirectories within the predefined directories so `#include`
    directives work whether they use relative paths or bare header filenames.
    """
    all_dirs: List[Path] = []
    seen = set()

    for candidate in [target_file.parent] + list(search_dirs):
        if candidate.exists() and candidate.is_dir():
            res = candidate.resolve()
            if res not in seen:
                seen.add(res)
                all_dirs.append(res)

    for d in search_dirs:
        if not d.exists() or not d.is_dir():
            continue
        for root, subdirs, _ in os.walk(d):
            root_path = Path(root).resolve()
            if root_path not in seen:
                seen.add(root_path)
                all_dirs.append(root_path)
            for sd in sorted(subdirs):
                sd_path = (Path(root) / sd).resolve()
                if sd_path not in seen:
                    seen.add(sd_path)
                    all_dirs.append(sd_path)

    return all_dirs


def build_workspace_symbol_tables(
    search_dirs: List[Path],
) -> Tuple[Dict[str, str], Dict[str, Tuple[List[str], str]]]:
    """
    Recursively scans all .h and .c files across the predefined directories and
    their subdirectories to build:
      1. `object_symbols`: `{macro_name: raw_body}` for object-like macros
      2. `func_symbols`: `{macro_name: (params, raw_body)}` for function-like macros
    """
    object_symbols: Dict[str, str] = {}
    func_symbols: Dict[str, Tuple[List[str], str]] = {}

    for d in search_dirs:
        if not d.exists() or not d.is_dir():
            continue
        for root, _, files in os.walk(d):
            for fname in sorted(files):
                if fname.endswith((".h", ".c", ".hpp")):
                    fpath = Path(root) / fname
                    defs = parse_macros_from_file(fpath, include_func_macros=True)
                    for d_item in defs:
                        if d_item.is_func_like:
                            func_symbols[d_item.name] = (d_item.params, d_item.raw_value)
                        else:
                            if d_item.raw_value or d_item.name not in object_symbols:
                                object_symbols[d_item.name] = d_item.raw_value
    return object_symbols, func_symbols


def _split_macro_call_args(arg_str: str) -> List[str]:
    """Splits a comma-separated macro argument list respecting nested parentheses."""
    args: List[str] = []
    current: List[str] = []
    depth = 0
    in_str = False
    escape = False

    for ch in arg_str:
        if escape:
            current.append(ch)
            escape = False
            continue
        if ch == "\\":
            current.append(ch)
            escape = True
            continue
        if ch == '"':
            in_str = not in_str
            current.append(ch)
            continue
        if not in_str:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth = max(0, depth - 1)
            elif ch == "," and depth == 0:
                args.append("".join(current).strip())
                current = []
                continue
        current.append(ch)

    tail = "".join(current).strip()
    if tail or args:
        args.append(tail)
    return args


def _expand_function_like_calls(
    expr: str, func_symbols: Dict[str, Tuple[List[str], str]]
) -> str:
    """
    Expands calls to function-like macros (e.g., `add1(T)` -> `((T) + 1)`) in pure
    Python using `func_symbols`.
    """
    if "(" not in expr or not func_symbols:
        return expr

    ident_re = re.compile(r"\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\(")
    pos = 0
    out: List[str] = []

    while pos < len(expr):
        m = ident_re.search(expr, pos)
        if not m:
            out.append(expr[pos:])
            break

        fn_name = m.group(1)
        open_paren_idx = m.end() - 1

        if fn_name not in func_symbols:
            out.append(expr[pos : m.end()])
            pos = m.end()
            continue

        close_idx = _find_matching_paren(expr, open_paren_idx)
        if close_idx == -1:
            out.append(expr[pos:])
            break

        out.append(expr[pos : m.start()])
        arg_text = expr[open_paren_idx + 1 : close_idx]
        call_args = _split_macro_call_args(arg_text) if arg_text.strip() else []
        params, body = func_symbols[fn_name]

        if len(call_args) == len(params):
            param_map = {p: f"({a})" for p, a in zip(params, call_args)}
            expanded_body = re.sub(
                r"\b([a-zA-Z_][a-zA-Z0-9_]*)\b",
                lambda pm: param_map.get(pm.group(1), pm.group(1)),
                body,
            )
            out.append(f"({expanded_body})")
        else:
            out.append(expr[m.start() : close_idx + 1])

        pos = close_idx + 1

    return "".join(out)


def expand_with_symbol_table(
    raw_expr: str,
    symbol_table: Dict[str, str],
    func_symbols: Optional[Dict[str, Tuple[List[str], str]]] = None,
    local_overrides: Optional[Dict[str, str]] = None,
    max_depth: int = 25,
) -> str:
    """
    Iteratively expands both function-like macro invocations (e.g., `add1(T)`)
    and object-like macro identifiers in `raw_expr`.
    """
    merged = dict(symbol_table)
    if local_overrides:
        merged.update(local_overrides)

    current = raw_expr
    ident_re = re.compile(r"\b([a-zA-Z_][a-zA-Z0-9_]*)\b")

    for _ in range(max_depth):
        prev = current

        if func_symbols:
            current = _expand_function_like_calls(current, func_symbols)

        def _replace_ident(match: re.Match) -> str:
            token = match.group(1)
            if token in merged and merged[token] != "" and merged[token] != token:
                return f"({merged[token]})"
            return token

        if '"' in current:
            parts = re.split(r'("(?:\\.|[^"\\])*")', current)
            new_parts = [
                part if idx % 2 == 1 else ident_re.sub(_replace_ident, part)
                for idx, part in enumerate(parts)
            ]
            current = "".join(new_parts)
        else:
            current = ident_re.sub(_replace_ident, current)

        if current == prev:
            break

    return current


def expand_via_compiler(
    compiler: str,
    target_file: Path,
    search_dirs: List[Path],
    raw_expr: str,
    local_defines: Dict[str, str],
    workspace_symbols: Dict[str, str],
    func_symbols: Dict[str, Tuple[List[str], str]],
) -> Optional[str]:
    """
    Uses `gcc -E -P` or `clang -E -P` to expand `raw_expr` (including both
    object-like and function-like macros across files).
    """
    include_flags = []
    for inc_dir in collect_all_include_dirs(search_dirs, target_file):
        include_flags.extend(["-I", str(inc_dir)])

    probe_lines = []
    for k, v in workspace_symbols.items():
        if v:
            probe_lines.append(f"#ifndef {k}\n#define {k} {v}\n#endif")

    for fn_name, (params, body) in func_symbols.items():
        if body:
            sig = f"{fn_name}({', '.join(params)})"
            probe_lines.append(f"#ifndef {fn_name}\n#define {sig} {body}\n#endif")

    probe_lines.append(f'#include "{target_file.resolve()}"')

    for k, v in local_defines.items():
        probe_lines.append(f"#undef {k}")
        probe_lines.append(f"#define {k} {v}")

    sentinel = "__MACRO_EVAL_SENTINEL__"
    probe_lines.append(f"{sentinel} = {raw_expr}")
    probe_source = "\n".join(probe_lines) + "\n"

    cmd = [compiler, "-E", "-P", "-xc"] + include_flags + ["-"]
    try:
        proc = subprocess.run(
            cmd,
            input=probe_source,
            capture_output=True,
            text=True,
            timeout=5,
        )
        if proc.returncode == 0 and proc.stdout:
            for line in reversed(proc.stdout.splitlines()):
                if sentinel in line:
                    _, _, rhs = line.partition(f"{sentinel} =")
                    return rhs.strip()
    except Exception:
        pass
    return None


def evaluate_via_c_runner(
    compiler: str,
    target_file: Path,
    search_dirs: List[Path],
    expr: str,
    local_defines: Dict[str, str],
    workspace_symbols: Dict[str, str],
    func_symbols: Dict[str, Tuple[List[str], str]],
) -> Optional[Union[int, float, str]]:
    """
    Fallback for macros that invoke actual C functions, `sizeof(...)`, or complex
    C expressions that cannot be reduced by Python AST alone.
    Compiles and runs a tiny C11 probe binary using `_Generic` to print the
    exact signed integer, unsigned integer, floating-point, or string result.
    """
    include_flags = []
    for inc_dir in collect_all_include_dirs(search_dirs, target_file):
        include_flags.extend(["-I", str(inc_dir)])

    extra_c_files: List[str] = []
    main_re = re.compile(r"\bmain\s*\(")
    for d in search_dirs:
        if not d.exists() or not d.is_dir():
            continue
        for root, _, files in os.walk(d):
            for fname in sorted(files):
                if fname.endswith(".c"):
                    c_path = (Path(root) / fname).resolve()
                    if c_path == target_file.resolve():
                        continue
                    try:
                        txt = strip_c_comments(c_path.read_text(encoding="utf-8", errors="ignore"))
                        if not main_re.search(txt):
                            extra_c_files.append(str(c_path))
                    except Exception:
                        pass

    lines = [
        "#include <stdio.h>",
        "#include <stdint.h>",
        "#include <stdbool.h>",
        "#include <string.h>",
        "#include <math.h>",
    ]
    for k, v in workspace_symbols.items():
        if v:
            lines.append(f"#ifndef {k}\n#define {k} {v}\n#endif")
    for fn_name, (params, body) in func_symbols.items():
        if body:
            lines.append(f"#ifndef {fn_name}\n#define {fn_name}({', '.join(params)}) {body}\n#endif")

    lines.append(f'#include "{target_file.resolve()}"')

    for k, v in local_defines.items():
        lines.append(f"#undef {k}")
        lines.append(f"#define {k} {v}")

    lines.append(
        """
int main(void) {
    #define __PRINT_VAL(v) _Generic((v), \\
        char*: printf("STR:%s\\n", (char*)(uintptr_t)(v)), \\
        const char*: printf("STR:%s\\n", (const char*)(uintptr_t)(v)), \\
        float: printf("FLT:%.9g\\n", (double)(v)), \\
        double: printf("FLT:%.17g\\n", (double)(v)), \\
        long double: printf("FLT:%.17g\\n", (double)(v)), \\
        unsigned char: printf("UINT:%llu\\n", (unsigned long long)(v)), \\
        unsigned short: printf("UINT:%llu\\n", (unsigned long long)(v)), \\
        unsigned int: printf("UINT:%llu\\n", (unsigned long long)(v)), \\
        unsigned long: printf("UINT:%llu\\n", (unsigned long long)(v)), \\
        unsigned long long: printf("UINT:%llu\\n", (unsigned long long)(v)), \\
        default: printf("INT:%lld\\n", (long long)(v)) \\
    )
"""
    )
    lines.append(f"    __PRINT_VAL(({expr}));")
    lines.append("    return 0;\n}")
    c_source = "\n".join(lines) + "\n"

    with tempfile.TemporaryDirectory() as tmpdir:
        src_path = Path(tmpdir) / "probe.c"
        bin_path = Path(tmpdir) / "probe_bin"
        src_path.write_text(c_source, encoding="utf-8")

        for extra in (extra_c_files, []):
            cmd = (
                [compiler, "-std=c11", "-w"]
                + include_flags
                + [str(src_path)]
                + extra
                + ["-o", str(bin_path), "-lm"]
            )
            try:
                comp = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
                if comp.returncode == 0 and bin_path.exists():
                    run = subprocess.run([str(bin_path)], capture_output=True, text=True, timeout=3)
                    if run.returncode == 0 and run.stdout:
                        out = run.stdout.strip().splitlines()[-1]
                        if out.startswith("INT:"):
                            return int(out[4:])
                        if out.startswith("UINT:"):
                            return int(out[5:])
                        if out.startswith("FLT:"):
                            return float(out[4:])
                        if out.startswith("STR:"):
                            return out[4:]
            except Exception:
                continue
    return None


def evaluate_macro_definitions(
    defs: List[MacroDefinition],
    target_file: Path,
    search_dirs: List[Path],
    workspace_symbols: Dict[str, str],
    func_symbols: Dict[str, Tuple[List[str], str]],
    compiler: Optional[str],
) -> Dict[str, Any]:
    """
    Groups object-like macro definitions by name, evaluates each definition to
    its concrete value (using function-like macros and C functions in the background
    when needed), and formats conditional macros explicitly by their condition branches.
    """
    grouped: Dict[str, List[MacroDefinition]] = {}
    for d in defs:
        if not d.is_func_like:
            grouped.setdefault(d.name, []).append(d)

    local_unconditional: Dict[str, str] = {}
    for d in defs:
        if not d.is_func_like and d.condition is None:
            local_unconditional[d.name] = d.raw_value

    evaluated_results: Dict[str, Any] = {}

    for name, macro_list in grouped.items():
        has_conditionals = any(m.condition is not None for m in macro_list)

        if not has_conditionals:
            latest_def = macro_list[-1]
            val = _resolve_single_definition(
                latest_def,
                target_file,
                search_dirs,
                local_unconditional,
                workspace_symbols,
                func_symbols,
                compiler,
            )
            evaluated_results[name] = val
        else:
            branch_map: Dict[str, Any] = {}
            for m in macro_list:
                cond_key = m.condition if m.condition is not None else "default (unconditional)"
                branch_overrides = dict(local_unconditional)
                branch_overrides[m.name] = m.raw_value
                val = _resolve_single_definition(
                    m,
                    target_file,
                    search_dirs,
                    branch_overrides,
                    workspace_symbols,
                    func_symbols,
                    compiler,
                )
                branch_map[cond_key] = val
            evaluated_results[name] = branch_map

    return evaluated_results


def _resolve_single_definition(
    m_def: MacroDefinition,
    target_file: Path,
    search_dirs: List[Path],
    local_overrides: Dict[str, str],
    workspace_symbols: Dict[str, str],
    func_symbols: Dict[str, Tuple[List[str], str]],
    compiler: Optional[str],
) -> Any:
    """Resolves a single MacroDefinition to its evaluated value or fallback literal."""
    raw = m_def.raw_value
    if raw == "":
        return ""

    # Step 1: Expand object-like and function-like macros using symbol tables
    sym_expanded = expand_with_symbol_table(
        raw, workspace_symbols, func_symbols, local_overrides
    )

    # Step 2: If a compiler is available, also run C preprocessor (`-E`) expansion
    # Pass `sym_expanded` so any cross-file function-like macros already expanded in Step 1 are preserved
    cpp_expanded = None
    if compiler:
        cpp_expanded = expand_via_compiler(
            compiler=compiler,
            target_file=target_file,
            search_dirs=search_dirs,
            raw_expr=sym_expanded,
            local_defines=local_overrides,
            workspace_symbols=workspace_symbols,
            func_symbols=func_symbols,
        )

    best_expanded = cpp_expanded if cpp_expanded else sym_expanded

    # Step 3: Evaluate constant arithmetic/bitwise/float/cast expression in Python AST
    evaluated = try_evaluate_expression(best_expanded)

    if isinstance(evaluated, str) and cpp_expanded and sym_expanded != cpp_expanded:
        alt_eval = try_evaluate_expression(sym_expanded)
        if not isinstance(alt_eval, str):
            return alt_eval

    # Step 4: If AST evaluation still left an unreduced C expression (e.g. a call to an
    # actual C function, `sizeof(...)`, or complex C expression), compile & execute
    # a tiny C probe via gcc/clang to obtain the concrete value.
    if isinstance(evaluated, str) and compiler:
        for candidate_expr in (best_expanded, sym_expanded, raw):
            c_val = evaluate_via_c_runner(
                compiler=compiler,
                target_file=target_file,
                search_dirs=search_dirs,
                expr=candidate_expr,
                local_defines=local_overrides,
                workspace_symbols=workspace_symbols,
                func_symbols=func_symbols,
            )
            if c_val is not None:
                return c_val

    return evaluated


# ==============================================================================
# File Discovery Across Predefined Directories & Subdirectories
# ==============================================================================
def locate_target_file(file_arg: str, search_dirs: List[Path]) -> Path:
    """
    Locates `file_arg` across the two predefined directories and all of their
    subdirectories (without priority).
    """
    direct_path = Path(file_arg)
    if direct_path.is_file():
        return direct_path.resolve()

    req_parts = Path(file_arg).parts
    matches: List[Path] = []
    seen_resolved = set()

    for d in search_dirs:
        if not d.exists() or not d.is_dir():
            continue
        candidate = d / file_arg
        if candidate.is_file():
            res = candidate.resolve()
            if res not in seen_resolved:
                seen_resolved.add(res)
                matches.append(res)
        for root, _, files in os.walk(d):
            if Path(file_arg).name in files:
                found = (Path(root) / Path(file_arg).name).resolve()
                if found not in seen_resolved:
                    if len(found.parts) >= len(req_parts) and found.parts[-len(req_parts):] == req_parts:
                        seen_resolved.add(found)
                        matches.append(found)

    if not matches:
        dirs_str = ", ".join(f"'{d}'" for d in search_dirs)
        print(
            f"[ERROR] File '{file_arg}' was not found in any of the predefined directories "
            f"or their subdirectories ({dirs_str}).",
            file=sys.stderr,
        )
        sys.exit(1)

    if len(matches) > 1:
        print(
            f"[ERROR] Multiple files matching '{file_arg}' were found across the predefined directories/subdirectories "
            "(no priority is set):",
            file=sys.stderr,
        )
        for m in matches:
            print(f"  - {m}", file=sys.stderr)
        print(
            "Please pass a disambiguating relative path (e.g., 'subdir/config.h' or 'dir1/subdir/config.h').",
            file=sys.stderr,
        )
        sys.exit(1)

    return matches[0]


# ==============================================================================
# JSON Storage & Update Engine
# ==============================================================================
def load_json_store(json_path: Path) -> Dict[str, Dict[str, Any]]:
    """Loads existing macros JSON file or returns an empty dictionary."""
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
        print(f"[ERROR] Failed to parse existing JSON file '{json_path}': {exc}", file=sys.stderr)
        sys.exit(1)


def update_json_macros(
    store: Dict[str, Dict[str, Any]],
    file_key: str,
    extracted_macros: Dict[str, Any],
) -> Tuple[int, int, int]:
    """
    Inserts or updates macros under `store[file_key]` and prints explicit
    action messages for each macro.
    """
    if file_key not in store or not isinstance(store[file_key], dict):
        store[file_key] = {}
        print(f"[INFO] Created new file section '{file_key}' in JSON store.")

    file_bucket = store[file_key]
    added_count = 0
    updated_count = 0
    unchanged_count = 0

    for macro_name, new_val in extracted_macros.items():
        if macro_name not in file_bucket:
            file_bucket[macro_name] = new_val
            added_count += 1
            print(
                f"[ADDED]     [{file_key}] Macro '{macro_name}' added with value: "
                f"{json.dumps(new_val)}"
            )
        else:
            old_val = file_bucket[macro_name]
            if old_val != new_val or type(old_val) is not type(new_val):
                file_bucket[macro_name] = new_val
                updated_count += 1
                print(
                    f"[UPDATED]   [{file_key}] Macro '{macro_name}' updated from "
                    f"{json.dumps(old_val)} -> {json.dumps(new_val)}"
                )
            else:
                unchanged_count += 1
                print(
                    f"[UNCHANGED] [{file_key}] Macro '{macro_name}' is already up to date "
                    f"({json.dumps(new_val)})"
                )

    return added_count, updated_count, unchanged_count


# ==============================================================================
# CLI Entry Point
# ==============================================================================
def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Extract and evaluate C/C++ object-like macros from .c or .h files "
            "and store/update them in a root JSON file."
        )
    )
    parser.add_argument(
        "-f",
        "--file",
        required=True,
        help="Name (or relative path) of the .c or .h file to inspect.",
    )

    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="Grab all object-like macros from the specified file.",
    )
    mode_group.add_argument(
        "-m",
        "--macro",
        metavar="MACRO_NAME",
        help="Grab a specific macro by name from the specified file.",
    )
    
    parser.add_argument(
        "-d",
        "--dirs",
        nargs=2,
        metavar=("DIR1", "DIR2"),
        default=PREDEFINED_DIRS,
        help=(
            f"Override the two predefined search directories "
            f"(default: {PREDEFINED_DIRS[0]} {PREDEFINED_DIRS[1]})."
        ),
    )
    parser.add_argument(
        "-o",
        "--output",
        default=DEFAULT_JSON_OUTPUT,
        help=f"Path to the output JSON file (default: {DEFAULT_JSON_OUTPUT}).",
    )
    return parser


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    search_dirs = [Path(d) for d in args.dirs]
    json_path = Path(args.output)

    print(
        f"[INFO] Searching for '{args.file}' in directories (and subdirectories): "
        f"'{search_dirs[0]}' and '{search_dirs[1]}'..."
    )
    target_file = locate_target_file(args.file, search_dirs)
    file_key = target_file.name
    print(f"[INFO] Located target file: '{target_file}' (JSON key: '{file_key}')")

    compiler = find_compiler()
    if compiler:
        print(f"[INFO] Using C compiler preprocessor: '{compiler}'")
    else:
        print(
            "[INFO] No gcc/clang found on PATH; using built-in cross-file AST macro evaluator."
        )

    # Build cross-file symbol tables (both object-like and function-like macros)
    workspace_symbols, func_symbols = build_workspace_symbol_tables(search_dirs)

    # Parse target file definitions (including local function-like macros for background use)
    defs = parse_macros_from_file(target_file, include_func_macros=True)
    for d in defs:
        if d.is_func_like:
            func_symbols[d.name] = (d.params, d.raw_value)

    object_defs = [d for d in defs if not d.is_func_like]
    if not object_defs:
        print(f"[INFO] No object-like macros found in '{target_file}'.")
        sys.exit(0)

    # Evaluate object-like macros
    evaluated_all = evaluate_macro_definitions(
        defs=object_defs,
        target_file=target_file,
        search_dirs=search_dirs,
        workspace_symbols=workspace_symbols,
        func_symbols=func_symbols,
        compiler=compiler,
    )

    # Filter if specific macro requested
    if args.macro:
        target_macro = args.macro
        if target_macro not in evaluated_all:
            print(
                f"[ERROR] Macro '{target_macro}' was not found in '{file_key}' "
                "(or is a function-like macro which is skipped).",
                file=sys.stderr,
            )
            sys.exit(1)
        selected_macros = {target_macro: evaluated_all[target_macro]}
        print(f"[INFO] Extracting specific macro '{target_macro}' from '{file_key}'...")
    else:
        selected_macros = evaluated_all
        print(
            f"[INFO] Extracting all ({len(selected_macros)}) object-like macros from '{file_key}'..."
        )

    # Load JSON store, update entries, and save
    store = load_json_store(json_path)
    added, updated, unchanged = update_json_macros(store, file_key, selected_macros)

    json_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(store, indent=2) + "\n", encoding="utf-8")

    print(
        f"[SUMMARY] Saved '{json_path}' | File: '{file_key}' | "
        f"Added: {added}, Updated: {updated}, Unchanged: {unchanged}"
    )


if __name__ == "__main__":
    main()