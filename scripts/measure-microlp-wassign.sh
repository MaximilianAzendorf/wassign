#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

INPUT_FILE="${1:-wassign-core/tests/inputs/realistic120.wassign}"
TIME_LIMIT="${2:-5s}"
JOB_COUNT="${JOB_COUNT:-1}"
SEED="${WASSIGN_SEED:-1}"
SUMMARY_FLAG="${MICROLP_BB_SUMMARY:-1}"
LOG_DIR="${MICROLP_MEASURE_LOG_DIR:-/tmp/microlp-measure}"

mkdir -p "$LOG_DIR"
STAMP="$(date +%Y%m%d-%H%M%S)"
INPUT_BASENAME="$(basename "$INPUT_FILE" .wassign)"
LOG_FILE="$LOG_DIR/${INPUT_BASENAME}-seed${SEED}-t${TIME_LIMIT}-${STAMP}.log"

echo "input=$INPUT_FILE"
echo "time_limit=$TIME_LIMIT"
echo "seed=$SEED"
echo "jobs=$JOB_COUNT"
echo "log=$LOG_FILE"

MICROLP_BB_SUMMARY="$SUMMARY_FLAG" \
WASSIGN_SEED="$SEED" \
cargo run -p wassign --release -- -i "$INPUT_FILE" -j "$JOB_COUNT" -t "$TIME_LIMIT" \
    >"$LOG_FILE" 2>&1

echo
echo "latest branch-and-bound summary:"
grep 'branch&bound summary' "$LOG_FILE" | tail -n 1 || echo "(no summary found)"

echo
echo "final output tail:"
tail -n 20 "$LOG_FILE"
