# Brief: locating Codeware addresses in the macOS Cyberpunk 2077 binary (static RE only)

You are helping port Codeware (Cyberpunk 2077 RED4ext plugin) to macOS. Your job is ONLY static reverse engineering of
the game binary to locate specific functions/globals and verify their arm64 signatures. You write findings to a file.

## Hard rules
- Never run the game, Steam, any build system, or any script/binary from any repo. Never write outside
  `/Users/mac/work/other/cp2077-mac-port/_work/codeware/`. Never modify anything under `/Users/mac/work/other/cp2077-mac-port/src`
  or `/Users/mac/work/other/cp2077-mac-port/ports`. Read-only use of the RE tools below is allowed.
- Do not guess. An address is only reported with concrete evidence (string refs, call graph, vtable slot, field access
  pattern that matches the known Windows semantics, argument usage). Say "not found" when unsure.

## Tools (read-only)
- `/Users/mac/work/other/cp2077-mac-port/_work/re/re.sh <command> [args] [\; command args ...]` — commands documented at the
  top of `/Users/mac/work/other/cp2077-mac-port/_work/re/cpre.cpp`: func, callers, callees, xref, xrefr, str, strx, strrefs,
  datarefs, ptrsto, ptrsat, gotname, sym, symat, imm. Chain with `\;`.
- `/Users/mac/work/other/cp2077-mac-port/_work/re/dis.sh <addr> [count]` — llvm-objdump disassembly.
- Bulk disassembly of a range (fast, then grep):
  `"/opt/homebrew/opt/llvm@21/bin/llvm-objdump" -d --no-show-raw-insn --print-imm-hex --start-address=0x.. --stop-address=0x.. "$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077" > <file in _work/codeware/>`
- Symbol dumps: `/Users/mac/work/other/cp2077-mac-port/_work/game_nm_demangled.txt`, `game_exports_addr_dem.txt`
  (mostly template/pool symbols; most engine functions are unnamed).
- Existing evidence docs (read them for technique and anchors): `/Users/mac/work/other/cp2077-mac-port/src/jackmaxwil_RED4ext.SDK-macos/docs/re/*.md`
  and `.../docs/ADDRESS_AUDIT.md`. Address DB: `.../cyberpunk2077_addresses.json` (only "verified": true entries are trusted).

## Binary facts
- arm64 Mach-O, image base 0x100000000, UUID A6656ADC-FBE2-36A4-9B9D-B4A9DE645089. __TEXT starts 0x100000000,
  __DATA_CONST vmaddr 0x106DE0000, __DATA vmaddr 0x107394000 (DB data offsets are segment-relative: "3:0x<addr-0x107394000>").
- Itanium C++ ABI: vtable address point = first virtual; slot 0/1 of a class whose destructor is its first virtual are
  D1/D0. Every virtual after the destructor is +8 vs. the MSVC (Windows) offset. Overloaded virtuals keep declaration order.
- A function returning a non-trivial struct by value (Handle<T>, SharedPtr<T>, JobHandle, DynArray, CString, ...) takes
  the hidden result pointer in **x8**, not as an argument: on Windows the out-pointer is the 1st arg (2nd for member functions);
  on arm64 every later argument moves one register earlier. Trivially copyable <=16-byte results come back in x0/x1.
- Common anchors: CRTTISystem::Get thunk 0x1021885A0; CNamePool::Add(const char*) thunk 0x102188524; CClass ctor
  0x102196F40 (cls, CName, size, flags); class registration pattern: name string -> CClass ctor -> TTypedClass vtable stored
  at [cls] -> vtable slot +0xE0 = ConstructCls -> stores the C++ vtable of the class (see docs/re/appearance.md).
  CClassFunction ctor thunk 0x102173860(fn obj, cls, fullName, shortName, handler x4, adj x5=0, flags w6);
  CClassStaticFunction ctor thunk 0x10217395C(..., handler x4, flags w5); CGlobalFunction ctor thunk 0x102173AD4(obj,
  name, name, handler x3). Script native handler signature: (IScriptable* ctx x0, CStackFrame* frame x1, void* ret x2,
  IType* x3); `frame->code` is [x1]; ParamEnd skip = `++[x1]`.
  Job label strings ("Something/Other") are referenced right where job lambdas are created; IsA = 0x10219AC60(cls, type).
  Entity C++ vtable 0x106ED2D20; entity status byte at +0x156; Entity::Initialize 0x100C985EC, Attach 0x100C99368,
  Detach 0x100C996C0, Uninitialize 0x100C992C4, Dispose 0x100C999E0, Reassemble 0x100C95744 (already done).
  GameInstance vtable 2:0x1D1FA0 (GetSystem slot +0x10); CGameEngine global 3:0x1D483A0 (framework at +0x338).
  InkSystem singleton pointer global 0x108D5D830 (getter 0x10488B8DC).

## What "found" means (write all of it)
For each requested item: absolute address (must be a function start: `re.sh func <addr>` shows it as the start), the
DB form `1:0x<addr-0x100000000>` (or `3:0x...` for __DATA globals), the evidence chain, the **arm64 signature** you
derived from register usage vs. the Windows declaration given to you (say MATCH or MISMATCH and how), whether it is a tiny
function (< 16 bytes) or starts with a PC-relative instruction in its first 4 instructions (hook risk), inlining notes, and a
confidence: high (identity proven by strings/vtable/call graph + signature checked), medium (strong structural match),
low (plausible only). Also report any layout offsets you were asked to check, with the instruction addresses that prove them.

Write results to the output file named in your task as Markdown, one section per item. Keep it factual and compact.
