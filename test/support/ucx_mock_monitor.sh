#!/usr/bin/env bash
# Monitor and auto-restart ucx_mock on exit
# Usage: ucx_mock_monitor.sh <path_to_ucx_mock> <pty_path>

set -euo pipefail

UCX_BIN="$1"
PTY_PATH="$2"

UCX_PID=""
RESTART_COUNT=0
MAX_RESTARTS=10

start_ucx() {
    "$UCX_BIN" --pty "$PTY_PATH" 2>&1 &
    UCX_PID=$!
    echo "[ucx_mock_monitor] Started ucx_mock (PID $UCX_PID)" >&2
}

# Start initial instance
start_ucx

# Monitor and restart on exit
while true; do
    if ! kill -0 "$UCX_PID" 2>/dev/null; then
        RESTART_COUNT=$((RESTART_COUNT + 1))
        if (( RESTART_COUNT > MAX_RESTARTS )); then
            echo "[ucx_mock_monitor] Too many restarts ($RESTART_COUNT), giving up"
            exit 1
        fi
        echo "[ucx_mock_monitor] ucx_mock exited (restart #$RESTART_COUNT), restarting..."
        start_ucx
        sleep 1
    fi
    sleep 1
done
