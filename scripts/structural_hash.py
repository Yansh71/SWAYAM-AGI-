#!/usr/bin/env python3
"""
structural_hash.py
AST-level duplicate detection for SWAYAM-AGI mutations.
Computes a canonical structural hash of a generated C++ file (identifiers
and literal values normalized, control/type structure preserved), then
registers it against manifest.json on the protected state/manifest
branch via cas_commit.sh the same CAS helper the rate-limiter uses, so
the agent's own sandbox can never write or poison its duplicate-detection
history directly. This script must run in the publisher job (which holds
GH_TOKEN), never inside the agent sandbox.
"""

import json
import subprocess
import hashlib
import sys
import os

REPO = "Yansh71/SWAYAM-AGI-"
BRANCH = "state/manifest"
MANIFEST_PATH = "manifest.json"

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CAS_COMMIT = os.path.join(SCRIPT_DIR, "cas_commit.sh")
TRANSFORM = os.path.join(SCRIPT_DIR, "transforms", "manifest_transform.sh")

def canonicalize(node: dict, counter: dict, seen: dict) -> str:
    """Strip identifier spellings/literal values but keep them distinguishable
    from each other (a+a canonicalizes differently from a+b), so renaming
    alone can't dodge the duplicate check."""
    kind = node.get("kind", "")
    parts = [kind]

    for field in ("name", "value"):
        if field in node:
            key = (field, node[field])
            if key not in seen:
                seen[key] = f"{field.upper()} {counter['n']}"
                counter["n"] += 1
            parts.append(seen[key])

    for child in node.get("inner", []):
        parts.append(canonicalize(child, counter, seen))
        
    return "(" + " ".join(parts) + ")"

def structural_hash(filepath: str) -> str:
    result = subprocess.run(
        ["clang", "-Xclang", "-ast-dump=json", "-fsyntax-only", filepath],
        capture_output=True, text=True, check=True,
    )
    ast = json.loads(result.stdout)
    canonical = canonicalize(ast, {"n": 0}, {})
    return hashlib.sha256(canonical.encode()).hexdigest()

def register_hash(new_hash: str) -> None:
    if "GH_TOKEN" not in os.environ:
        sys.exit("GH_TOKEN must be set publisher token only, never the agent's env")
        
    proc = subprocess.run(
        [CAS_COMMIT, REPO, MANIFEST_PATH, BRANCH, TRANSFORM, new_hash],
        capture_output=True, text=True,
    )
    
    if proc.returncode != 0:
        print(proc.stderr, file=sys.stderr)
        sys.exit("REJECTED: duplicate structure or CAS failure see stderr above")
        
    print(f"OK: novel structure registered ({new_hash[:12]})")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: structural_hash.py <generated-file.h>")
    h = structural_hash(sys.argv[1])
    register_hash(h)
