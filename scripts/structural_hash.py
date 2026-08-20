#!/usr/bin/env python3
"""
scripts/structural_hash.py
AST-level structural duplicate detector for generated C++ headers.
"""
import hashlib
import json
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CAS_COMMIT = os.path.join(SCRIPT_DIR, "cas_commit.sh")
DEFAULT_TRANSFORM = os.path.join(SCRIPT_DIR, "transforms", "manifest_transform.sh")

class DuplicateError(Exception):
    pass

class HashingError(Exception):
    pass

def canonicalize(node: dict, counter: dict, seen: dict) -> str:
    kind = node.get("kind", "")
    parts = [kind]
    for field in ("name", "value"):
        if field in node:
            key = (field, node[field])
            if key not in seen:
                seen[key] = f"{field.upper()}_{counter['n']}"
                counter["n"] += 1
            parts.append(seen[key])
    for child in node.get("inner", []):
        parts.append(canonicalize(child, counter, seen))
    return "(" + " ".join(parts) + ")"

def structural_hash(filepath: str) -> str:
    try:
        result = subprocess.run(
            ["clang", "-Xclang", "-ast-dump=json", "-fsyntax-only", filepath],
            capture_output=True, text=True, check=True,
        )
    except FileNotFoundError:
        raise HashingError("clang not found on PATH")
    except subprocess.CalledProcessError as e:
        raise HashingError(f"clang failed to parse {filepath}: {e.stderr.strip()}")
    
    try:
        ast = json.loads(result.stdout)
    except json.JSONDecodeError as e:
        raise HashingError(f"clang produced invalid AST JSON for {filepath}: {e}")
    
    canonical = canonicalize(ast, {"n": 0}, {})
    return hashlib.sha256(canonical.encode()).hexdigest()

def register_hash(new_hash: str) -> None:
    cas_commit = os.environ.get("CAS_COMMIT_SCRIPT", DEFAULT_CAS_COMMIT)
    transform = os.environ.get("MANIFEST_TRANSFORM_SCRIPT", DEFAULT_TRANSFORM)
    repo = os.environ.get("GITHUB_REPOSITORY", "Yansh71/SWAYAM-AGI-")
    branch = os.environ.get("MANIFEST_BRANCH", "state/manifest")
    manifest_path = os.environ.get("MANIFEST_PATH", "manifest.json")
    
    proc = subprocess.run(
        ["bash", cas_commit, repo, manifest_path, branch, transform, new_hash],
        capture_output=True, text=True,
    )
    if proc.returncode == 0:
        return
    if "structural duplicate" in proc.stderr:
        raise DuplicateError(proc.stderr.strip())
    raise RuntimeError(f"CAS commit failed (exit {proc.returncode}): {proc.stderr.strip() or proc.stdout.strip()}")

def resolve_inputs(argv: list) -> list:
    if not argv:
        sys.exit("[structural_hash] no input files given - failing closed")
    missing = [f for f in argv if not os.path.isfile(f)]
    if missing:
        sys.exit(f"[structural_hash] input(s) not found on disk: {missing}")
    return argv

def main():
    files = resolve_inputs(sys.argv[1:])
    failures = []
    for filepath in files:
        try:
            h = structural_hash(filepath)
        except HashingError as e:
            print(f"[structural_hash] HASH ERROR {filepath}: {e}", file=sys.stderr)
            failures.append(filepath)
            continue
        
        try:
            register_hash(h)
            print(f"[structural_hash] ACCEPTED {filepath} ({h[:12]}...)")
        except DuplicateError as e:
            print(f"[structural_hash] DUPLICATE {filepath}: {e}", file=sys.stderr)
            failures.append(filepath)
        except RuntimeError as e:
            print(f"[structural_hash] CAS ERROR {filepath}: {e}", file=sys.stderr)
            failures.append(filepath)
    
    if failures:
        sys.exit(f"[structural_hash] FAILED {len(failures)}/{len(files)} file(s) rejected or unhashable: {failures}")
    print(f"[structural_hash] OK {len(files)} file(s) structurally novel and registered.")

if __name__ == "__main__":
    main()
