#!/bin/bash
# Test-compiles a set of redscript folders with the game's scc in a scratch copy (the game folder is only read).
# scc runs in a sandbox without window-server access and is stopped once its result is logged, so it never shows its
# error dialog. usage: _work/scc-test.sh <name> <folder>...   prints OK or the first error lines
set -euo pipefail
G="$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077"
T="$(cd "$(dirname "$0")" && pwd)/scc-runs/$1"
shift
rm -rf "$T" && mkdir -p "$T/r6/scripts" "$T/r6/cache"
cp "$G/r6/cache/final.redscripts.bk" "$T/r6/cache/"
i=0
for d in "$@"; do i=$((i + 1)); mkdir -p "$T/r6/scripts/s$i"; rsync -a --exclude r6 "$d/" "$T/r6/scripts/s$i/"; done
LOG="$T/r6/logs/redscript_rCURRENT.log"
(cd "$T" && exec sandbox-exec -p '(version 1)(allow default)(deny mach-lookup (global-name "com.apple.windowserver.active"))' \
    "$G/engine/tools/scc" -compile "$T/r6/scripts" > "$T/out.txt" 2>&1) &
pid=$!
last=-1; still=0
for _ in $(seq 1 900); do
    kill -0 "$pid" 2>/dev/null || break
    grep -q "Output successfully saved" "$LOG" 2>/dev/null && break
    size=$(stat -f %z "$LOG" 2>/dev/null || echo 0)
    if [[ "$size" == "$last" ]] && grep -q "\[ERROR" "$LOG" 2>/dev/null; then still=$((still + 1)); else still=0; fi
    last=$size
    (( still >= 6 )) && break   # errors logged and the log quiet for 6 s: scc now waits on its (invisible) dialog
    sleep 1
done
kill "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true
if grep -q "Output successfully saved" "$LOG"; then echo "OK"; else grep -m3 -E "ERROR" "$LOG" | cut -c1-300; fi
