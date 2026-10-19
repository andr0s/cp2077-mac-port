#!/bin/bash
# re.sh <cpre args...>  — runs cpre on the installed macOS game binary (read-only)
B="$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077"
exec "$(dirname "$0")/cpre" "$B" "$@"
