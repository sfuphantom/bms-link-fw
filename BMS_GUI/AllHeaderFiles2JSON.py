#!/usr/bin/env python3
"""
AllHeaderFiles2JSON.py

Walks a C codebase, extracts every plain #define constant from all .h files,
resolves #include dependencies (dependent headers are parsed BEFORE the files
that include them), evaluates #if / #ifdef / #ifndef / #elif / #else / #endif
so that only constants that would actually survive that conditional branch
are kept, evaluates each #define to its compile-time value where possible,
and excludes register / memory-mapped-pointer style defines.

Output JSON shape mirrors the folder hierarchy:

    {
      "FolderA": {
        "FolderB": {
          "some_header.h": { "SOME_CONST": 42, "OTHER_CONST": "foo" }
        }
      }
    }

--------------------------------------------------------------------------
IMPORTANT LIMITATIONS (read before trusting the output blindly)
--------------------------------------------------------------------------
This is a lightweight, regex-based reimplementation of (a subset of) the C
preprocessor -- it is NOT a real one. It handles the common embedded-C
patterns (simple #define constants, nested #ifdef/#ifndef/#if/#elif/#else,
#include "x.h", integer/hex literals, arithmetic, shifts, defined()) but it
will NOT correctly handle:
  - function-like macros used as constants (e.g. #define MHZ(x) ((x)*1000000))
  - token-pasting (##) or stringizing (#)
  - macros defined via the compiler command line (-D flags)
  - multi-line function-like macro invocations
  - C integer promotion / overflow / division-truncation edge cases
If you have `gcc`/`cpp` available, running `gcc -E -dM` on each header (with
-I for every folder that contains a header) will give you a fully correct
macro table and is worth doing as a cross-check against this script's output.
--------------------------------------------------------------------------
"""

import ast
import json
import os
import re


# --------------------------------------------------------------------------
# Configuration - tweak these for your repo layout
# --------------------------------------------------------------------------

GIT_DIR = '\\'.join(os.getcwd().split('\\')[:-1])
C_CODE_FOLDER = "bms-link-FW"
C_CODE_ROOT_DIR = os.path.join(GIT_DIR, C_CODE_FOLDER)

FOLDERS2IGNORE = {"source", ".settings", "Debug", ".launches", "Release", "Test", "targetConfigs", "BMS_Master"}
HFILES2IGNORE = {"PhantomHelpers.h", "ltc6811_commands.h", "sci_helpers.h", "spi_helpers.h", "GIO_helpers.h"}
OUTPUT_JSON = "AllHeaderConstants.json"

# Heuristics used to decide whether a #define is a register / memory-mapped
# pointer rather than a "real" constant that should end up in the JSON.
# Tune these patterns to match your codebase's naming conventions.
REGISTER_NAME_HINTS = re.compile(
    r'(_REG(\b|_)|_REGISTER(\b|_)|_ADDR(\b|_)|_ADDRESS(\b|_)|_BASE(\b|_)|_PTR(\b|_))',
    re.IGNORECASE,
)
POINTER_DEREF_RE = re.compile(r'\*\s*\(\s*\(?\s*(?:volatile\s+)?[\w\s]+\*\s*\)?\s*\)')
CAST_TO_PTR_RE = re.compile(r'\(\s*\(?\s*(?:volatile\s+)?[\w\s]+\*\s*\)\s*0[xX][0-9A-Fa-f]+')


# --------------------------------------------------------------------------
# Regexes for stripping comments and matching preprocessor directives
# --------------------------------------------------------------------------

