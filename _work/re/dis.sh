#!/bin/bash
# dis.sh <start> [count=64]  — disassemble count instructions from start (read-only)
B="$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077"
s=$(( $1 )); n=${2:-64}; e=$(( s + n*4 ))
/opt/homebrew/opt/llvm@21/bin/llvm-objdump -d --no-show-raw-insn --print-imm-hex --start-address=$s --stop-address=$e "$B" | tail -n +7
