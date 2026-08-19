#!/usr/bin/env bash
# manifest_transform.sh called by cas_commit.sh with old manifest.json content
# on stdin and the new hash as $1. Rejects (non-zero exit) if the hash is
# already registered this rejection IS the duplicate check.

set -euo pipefail

new_hash="$1"
old="$(cat)"

if [[ -z "$old" ]]; then
  hashes="[]"
else
  hashes=$(jq -c '.accepted_hashes // []' <<<"$old")
fi

if jq -e --arg h "$new_hash" 'index($h) != null' <<<"$hashes" >/dev/null; then
  echo "REJECTED: structural duplicate of hash ${new_hash:0:12}" >&2
  exit 1
fi

jq -c --arg h "$new_hash" '{accepted_hashes: (. + [$h])}' <<<"$hashes"
