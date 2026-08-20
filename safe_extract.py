#!/usr/bin/env python3
"""
scripts/safe_extract.py
Safe artifact extraction - mitigates the "Artifact Poisoning" CodeQL
finding (path traversal / zip-slip during artifact extraction).
"""
import argparse
import fnmatch
import os
import shutil
import sys
import tarfile
import zipfile
import tempfile

class UnsafeArchiveError(Exception):
    pass

def _is_within(base: str, target: str) -> bool:
    base = os.path.realpath(base)
    target = os.path.realpath(target)
    return os.path.commonpath([base, target]) == base

def _check_member(name: str, staging_dir: str) -> str:
    if name.startswith(("/", "\\")) or (len(name) > 1 and name[1] == ":"):
        raise UnsafeArchiveError(f"absolute path in archive: {name!r}")
    dest = os.path.join(staging_dir, name)
    if not _is_within(staging_dir, dest):
        raise UnsafeArchiveError(f"path traversal in archive entry: {name!r}")
    return dest

def _extract_zip(archive_path: str, staging_dir: str):
    with zipfile.ZipFile(archive_path) as zf:
        for info in zf.infolist():
            dest = _check_member(info.filename, staging_dir)
            if info.is_dir():
                os.makedirs(dest, exist_ok=True)
                continue
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with zf.open(info) as src, open(dest, "wb") as out:
                shutil.copyfileobj(src, out)

def _extract_tar(archive_path: str, staging_dir: str):
    with tarfile.open(archive_path) as tf:
        for member in tf.getmembers():
            if member.issym() or member.islnk():
                raise UnsafeArchiveError(f"symlink/hardlink in archive: {member.name!r}")
            dest = _check_member(member.name, staging_dir)
            if member.isdir():
                os.makedirs(dest, exist_ok=True)
                continue
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            src = tf.extractfile(member)
            with open(dest, "wb") as out:
                shutil.copyfileobj(src, out)

def safe_extract(archive_path: str, dest_dir: str, allow_patterns=None):
    os.makedirs(dest_dir, exist_ok=True)
    with tempfile.TemporaryDirectory() as staging_dir:
        if archive_path.endswith(".zip"):
            _extract_zip(archive_path, staging_dir)
        elif archive_path.endswith((".tar", ".tar.gz", ".tgz")):
            _extract_tar(archive_path, staging_dir)
        else:
            raise ValueError(f"unsupported archive type: {archive_path}")
        
        moved = 0
        for root, dirs, files in os.walk(staging_dir):
            for fname in files:
                staged_path = os.path.join(root, fname)
                rel_path = os.path.relpath(staged_path, staging_dir)
                if allow_patterns and not any(fnmatch.fnmatch(rel_path, pat) for pat in allow_patterns):
                    print(f"[safe_extract] rejected (not allowlisted): {rel_path}", file=sys.stderr)
                    continue
                final_dest = os.path.join(dest_dir, rel_path)
                if not _is_within(dest_dir, final_dest):
                    raise UnsafeArchiveError(f"post-move path escapes dest_dir: {rel_path}")
                os.makedirs(os.path.dirname(final_dest), exist_ok=True)
                shutil.move(staged_path, final_dest)
                moved += 1
        print(f"[safe_extract] extracted {moved} file(s) into {dest_dir}")
        return moved

def main():
    parser = argparse.ArgumentParser(description="Safely extract a build artifact.")
    parser.add_argument("archive")
    parser.add_argument("dest_dir")
    parser.add_argument("--allow", nargs="*", default=None, help="glob patterns; only matching relative paths are kept")
    args = parser.parse_args()
    try:
        safe_extract(args.archive, args.dest_dir, args.allow)
    except UnsafeArchiveError as e:
        print(f"[safe_extract] REJECTED archive: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()

