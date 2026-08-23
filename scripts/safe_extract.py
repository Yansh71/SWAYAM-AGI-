#!/usr/bin/env python3
import zipfile
import os
import sys
import fnmatch

def is_safe_path(base_dir, target_path):
    # Resolves absolute path and ensures it does not break out of base_dir
    base_dir = os.path.abspath(base_dir)
    target_path = os.path.abspath(target_path)
    return target_path.startswith(base_dir + os.sep)

def safe_extract(zip_path, extract_to, allowed_patterns):
    extract_to = os.path.abspath(extract_to)
    
    # Resource Constraints for Zip-Bomb Defense
    MAX_MEMBER_BYTES = 10 * 1024 * 1024   # 10MB per file max
    MAX_TOTAL_BYTES = 50 * 1024 * 1024    # 50MB total uncompressed max
    MAX_MEMBERS = 200                     # Max 200 files
    
    with zipfile.ZipFile(zip_path, 'r') as zf:
        total_bytes = 0
        
        # 1. Pre-scan: Enforce resource constraints BEFORE extraction
        for i, member_info in enumerate(zf.infolist()):
            if i >= MAX_MEMBERS:
                raise ValueError(f"[FATAL] Archive exceeds member count limit ({MAX_MEMBERS})")
            if member_info.file_size > MAX_MEMBER_BYTES:
                raise ValueError(f"[FATAL] Member exceeds size limit: {member_info.filename}")
            
            total_bytes += member_info.file_size
            if total_bytes > MAX_TOTAL_BYTES:
                raise ValueError(f"[FATAL] Archive exceeds total size limit ({MAX_TOTAL_BYTES} bytes)")

        # 2. Path Validation & Extraction Loop
        for member_info in zf.infolist():
            filename = member_info.filename
            
            # Guard: Absolute paths
            if filename.startswith('/') or filename.startswith('\\'):
                raise ValueError(f"[FATAL] Zip-Slip attempt: Absolute path detected -> {filename}")
            
            # Guard: Path Traversal (../)
            target_path = os.path.join(extract_to, filename)
            if not is_safe_path(extract_to, target_path):
                raise ValueError(f"[FATAL] Zip-Slip attempt: Directory traversal detected -> {filename}")
                
            # Guard: Pattern Allowlist
            matched = False
            for pattern in allowed_patterns:
                if fnmatch.fnmatch(filename, pattern):
                    matched = True
                    break
            
            if not matched:
                print(f"[WARN] Skipping unauthorized file: {filename}")
                continue
                
            # Extract verified member
            zf.extract(member_info, extract_to)
            
    print(f"[VENOMICA] Secure extraction complete. Artifacts safely staged in {extract_to}")

if __name__ == "__main__":
    if len(sys.argv) < 4 or sys.argv[3] != "--allow":
        print("Usage: python3 safe_extract.py <zip_path> <dest_dir> --allow <patterns...>")
        sys.exit(1)
        
    zip_target = sys.argv[1]
    dest_directory = sys.argv[2]
    patterns = sys.argv[4:]
    
    safe_extract(zip_target, dest_directory, patterns)
