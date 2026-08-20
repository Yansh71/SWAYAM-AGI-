#!/usr/bin/env bash
# scripts/testing/mock_cas_commit.sh
#
# Local, no-network stand-in for cas_commit.sh - same CLI contract
# (REPO FILE_PATH BRANCH TRANSFORM [extra-args-for-transform]) but
# reads/writes a local file instead of the GitHub Contents API.

set -euo pipefail

REPO="$1"; FILE_PATH="$2"; BRANCH="$3"; TRANSFORM="$4"; shift 4 || true

STATE_DIR="${MOCK_CAS_STATE_DIR:-./.mock_cas_state}"

# Safe path generation avoiding Bash syntax quirks using 'tr'
SAFE_BRANCH=$(echo "$BRANCH" | tr '/' '_')
SAFE_FILE=$(echo "$FILE_PATH" | tr '/' '_')

STATE_FILE="$STATE_DIR/${SAFE_BRANCH}_${SAFE_FILE}"

# Bulletproof directory creation (ensures exact path exists)
mkdir -p "$(dirname "$STATE_FILE")"

old_content=""
[[ -f "$STATE_FILE" ]] && old_content="$(cat "$STATE_FILE")"

if ! new_content=$(printf '%s' "$old_content" | bash "$TRANSFORM" "$@"); then
  echo "Transform rejected the update - not retrying." >&2
  exit 1
fi

printf '%s' "$new_content" > "$STATE_FILE"
echo "$new_content"

