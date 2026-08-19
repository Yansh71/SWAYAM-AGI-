#!/usr/bin/env bash
# cas_increment.sh crash-surviving, race-safe hourly rate limit for
# SWAYAM-PRIME-01. State lives on the protected 'state/rate-limit' branch,
# never in this process's memory, so a crash/restart can't reset the count.
#
# Requires GH_TOKEN in the environment pass the publisher App's
# installation token here, never a credential the agent sandbox can see.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="Yansh71/SWAYAM-AGI-"
NODE_ID="SWAYAM-PRIME-01"

export MAX_MUTATIONS_PER_HOUR="${MAX_MUTATIONS_PER_HOUR:-5}"

"$SCRIPT_DIR/cas_commit.sh" \
  "$REPO" \
  "rate-limit/${NODE_ID}.json" \
  "state/rate-limit" \
  "$SCRIPT_DIR/transforms/rate_limit_transform.sh"
