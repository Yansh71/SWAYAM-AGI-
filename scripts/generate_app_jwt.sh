#!/usr/bin/env bash
# generate_app_jwt.sh signs an App-level JWT (RS256) from the App's
# private key. Needed for App-level API calls like revoking an
# installation an installation token can't do this, only a JWT can.
#
# Usage: generate_app_jwt.sh <app_id> <path_to_private_key.pem>

set -euo pipefail

APP_ID="$1"
PEM_FILE="$2"

now=$(date +%s)
iat=$((now - 60))
exp=$((now + 540)) # GitHub caps App JWTs at 10 minutes

b64() { openssl base64 -A | tr '+/' '-_' | tr -d '='; }

header='{"alg":"RS256","typ":"JWT"}'
payload=$(jq -n --argjson iat "$iat" --argjson exp "$exp" --arg iss "$APP_ID" \
  '{iat: $iat, exp: $exp, iss: $iss}')

signing_input="$(printf '%s' "$header" | b64).$(printf '%s' "$payload" | b64)"
signature=$(printf '%s' "$signing_input" | openssl dgst -sha256 -sign "$PEM_FILE" -binary | b64)

echo "${signing_input}.${signature}"