BLOCK_COMMENT_RE = re.compile(r'/\*.*?\*/', re.DOTALL)
LINE_COMMENT_RE = re.compile(r'//.*')
LINE_CONTINUATION_RE = re.compile(r'\\\r?\n')

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+[<"]([^">]+)[">]')
DEFINE_RE = re.compile(r'^\s*#\s*define\s+(\w+)(\([^)]*\))?\s*(.*)$')
UNDEF_RE = re.compile(r'^\s*#\s*undef\s+(\w+)')
IFDEF_RE = re.compile(r'^\s*#\s*ifdef\s+(\w+)')
IFNDEF_RE = re.compile(r'^\s*#\s*ifndef\s+(\w+)')
IF_RE = re.compile(r'^\s*#\s*if\s+(.*)$')
ELIF_RE = re.compile(r'^\s*#\s*elif\s+(.*)$')
ELSE_RE = re.compile(r'^\s*#\s*else\b')
ENDIF_RE = re.compile(r'^\s*#\s*endif\b')

DEFINED_RE = re.compile(r'defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)')
TOKEN_RE = re.compile(r'\b[A-Za-z_]\w*\b')
INT_SUFFIX_RE = re.compile(r'\b(0[xX][0-9A-Fa-f]+|\d+)[uUlL]+\b')
CAST_RE = re.compile(
    r'\(\s*(?:const\s+|volatile\s+|unsigned\s+|signed\s+)*'
    r'(?:u?int(?:8|16|32|64)_t|int|long|short|char|float|double|size_t|bool)\s*\)'
)
# Any "(type *)" / "(type **)" style pointer cast, e.g. (void *), (uint8_t*),
# (struct foo *). Stripped so expressions like "(void *) 0" reduce to "0".
POINTER_CAST_RE = re.compile(
    r'\(\s*(?:const\s+|volatile\s+|unsigned\s+|signed\s+|struct\s+|enum\s+)*'
    r'[A-Za-z_]\w*\s*(?:\*\s*)+\)'
)
NULL_RE = re.compile(r'\bNULL\b')

ALLOWED_EVAL_NAMES = {'True', 'False', 'and', 'or', 'not'}


# --------------------------------------------------------------------------
# Filesystem walk
# --------------------------------------------------------------------------

def find_h_files(root_folder):
    """Return every .h file under root_folder, pruning ignored folders."""
    h_files = []
    
    for dirpath, dirnames, filenames in os.walk(root_folder):
        # prune ignored folders in-place so os.walk doesn't descend into them
        dirnames[:] = [d for d in dirnames if d not in FOLDERS2IGNORE]
        for filename in filenames:
            if filename in HFILES2IGNORE:
                continue
            if not filename.endswith('.h'):
                continue

            h_files.append(os.path.join(dirpath, filename))
    return h_files


def build_basename_map(all_files):
    """Map bare filename ('foo.h') -> first full path found with that name."""
    mapping = {}
    dupes = {}
    for f in all_files:
        b = os.path.basename(f)
        if b not in mapping:
            mapping[b] = f
        else:
            dupes.setdefault(b, [mapping[b]]).append(f)
    if dupes:
        print("WARNING: duplicate header basenames found (first match used for #include resolution):")
        for name, paths in dupes.items():
            print(f"  {name}:")
            for p in paths:
                print(f"    {p}")
    return mapping


# --------------------------------------------------------------------------
# Text cleanup
# --------------------------------------------------------------------------

def strip_comments_and_join_continuations(text):
    text = BLOCK_COMMENT_RE.sub(' ', text)
    text = LINE_COMMENT_RE.sub('', text)
    text = LINE_CONTINUATION_RE.sub('', text)
    return text


# --------------------------------------------------------------------------
# Register / pointer exclusion heuristic
# --------------------------------------------------------------------------

def is_register_or_pointer_define(name, raw_value):
    if REGISTER_NAME_HINTS.search(name):
        return True
    if POINTER_DEREF_RE.search(raw_value):
        return True
    if CAST_TO_PTR_RE.search(raw_value):
        return True
    if raw_value.strip().startswith('*'):
        return True
    return False


# --------------------------------------------------------------------------
# Expression cleanup / macro substitution / evaluation
# --------------------------------------------------------------------------

def clean_c_literal(expr):
    expr = INT_SUFFIX_RE.sub(lambda m: m.group(1), expr)
    expr = CAST_RE.sub('', expr)
    expr = POINTER_CAST_RE.sub('', expr)
    expr = NULL_RE.sub('0', expr)
    return expr


def substitute_macros(expr, obj_eval, obj_raw, depth=0):
    if depth > 15:
        return expr

    def repl(m):
        tok = m.group(0)
        if tok in obj_eval:
            return f'({obj_eval[tok]!r})' if isinstance(obj_eval[tok], str) else f'({obj_eval[tok]})'
        if tok in obj_raw:
            return f'({obj_raw[tok]})'
        return tok

    new_expr = TOKEN_RE.sub(repl, expr)
    if new_expr != expr:
        return substitute_macros(new_expr, obj_eval, obj_raw, depth + 1)
    return new_expr


def safe_eval(expr):
    """Evaluate a sanitized arithmetic/boolean expression without exec risk.
    Never raises -- any failure (syntax error, disallowed node, runtime
    error such as division by zero) just yields None, meaning "couldn't
    resolve this one, move on"."""
    if not expr or not expr.strip():
        return None

    try:
        node = ast.parse(expr, mode='eval')

        allowed_nodes = (
            ast.Expression, ast.BinOp, ast.UnaryOp, ast.BoolOp, ast.Compare,
            ast.Num, ast.Constant, ast.Name, ast.Load,
            ast.Add, ast.Sub, ast.Mult, ast.Div, ast.FloorDiv, ast.Mod, ast.Pow,
            ast.LShift, ast.RShift, ast.BitOr, ast.BitAnd, ast.BitXor, ast.Invert,
            ast.Not, ast.UAdd, ast.USub, ast.And, ast.Or,
            ast.Eq, ast.NotEq, ast.Lt, ast.LtE, ast.Gt, ast.GtE,
        )
        for n in ast.walk(node):
            if not isinstance(n, allowed_nodes):
                return None
            if isinstance(n, ast.Name) and n.id not in ALLOWED_EVAL_NAMES:
                return None

        return eval(compile(node, '<expr>', 'eval'), {"__builtins__": {}}, {})
    except Exception:
        return None


def evaluate_define(raw_value, obj_eval, obj_raw):
    """Best-effort evaluation of a #define's value to a Python int/float/bool."""
    if not raw_value or not raw_value.strip():
        return None
    try:
        expr = clean_c_literal(raw_value)
        expr = substitute_macros(expr, obj_eval, obj_raw)
        expr = clean_c_literal(expr)
        expr = re.sub(r'!(?!=)', ' not ', expr)
        expr = expr.replace('&&', ' and ').replace('||', ' or ')
    except Exception:
        return None
    val = safe_eval(expr)
    if isinstance(val, (int, float, bool)):
        return val
    return None


def eval_condition(expr, obj_eval, obj_raw):
    """Evaluate a #if / #elif expression to True/False (best effort)."""

    def defined_repl(m):
        name = m.group(1) or m.group(2)
        return '1' if (name in obj_raw or name in obj_eval) else '0'

    try:
        expr = DEFINED_RE.sub(defined_repl, expr)
        expr = clean_c_literal(expr)
        expr = substitute_macros(expr, obj_eval, obj_raw)
        expr = clean_c_literal(expr)
        expr = re.sub(r'!(?!=)', ' not ', expr)
        expr = expr.replace('&&', ' and ').replace('||', ' or ')

        def ident_repl(m):
            tok = m.group(0)
            return tok if tok in ALLOWED_EVAL_NAMES else '0'

        expr = TOKEN_RE.sub(ident_repl, expr)
    except Exception:
        return False

    val = safe_eval(expr)
    if val is None:
        # Can't resolve it (e.g. depends on an undefined build-system macro).
        # Default to False so we don't silently pull in constants that may
        # not actually be compiled in.
        return False
    return bool(val)


# --------------------------------------------------------------------------
# Core per-file processing
# --------------------------------------------------------------------------

def process_all(root_dir):
    all_h_files = find_h_files(root_dir)
    if not all_h_files:
        print(f"No .h files found under {root_dir}")
        return {}

    basename_to_path = build_basename_map(all_h_files)

    obj_raw = {}          # object-like macro name -> raw value string
    obj_eval = {}         # object-like macro name -> evaluated python value
    func_like_names = set()

    file_constants = {}   # full path -> {name: value}
    processed = set()
    processing = set()

    def process_file(path):
        if path in processed or path in processing:
            return
        if not os.path.isfile(path):
            print(f"WARNING: could not find file on disk: {path}")
            return

        processing.add(path)
        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                raw_text = f.read()
        except OSError as e:
            print(f"WARNING: could not read {path}: {e}")
            processing.discard(path)
            processed.add(path)
            return

        text = strip_comments_and_join_continuations(raw_text)
        local_constants = {}
        cond_stack = []  # each frame: {'active','taken','parent_active'}

        for line_no, line in enumerate(text.splitlines(), start=1):
          try:
              m = IFDEF_RE.match(line)
              if m:
                  parent_active = cond_stack[-1]['active'] if cond_stack else True
                  name = m.group(1)
                  cond = (name in obj_raw) or (name in obj_eval) or (name in func_like_names)
                  cond_stack.append({'active': parent_active and cond, 'taken': cond, 'parent_active': parent_active})
                  continue

              m = IFNDEF_RE.match(line)
              if m:
                  parent_active = cond_stack[-1]['active'] if cond_stack else True
                  name = m.group(1)
                  cond = not ((name in obj_raw) or (name in obj_eval) or (name in func_like_names))
                  cond_stack.append({'active': parent_active and cond, 'taken': cond, 'parent_active': parent_active})
                  continue

              m = IF_RE.match(line)
              if m:
                  parent_active = cond_stack[-1]['active'] if cond_stack else True
                  cond = eval_condition(m.group(1), obj_eval, obj_raw) if parent_active else False
                  cond_stack.append({'active': parent_active and cond, 'taken': cond, 'parent_active': parent_active})
                  continue

              m = ELIF_RE.match(line)
              if m and cond_stack:
                  frame = cond_stack[-1]
                  if not frame['taken'] and frame['parent_active']:
                      cond = eval_condition(m.group(1), obj_eval, obj_raw)
                      frame['active'] = frame['parent_active'] and cond
                      frame['taken'] = frame['taken'] or cond
                  else:
                      frame['active'] = False
                  continue

              m = ELSE_RE.match(line)
              if m and cond_stack:
                  frame = cond_stack[-1]
                  frame['active'] = frame['parent_active'] and (not frame['taken'])
                  frame['taken'] = True
                  continue

              m = ENDIF_RE.match(line)
              if m and cond_stack:
                  cond_stack.pop()
                  continue

              active = cond_stack[-1]['active'] if cond_stack else True
              if not active:
                  continue

              m = INCLUDE_RE.match(line)
              if m:
                  inc_name = os.path.basename(m.group(1))
                  inc_path = basename_to_path.get(inc_name)
                  if inc_path:
                      process_file(inc_path)
                  continue

              m = UNDEF_RE.match(line)
              if m:
                  name = m.group(1)
                  obj_raw.pop(name, None)
                  obj_eval.pop(name, None)
                  func_like_names.discard(name)
                  continue

              m = DEFINE_RE.match(line)
              if m:
                  name, params, value = m.groups()
                  value = (value or '').strip()

                  if params is not None:
                      # function-like macro: can't safely evaluate as a constant.
                      func_like_names.add(name)
                      continue

                  if is_register_or_pointer_define(name, value):
                      continue

                  obj_raw[name] = value

                  if value == '':
                      # Empty-value defines are almost always #include guards
                      # (e.g. "#ifndef FOO_H / #define FOO_H"), not real
                      # constants, so keep them out of the JSON but still let
                      # them satisfy defined()/#ifdef checks above.
                      continue

                  evaluated = evaluate_define(value, obj_eval, obj_raw)
                  if evaluated is not None:
                      obj_eval[name] = evaluated
                      local_constants[name] = evaluated
                  else:
                      # keep the raw text so downstream files can still chain off it,
                      # and record it in the output so nothing is silently dropped.
                      local_constants[name] = value
                  continue
          except Exception as e:
            print(f"WARNING: {os.path.basename(path)} line {line_no}: skipping ('{e}') - {line.strip()[:80]!r}")
            continue

        processing.discard(path)
        processed.add(path)
        file_constants[path] = local_constants

    for hfile in all_h_files:
        try:
            process_file(hfile)
        except Exception as e:
            print(f"WARNING: failed to process {hfile}: {e!r} (skipping)")

    return file_constants


# --------------------------------------------------------------------------
# JSON assembly
# --------------------------------------------------------------------------

def build_nested_json(file_constants, root_dir):
    result = {}
    for path, consts in file_constants.items():
        if not consts:
            continue
        rel = os.path.relpath(path, root_dir)
        parts = rel.split(os.sep)
        d = result
        for p in parts[:-1]:
            d = d.setdefault(p, {})
        d[parts[-1]] = consts
    return result


def main():
    print(f"Scanning: {C_CODE_ROOT_DIR}")
    file_constants = process_all(C_CODE_ROOT_DIR)

    result = build_nested_json(file_constants, C_CODE_ROOT_DIR)

    with open(OUTPUT_JSON, 'w', encoding='utf-8') as out:
        json.dump(result, out, indent=2)

    total_consts = sum(len(c) for c in file_constants.values())
    total_files = sum(1 for c in file_constants.values() if c)
    print(f"Wrote {total_consts} constants from {total_files} files to {OUTPUT_JSON}")


if __name__ == '__main__':
    main()
