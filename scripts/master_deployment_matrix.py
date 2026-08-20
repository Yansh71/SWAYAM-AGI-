#!/usr/bin/env python3
"""
scripts/master_deployment_matrix.py
Real verification harness for the SWAYAM-AGI CI/CD pipeline.
"""
import argparse
import hashlib
import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

RESULTS = [] # (name, status, detail)

def record(name, status, detail=""):
    RESULTS.append((name, status, detail))
    print(f"[{status}] {name}" + (f" - {detail}" if detail else ""))

def find_repo_root(start):
    cur = os.path.abspath(start)
    for _ in range(6):
        if os.path.isdir(os.path.join(cur, ".github", "workflows")):
            return cur
        parent = os.path.dirname(cur)
        if parent == cur:
            break
        cur = parent
    return os.path.abspath(os.path.join(start, ".."))

def check_safe_extract(repo_root):
    name = "safe_extract.py rejects path traversal / accepts benign archive"
    extractor = os.path.join(repo_root, "scripts", "safe_extract.py")
    if not os.path.isfile(extractor):
        record(name, "FAIL", f"not found at {extractor}")
        return
    with tempfile.TemporaryDirectory() as tmp:
        good_zip = os.path.join(tmp, "good.zip")
        evil_zip = os.path.join(tmp, "evil.zip")
        dest_good = os.path.join(tmp, "dest_good")
        dest_evil = os.path.join(tmp, "dest_evil")
        
        with zipfile.ZipFile(good_zip, "w") as z:
            z.writestr("manifest.json", "{}")
        with zipfile.ZipFile(evil_zip, "w") as z:
            z.writestr("../../evil.txt", "pwned")
            
        good = subprocess.run([sys.executable, extractor, good_zip, dest_good], capture_output=True, text=True)
        evil = subprocess.run([sys.executable, extractor, evil_zip, dest_evil], capture_output=True, text=True)
        
        good_ok = good.returncode == 0 and os.path.isfile(os.path.join(dest_good, "manifest.json"))
        evil_rejected = evil.returncode != 0
        dest_evil_clean = (not os.path.isdir(dest_evil)) or (len(os.listdir(dest_evil)) == 0)
        
        if good_ok and evil_rejected and dest_evil_clean:
            record(name, "PASS")
        else:
            record(name, "FAIL", f"good_ok={good_ok} evil_rejected={evil_rejected} dest_evil_clean={dest_evil_clean}")

def check_workflow_permissions(repo_root):
    workflows_dir = os.path.join(repo_root, ".github", "workflows")
    if not os.path.isdir(workflows_dir):
        record("workflow permissions", "FAIL", f"no workflows dir at {workflows_dir}")
        return
    
    files = sorted(f for f in os.listdir(workflows_dir) if f.endswith((".yml", ".yaml")))
    if not files:
        record("workflow permissions", "FAIL", "no workflow files found")
        return
        
    for fname in files:
        path = os.path.join(workflows_dir, fname)
        name = f"{fname} declares least-privilege permissions"
        with open(path) as f:
            text = f.read()
        
        has_block = "\npermissions:" in ("\n" + text)
        is_write_all = "permissions:\n  write-all" in text
        
        if not has_block:
            record(name, "FAIL", "no `permissions:` block found")
        elif is_write_all:
            record(name, "FAIL", "uses write-all")
        else:
            record(name, "PASS", "has a permissions: block")

def check_glob_fail_closed(repo_root):
    script = os.path.join(repo_root, "scripts", "structural_hash.py")
    name = "structural_hash.py fails closed on zero input files"
    if not os.path.isfile(script):
        record(name, "FAIL", f"not found at {script}")
        return
    
    proc = subprocess.run([sys.executable, script], capture_output=True, text=True)
    record(name, "PASS" if proc.returncode != 0 else "FAIL", f"exit code {proc.returncode} (expected non-zero)")

