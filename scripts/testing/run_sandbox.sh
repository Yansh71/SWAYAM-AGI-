#!/bin/bash
# scripts/testing/run_sandbox.sh
# Compiles each mutation in src/generated/, then runs it through the
# crash-safe supervisor (core.hpp) instead of executing it directly.

set -uo pipefail

GEN_DIR="src/generated"
INCLUDE_DIR="include"
WORK="/tmp/swayam_sandbox"
mkdir -p "$WORK"
export SWAYAM_QUARANTINE_LOG="$WORK/quarantine.log"

OVERALL_STATUS=0

if [ ! -d "$GEN_DIR" ] || [ -z "$(ls -A "$GEN_DIR"/*.h 2>/dev/null)" ]; then
    echo "[VENOMICA] No mutations found in $GEN_DIR - nothing to validate."
    exit 0
fi

echo "[VENOMICA] Building supervisor..."
if ! g++ -O2 -std=c++23 -I"$INCLUDE_DIR" scripts/testing/supervisor_main.cpp -o "$WORK/supervisor" 2>"$WORK/supervisor_build.log"; then
    echo "[VENOMICA FATAL] Supervisor itself failed to build:"
    cat "$WORK/supervisor_build.log"
    exit 1
fi

for mutation_header in "$GEN_DIR"/*.h; do
    name=$(basename "$mutation_header" .h)
    echo "[VENOMICA] Validating mutation: $name"
    
    driver="$WORK/${name}_driver.cpp"
    cat > "$driver" <<EOF
#include "${mutation_header}"
int main() { return 0; }
EOF

    if ! g++ -O1 -std=c++23 -I"$INCLUDE_DIR" -I"src" "$driver" -o "$WORK/${name}_bin" 2>"$WORK/${name}_compile.log"; then
        echo "[VENOMICA FATAL] $name failed to compile:"
        cat "$WORK/${name}_compile.log"
        OVERALL_STATUS=1
        continue
    fi

    if ! "$WORK/supervisor" "$WORK/${name}_bin" "$mutation_header"; then
        echo "[VENOMICA FATAL] $name crashed, was already quarantined, or exited non-zero."
        OVERALL_STATUS=1
    else
        echo "[VENOMICA] $name passed."
    fi
done

exit $OVERALL_STATUS
