#!/usr/bin/env bash
# rate_limit_transform.sh called by cas_commit.sh with old content on stdin.
# Exits non-zero (rate limit hit) without printing new content, which
# cas_commit.sh treats as a hard rejection, not a race to retry.

set -euo pipefail

old="$(cat)"
now=$(date +%s)
max="${MAX_MUTATIONS_PER_HOUR:-5}"

if [[ -z "$old" ]]; then
  count=0
  window_start=$now
else
  count=$(jq -r .count <<<"$old")
  window_start=$(jq -r .window_start <<<"$old")

  if ((now - window_start > 3600)); then
    count=0
    window_start=$now
  fi
fi

if ((count >= max)); then
  echo "RATE LIMIT EXCEEDED: ${count}/${max} in window starting ${window_start}" >&2
  exit 42
fi

jq -n --argjson c "$((count + 1))" --argjson w "$window_start" '{count: $c, window_start: $w}'
