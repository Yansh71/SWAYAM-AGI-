#!/bin/bash
set -euo pipefail

STATE_FILE="rate-limit/SWAYAM-PRIME-01.json"
BRANCH="main"

echo "[VENOMICA] Checking CAS State on remote..."

STATUS_CODE=$(curl -s -o current_state.json -w "%{http_code}" \
  -H "Authorization: token $GH_TOKEN" \
  -H "Accept: application/vnd.github.v3+json" \
  "https://api.github.com/repos/$GITHUB_REPOSITORY/contents/$STATE_FILE")

if [ "$STATUS_CODE" -eq 404 ]; then
  echo "[VENOMICA] State file missing (404). Initializing Genesis State..."
  NEW_CONTENT=$(echo -n '{"count": 0, "last_reset": "'$(date -u +"%Y-%m-%d")'"}' | base64 -w 0)
  
  # FIX: Checking GENESIS_STATUS directly to prevent race conditions during 404
  GENESIS_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X PUT \
    -H "Authorization: token $GH_TOKEN" \
    -H "Accept: application/vnd.github.v3+json" \
    -d "{\"message\": \"Initialize Genesis Rate-Limit State\", \"content\": \"$NEW_CONTENT\", \"branch\": \"$BRANCH\"}" \
    "https://api.github.com/repos/$GITHUB_REPOSITORY/contents/$STATE_FILE")
    
  if [ "$GENESIS_STATUS" -eq 201 ] || [ "$GENESIS_STATUS" -eq 200 ]; then
    echo "[VENOMICA] Genesis State initialized successfully. Proceeding..."
  else
    echo "[VENOMICA FATAL] Genesis race lost (HTTP $GENESIS_STATUS). Another run initialized the state first. Re-run to pick up the increment path."
    exit 1
  fi
else
  echo "[VENOMICA] State file exists. Proceeding with atomic increment..."
  
  FILE_SHA=$(jq -r '.sha' current_state.json)
  CURRENT_CONTENT_BASE64=$(jq -r '.content' current_state.json)
  
  CURRENT_COUNT=$(echo "$CURRENT_CONTENT_BASE64" | base64 --decode | jq -r '.count')
  NEW_COUNT=$((CURRENT_COUNT + 1))
  
  echo "[VENOMICA] Current count is $CURRENT_COUNT. Incrementing to $NEW_COUNT..."
  NEW_CONTENT_PAYLOAD=$(echo -n "{\"count\": $NEW_COUNT, \"last_reset\": \"$(date -u +"%Y-%m-%d")\"}" | base64 -w 0)
  
  UPDATE_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X PUT \
    -H "Authorization: token $GH_TOKEN" \
    -H "Accept: application/vnd.github.v3+json" \
    -d "{\"message\": \"Atomic CAS Increment to $NEW_COUNT\", \"content\": \"$NEW_CONTENT_PAYLOAD\", \"sha\": \"$FILE_SHA\", \"branch\": \"$BRANCH\"}" \
    "https://api.github.com/repos/$GITHUB_REPOSITORY/contents/$STATE_FILE")
    
  if [ "$UPDATE_STATUS" -eq 200 ] || [ "$UPDATE_STATUS" -eq 201 ]; then
    echo "[VENOMICA] CAS Increment Successful."
  else
    echo "[VENOMICA FATAL] CAS Collision or API Error (HTTP $UPDATE_STATUS). Aborting to maintain integrity."
    exit 1
  fi
fi
