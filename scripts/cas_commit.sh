#!/usr/bin/env bash
# cas_commit.sh generic compare-and-swap commit against the GitHub Contents API.
#
# Usage: cas_commit.sh <repo> <path> <branch> <transform_script> [transform_args...]
#
# <transform_script> is an executable that reads the current file's decoded
# content on stdin (empty string if the file doesn't exist yet) and prints
# the new content on stdout. If the transform rejects the update (e.g. rate
# limit exceeded, duplicate hash found), it exits non-zero with a reason on
# stderr this script does NOT retry that case, it just propagates the
# rejection. Retries only happen on a genuine 409/SHA-mismatch race between
# two nodes writing concurrently.
#
# Used by both cas_increment.sh (rate limiting) and structural_hash.py
# (duplicate-detection manifest) so both get the same crash-survival and
# race-safety guarantees from one reviewed implementation.

set -euo pipefail

REPO="$1"; FILE_PATH="$2"; BRANCH="$3"; TRANSFORM="$4"; shift 4 || true
: "${GH_TOKEN: ?GH_TOKEN must be set}"

TMP_GET="$(mktemp)"
TMP_PUT="$(mktemp)"
trap 'rm -f "$TMP_GET" "$TMP_PUT"' EXIT

for attempt in 1 2 3 4 5 6; do
  http_code=$(curl -s -o "$TMP_GET" -w "%{http_code}" \
    -H "Authorization: Bearer $GH_TOKEN" \
    "https://api.github.com/repos/$REPO/contents/$FILE_PATH?ref=$BRANCH")

  case "$http_code" in
    200) sha=$(jq -r .sha "$TMP_GET")
         old_content=$(jq -r .content "$TMP_GET" | base64 -d);;
    404) sha=""; old_content="";;
    *)
      echo "Unexpected GET status $http_code for $FILE_PATH" >&2
      cat "$TMP_GET" >&2; exit 1;;
  esac

  if ! new_content=$(printf '%s' "$old_content" | "$TRANSFORM" "$@"); then
    echo "Transform rejected the update - not retrying." >&2
    exit 1
  fi

  b64=$(printf '%s' "$new_content" | base64 -w0)

  if [[ -n "$sha" ]]; then
    body=$(jq -n --arg m "cas update $(date -u +%FT%TZ)" --arg c "$b64" --arg s "$sha" --arg b "$BRANCH" \
      '{message:$m, content:$c, sha:$s, branch: $b}')
  else
    body=$(jq -n --arg m "cas create $(date -u +%FT%TZ)" --arg c "$b64" --arg b "$BRANCH" \
      '{message: $m, content:$c, branch: $b}')
  fi

  status=$(curl -s -o "$TMP_PUT" -w "%{http_code}" \
    -X PUT -H "Authorization: Bearer $GH_TOKEN" \
    "https://api.github.com/repos/$REPO/contents/$FILE_PATH" -d "$body")

  if [[ "$status" == "200" || "$status" == "201" ]]; then
    echo "$new_content"
    exit 0
  fi

  echo "CAS attempt $attempt: PUT returned $status (likely a concurrent write) - retrying" >&2
  cat "$TMP_PUT" >&2
  sleep $(((RANDOM % 3) + attempt ))
done

echo "CAS commit failed after retries: $FILE_PATH" >&2
exit 1
