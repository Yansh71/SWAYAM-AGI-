#!/bin/bash
set -euo pipefail

STATE_FILE="rate-limit/SWAYAM-PRIME-01.json"
BRANCH="main"

echo "[VENOMICA] Checking CAS State on remote..."

# Fetch current state, if 404 (Not Found), initialize it dynamically
STATUS_CODE=$(curl -s -o current_state.json -w "%{http_code}" \
  -H "Authorization: token $GH_TOKEN" \
  -H "Accept: application/vnd.github.v3+json" \
  "https://api.github.com/repos/$GITHUB_REPOSITORY/contents/$STATE_FILE")

if [ "$STATUS_CODE" -eq 404 ]; then
  echo "[VENOMICA] State file missing (404). Initializing Genesis State..."
  NEW_CONTENT=$(echo -n '{"count": 0, "last_reset": "'$(date -u +"%Y-%m-%d")'"}' | base64 -w 0)
  
  curl -s -X PUT \
    -H "Authorization: token $GH_TOKEN" \
    -H "Accept: application/vnd.github.v3+json" \
    -d "{\"message\": \"Initialize Genesis Rate-Limit State\", \"content\": \"$NEW_CONTENT\", \"branch\": \"$BRANCH\"}" \
    "https://api.github.com/repos/$GITHUB_REPOSITORY/contents/$STATE_FILE" > /dev/null
    
  echo "[VENOMICA] Genesis State initialized successfully. Proceeding..."
else
  echo "[VENOMICA] State file exists. Proceeding with atomic increment..."
  
  # Existing CAS increment logic should follow here
  # (Extract SHA, increment count, and PUT the updated file back)
fi