def _load_structural_hash_module(repo_root):
    path = os.path.join(repo_root, "scripts", "structural_hash.py")
    spec = importlib.util.spec_from_file_location("structural_hash", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

def check_canonicalize_rename_invariance(repo_root):
    name = "canonicalize() is rename-invariant but detects real structural changes"
    try:
        mod = _load_structural_hash_module(repo_root)
    except Exception as e:
        record(name, "FAIL", f"could not load structural_hash.py: {e}")
        return
        
    ast_a = {"kind": "TranslationUnitDecl", "inner": [{"kind": "FunctionDecl", "name": "computeTotal", "inner": [{"kind": "IntegerLiteral", "value": "42"}]}]}
    ast_b = {"kind": "TranslationUnitDecl", "inner": [{"kind": "FunctionDecl", "name": "sumThings", "inner": [{"kind": "IntegerLiteral", "value": "99"}]}]}
    ast_c = {"kind": "TranslationUnitDecl", "inner": [{"kind": "FunctionDecl", "name": "computeTotal", "inner": [{"kind": "IntegerLiteral", "value": "42"}, {"kind": "IntegerLiteral", "value": "7"}]}]}
    
    def h(ast):
        return hashlib.sha256(mod.canonicalize(ast, {"n": 0}, {}).encode()).hexdigest()
        
    hash_a, hash_b, hash_c = h(ast_a), h(ast_b), h(ast_c)
    rename_invariant = hash_a == hash_b
    detects_real_diff = hash_a != hash_c
    
    if rename_invariant and detects_real_diff:
        record(name, "PASS")
    else:
        record(name, "FAIL", f"rename_invariant={rename_invariant} detects_real_diff={detects_real_diff}")

def check_duplicate_registration_contract(repo_root):
    name = "manifest_transform.sh + register_hash() reject a resubmitted hash"
    if shutil.which("jq") is None:
        record(name, "SKIP", "jq not found")
        return
        
    transform = os.path.join(repo_root, "scripts", "transforms", "manifest_transform.sh")
    mock_cas = os.path.join(repo_root, "scripts", "testing", "mock_cas_commit.sh")
    
    if not os.path.isfile(transform) or not os.path.isfile(mock_cas):
        record(name, "FAIL", "transform or mock CAS script missing")
        return
        
    try:
        mod = _load_structural_hash_module(repo_root)
    except Exception as e:
        record(name, "FAIL", f"could not load structural_hash.py: {e}")
        return
        
    with tempfile.TemporaryDirectory() as tmp:
        env_overrides = {
            "CAS_COMMIT_SCRIPT": mock_cas,
            "MANIFEST_TRANSFORM_SCRIPT": transform,
            "MOCK_CAS_STATE_DIR": os.path.join(tmp, "state"),
        }
        old_env = {k: os.environ.get(k) for k in env_overrides}
        os.environ.update(env_overrides)
        
        first_ok = False
        second_rejected = False
        fake_hash = "deadbeef" * 8
        
        try:
            try:
                mod.register_hash(fake_hash)
                first_ok = True
            except Exception:
                pass
            try:
                mod.register_hash(fake_hash)
            except mod.DuplicateError:
                second_rejected = True
            except Exception:
                pass
        finally:
            for k, v in old_env.items():
                if v is None:
                    os.environ.pop(k, None)
                else:
                    os.environ[k] = v
                    
        if first_ok and second_rejected:
            record(name, "PASS")
        else:
            record(name, "FAIL", f"first_registration_ok={first_ok}, duplicate_correctly_rejected={second_rejected}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", default=None)
    args = parser.parse_args()
    repo_root = args.repo_root or find_repo_root(os.path.dirname(os.path.abspath(__file__)))
    
    print(f"[*] SWAYAM-AGI pipeline verification repo root: {repo_root}\n")
    
    check_safe_extract(repo_root)
    check_workflow_permissions(repo_root)
    check_glob_fail_closed(repo_root)
    check_canonicalize_rename_invariance(repo_root)
    check_duplicate_registration_contract(repo_root)
    
    passed = sum(1 for _, s, _ in RESULTS if s == "PASS")
    failed = sum(1 for _, s, _ in RESULTS if s == "FAIL")
    skipped = sum(1 for _, s, _ in RESULTS if s == "SKIP")
    total = len(RESULTS)
    
    print(f"\n{passed}/{total} passed, {failed} failed, {skipped} skipped.")
    if failed:
        sys.exit(1)

if __name__ == "__main__":
    main()
