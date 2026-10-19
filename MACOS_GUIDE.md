# Cyberpunk 2077 mods on macOS (Apple silicon)

**RED4ext · Cyber Engine Tweaks · TweakXL · ArchiveXL · Codeware · AudioXL · redscript**, and the
**Titanfall BT-7274 (Quest and Companion)** mod that needs all of them.

Maintained by **andr0s** (<https://github.com/andr0s>). This guide covers everything needed to run the main
Cyberpunk 2077 modding frameworks natively on a Mac: what was ported, how to build and install it, how to install mods,
what was fixed along the way, and what is still missing. It is written for players, for mod authors (the BT-7274 author
in particular) and for anyone who wants to continue the work.

> **Status:** tested end to end on Cyberpunk 2077 **2.3.1** (Steam), macOS 26.5, Apple silicon. The BT-7274 quest was
> played with all frameworks loaded: BT is called and spawned, the link shard is taken, the fights, the saves and BT's
> keys work. Other mods should work as long as they only need these frameworks. See
> [Known limitations](#13-known-limitations) before you start.

---

## Contents

1. [What you get](#1-what-you-get)
2. [Before you start](#2-before-you-start)
3. [Quick start](#3-quick-start)
4. [Step by step](#4-step-by-step)
5. [Installing Titanfall BT-7274](#5-installing-titanfall-bt-7274)
6. [Playing, logs and saves](#6-playing-logs-and-saves)
7. [Troubleshooting](#7-troubleshooting)
8. [What was changed, per component](#8-what-was-changed-per-component)
9. [macOS pitfalls for plugin and mod developers](#9-macos-pitfalls-for-plugin-and-mod-developers)
10. [Notes for the BT-7274 author and other mod authors](#10-notes-for-the-bt-7274-author-and-other-mod-authors)
11. [Security review](#11-security-review)
12. [Uninstalling](#12-uninstalling)
13. [Known limitations](#13-known-limitations)
14. [Credits and licenses](#14-credits-and-licenses)
- [Appendix A: file locations](#appendix-a-file-locations)
- [Appendix B: repositories, branches and commits](#appendix-b-repositories-branches-and-commits)
- [Appendix C: upstream pull requests](#appendix-c-upstream-pull-requests)

---

## 1. What you get

| Component | Version | What it is | macOS source used |
| --- | --- | --- | --- |
| **RED4ext** | 1.29.1 | Native plugin loader | jackmaxwil's macOS port + fixes from this project |
| **RED4ext.SDK** | macOS fork | Headers and the address database every plugin builds against | jackmaxwil's macOS fork + fixes from this project |
| **redscript** | 0.5.x | Script compiler | already shipped with the macOS game: `engine/tools/scc` (by CD PROJEKT RED) |
| **TweakXL** | 1.11.4 | TweakDB edits (`r6/tweaks`) | jackmaxwil's macOS port, unchanged |
| **ArchiveXL** | 1.27.4 | Archive and resource extensions | jackmaxwil's macOS port, unchanged |
| **Codeware** | 1.20.5 | Script API extensions (entity spawning, callbacks, reflection, UI) | ported here from psiberx/cp2077-codeware |
| **AudioXL** | 0.4.3 | Custom sounds for mods | ported here; plays through AVAudioEngine |
| **Cyber Engine Tweaks (CET)** | 1.37.1 | Lua mods, overlay, console | ported here (new macOS port) |

All of it is built from source by one script, `cp2077-mac.sh`. The script also installs everything into the game
folder, adds and removes mods safely, starts the game with mods, and uninstalls.

**Tested setup:**

- Cyberpunk 2077 2.3.1, Steam, macOS build with Mach-O UUID `A6656ADC-FBE2-36A4-9B9D-B4A9DE645089`
- macOS 26.5, Apple silicon
- Xcode Command Line Tools 26.6 (Apple clang 21), CMake 3.25

---

## 2. Before you start

- **Apple silicon only.** Everything is built for arm64.
- **Steam version 2.3.1 only.** Native plugins call game functions by address. The address database is tied to one
  game executable (checked by its Mach-O UUID). After a game update the launcher refuses to start with mods until the
  database is updated. Steam's own Play button always starts the game without mods.
- **Back up your saves first.** Saves live in `~/Library/Application Support/CD Projekt Red/Cyberpunk 2077/saves`:

  ```bash
  ditto ~/"Library/Application Support/CD Projekt Red/Cyberpunk 2077" ~/Desktop/Cyberpunk2077-saves-backup
  ```

- **The game executable gets re-signed.** RED4ext is injected with `DYLD_INSERT_LIBRARIES`, which needs three
  entitlements macOS does not give CD PROJEKT's signed executable: DYLD environment variables, unsigned libraries and
  unsigned executable memory. `install` backs up the original executable (`Cyberpunk2077.orig`) and re-signs the
  game ad hoc with those entitlements. `uninstall`, or Steam's *Verify integrity of game files*, puts the original
  back. The re-signed game also starts this way from Steam; it then simply loads no mods.
- **Your terminal needs "App Management" once.** Re-signing an app needs *System Settings → Privacy & Security → App
  Management* allowed for your terminal (Terminal, iTerm, …). You can switch it off again after `install`.
- **Native plugins run with your privileges.** Only install RED4ext plugins you trust. `add-mod` refuses native code
  in mod archives for that reason. Lua mods (CET) run in a sandbox, and redscript mods only see the game's script API.
- **Disk space:** about 3 GB for the sources and the build.

---

## 3. Quick start

```bash
xcode-select --install          # Command Line Tools (or update them: softwareupdate --list)
```

```bash
brew install cmake              # CMake 3.25 or newer
```

```bash
git clone https://github.com/andr0s/cp2077-mac-port.git && cd cp2077-mac-port
```

Then clone the source repositories into `ports/` and `src/` exactly as listed in [4.2](#42-get-the-sources)
(`cp2077-mac.sh` expects those folder names), and continue:

```bash
./cp2077-mac.sh doctor          # checks the Mac, the tools and the game build; changes nothing
```

```bash
./cp2077-mac.sh build           # about 10 to 20 minutes the first time
```

```bash
./cp2077-mac.sh install         # copies into the game, backs up and re-signs the executable
```

```bash
./cp2077-mac.sh add-mod ~/Downloads/BT7274      # an unpacked mod folder (see section 5)
```

```bash
./cp2077-mac.sh play            # Steam must be running and signed in
```

---

## 4. Step by step

### 4.1 Tools

| Tool | Version | How |
| --- | --- | --- |
| Command Line Tools | Apple clang 16 or newer, macOS SDK 14 or newer | `xcode-select --install`, or `softwareupdate --list` and install "Command Line Tools for Xcode" |
| CMake | 3.25 or newer | `brew install cmake` |
| git, Python 3, Perl | system | come with the Command Line Tools / macOS |

Old Command Line Tools installed over an older copy can leave stale libc++ headers behind (no `<ranges>`, no
`<concepts>`). The build detects that and uses the SDK's headers instead.

### 4.2 Get the sources

`cp2077-mac.sh` expects this layout next to it:

| Folder | Repository | Branch |
| --- | --- | --- |
| `ports/RED4ext-macos` | `andr0s/RED4ext-macos` (fork of `jackmaxwil/RED4ext-macos`) | `macos-fixes` |
| `ports/CyberEngineTweaks` | `andr0s/CyberEngineTweaks` (fork of `maximegmd/CyberEngineTweaks`) | `macos` |
| `ports/CyberEngineTweaks/vendor/RED4ext.SDK-macos` | `andr0s/RED4ext.SDK-macos` (fork of `jackmaxwil/RED4ext.SDK-macos`), submodule | `macos-fixes` |
| `ports/Codeware` | `andr0s/cp2077-codeware` (fork of `psiberx/cp2077-codeware`) | `macos` |
| `ports/AudioXL` | `andr0s/cp2077-audio-xl` (port of `DigitalVixenSWE/cp2077-audio-xl`, which is no longer on GitHub) | `macos` |
| `src/jackmaxwil_cp2077-tweak-xl-macos` | `jackmaxwil/cp2077-tweak-xl-macos` | commit `4ba4565` |
| `src/jackmaxwil_cp2077-archive-xl-macos` | `jackmaxwil/cp2077-archive-xl-macos` | commit `033f7ec` |

```bash
git clone -b macos-fixes https://github.com/andr0s/RED4ext-macos.git ports/RED4ext-macos
```

```bash
git clone -b macos https://github.com/andr0s/CyberEngineTweaks.git ports/CyberEngineTweaks
```

```bash
git clone -b macos https://github.com/andr0s/cp2077-codeware.git ports/Codeware
```

```bash
git clone -b macos --recurse-submodules https://github.com/andr0s/cp2077-audio-xl.git ports/AudioXL
```

```bash
git clone https://github.com/jackmaxwil/cp2077-tweak-xl-macos.git src/jackmaxwil_cp2077-tweak-xl-macos && git -C src/jackmaxwil_cp2077-tweak-xl-macos checkout 4ba4565
```

```bash
git clone https://github.com/jackmaxwil/cp2077-archive-xl-macos.git src/jackmaxwil_cp2077-archive-xl-macos && git -C src/jackmaxwil_cp2077-archive-xl-macos checkout 033f7ec
```

The other submodules (RED4ext's `deps/*`, CET's `vendor/*`, Codeware's `vendor/*`) are fetched by `build`, pinned to the
commits recorded in each repository.

### 4.3 Check the Mac: `doctor`

```bash
./cp2077-mac.sh doctor
```

It checks: Apple silicon, the game executable's UUID against the address database, whether the executable is CD PROJEKT's
original or already set up for RED4ext, Apple clang ≥ 16, macOS SDK ≥ 14, CMake ≥ 3.25, Steam running, and the source
folders. It changes nothing. Set `CP2077_GAME_DIR` if the game is not in the default Steam library
(`~/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077`).

### 4.4 Build: `build`

```bash
./cp2077-mac.sh build
```

- Builds RED4ext, TweakXL, ArchiveXL, Codeware, AudioXL and CET into `build/` with Apple's clang (even if another clang
  comes first in `PATH`).
- Runs CET's unit tests (path jail, SQLite authorizer, UTF-8 conversion).
- Downloads ArchiveXL's packed resources (`ArchiveXL.archive`, built with WolvenKit on Windows; the format is the same
  on macOS) and checks the zip and the archive by SHA-256.
- Checks that every built library links only system libraries.
- Runs RED4ext's **address gate** (`red4ext_plugin_check`): a plugin that uses a game address the database does not mark
  *verified* would be refused by the loader, so the build fails instead.

Nothing outside `build/` is touched.

### 4.5 Install: `install`

```bash
./cp2077-mac.sh install
```

- Copies RED4ext, the plugins, their script folders and the address database into the game folder. The layout is the
  release layout of RED4ext-macos (see [Appendix A](#appendix-a-file-locations)).
- Records every file it copied (`install-state/installed-files.txt`, outside the game folder) so `uninstall` removes
  exactly those files.
- Signs the libraries ad hoc.
- Runs RED4ext's `install_macos.sh`: checks the game build, clears quarantine flags, backs up the executable once
  (`Cyberpunk2077.orig`) and re-signs it with the three entitlements. If the executable is already set up, nothing is
  re-signed (no permission needed).

A second `install` replaces the previous one cleanly. Do not run it while the game is running: replacing a library
under a running process can crash it.

### 4.6 Play: `play`

```bash
./cp2077-mac.sh play
```

Steam must be running and signed in, or saves and achievements are not available. The launcher (`launch_red4ext.sh`):

1. Refuses to start if the game executable changed (game update) or lost its entitlements (Steam replaced it).
2. Stages the `Scripts` folders of the plugins RED4ext will actually load and compiles all redscript with the game's
   own `engine/tools/scc`. A plugin's scripts declare its natives, and the game stops at "Failed to initialize scripts
   data!" if a declared native's plugin is not loaded, so plugins that fail the address gate are left out.
3. Merges mod key bindings (`r6/input/*.xml`).
4. Starts the game with `DYLD_INSERT_LIBRARIES=red4ext/RED4ext.dylib`.
5. When the game exits, removes the staged plugin scripts and restores the game's own script bundle, so Steam's Play
   button keeps working.

Keep the Terminal window open while you play.

### 4.7 Mods: `add-mod` and `remove-mod`

```bash
./cp2077-mac.sh add-mod <unpacked mod folder>
```

```bash
./cp2077-mac.sh remove-mod <name>
```

Unpack the mod archive (the folder that contains `archive/`, `bin/`, `r6/`, … like the game folder). `add-mod` only
copies into the folders mods may change: `archive/pc/mod/`, `bin/x64/plugins/cyber_engine_tweaks/mods/`, `mods/`,
`r6/scripts/`, `r6/tweaks/`, `r6/input/`, `red4ext/plugins/`. It refuses, without copying anything:

- native code (`.dylib`, `.so`, `.dll`, `.exe`, `.asi`, bundles, apps),
- files outside those folders,
- files that would replace a file of the installed frameworks,
- paths whose folders lead out of the game folder through symlinks.

It records what it copied (`install-state/mods/<name>.txt`), and `remove-mod` deletes exactly those files.

Windows-only mods stay Windows-only: anything with a `.dll`, or that needs a framework not listed in
[section 1](#1-what-you-get), will not work.

---

## 5. Installing Titanfall BT-7274

Mod page: <https://www.nexusmods.com/cyberpunk2077/mods/35041>. It needs RED4ext, redscript, TweakXL, ArchiveXL,
Codeware, AudioXL and CET, all provided here. The mod itself contains only data, redscript and Lua (no native code), so
it installs with `add-mod`.

1. Download the main file from Nexus (version 1.0.0 was tested) and unpack it into a folder, e.g. `~/Downloads/BT7274`.
2. Install it:

   ```bash
   ./cp2077-mac.sh add-mod ~/Downloads/BT7274
   ```

3. Start the game with `./cp2077-mac.sh play` and load a save.

**Keys on MacBooks.** BT's default keys are on the numeric keypad (NumPad9 call, NumPad8 send home, NumPad0 cycle
orders), which MacBooks do not have. Mod Settings (where they are normally changed) is not available on macOS
([section 13](#13-known-limitations)). The defaults live in `r6/scripts/bt7274/BT7274Keys.reds`; before running
`add-mod`, change the three defaults in the unpacked mod, for example to `[`, `]` and `\`:

```diff
-  public let callKey: EInputKey = EInputKey.IK_NumPad9;
+  public let callKey: EInputKey = EInputKey.IK_LeftBracket;
-  public let homeKey: EInputKey = EInputKey.IK_NumPad8;
+  public let homeKey: EInputKey = EInputKey.IK_RightBracket;
-  public let followKey: EInputKey = EInputKey.IK_NumPad0;
+  public let followKey: EInputKey = EInputKey.IK_Backslash;
```

(The matching `@runtimeProperty("ModSettings.description", …)` texts mention the old keys; change them too if you
like.) Then run `add-mod` again; it overwrites the mod's files.

**What on macOS makes BT-7274 work** (details in [section 8](#8-what-was-changed-per-component)):

- `exEntitySpawner` (a CET API) is provided on top of Codeware's `DynamicEntitySystem`. BT, the shard case, the report
  and the guards' poses are spawned through it.
- `AudioXL.AudioXLLua`, the class the mod's Lua plays its 653 sounds through, was rebuilt (it is not in the public
  AudioXL sources).
- The global script functions the mod calls through CET (`TSQ_ALL;`, `TSQ_NPC;`, `CreateForceRagdollEvent;CName`) are
  found again (an SDK vtable fix).
- `TDBID.ToStringDEBUG` returns record names the mod compares (BT's own record, V's head item).
- Saving works (an SDK fix for `IGameSystem::IsSavingLocked`).

---

## 6. Playing, logs and saves

- **Start:** `./cp2077-mac.sh play` (Steam running). Steam's Play button = vanilla game.
- **CET overlay:** on the first start CET asks you to bind its overlay key. It is saved in
  `bin/x64/plugins/cyber_engine_tweaks/bindings.json`.
- **Saving during fights** is blocked by the game itself (as on Windows); autosaves happen again after the fight.
- **Logs** (relative to the game folder, see [Appendix A](#appendix-a-file-locations)):

  | What | Where |
  | --- | --- |
  | RED4ext (loader, plugin list, hook failures) | `red4ext/logs/red4ext-*.log`, hook statistics in `red4ext/logs/hookstats.json` |
  | TweakXL, ArchiveXL, Codeware | `red4ext/plugins/<name>/<name>-*.log` |
  | AudioXL | `red4ext/logs/audioxl-*.log` |
  | CET | `bin/x64/plugins/cyber_engine_tweaks/cyber_engine_tweaks.log`, Lua output in `scripting.log` |
  | CET Lua mods | `bin/x64/plugins/cyber_engine_tweaks/mods/<mod>/<mod>.log` |
  | redscript compiler | `r6/logs/redscript_rCURRENT.log` |
  | Crash reports (macOS) | `~/Library/Logs/DiagnosticReports/Cyberpunk2077-*.ips` |

---

## 7. Troubleshooting

| Symptom | Cause | Fix |
| --- | --- | --- |
| `REDscript compilation failed; not launching` | A script mod does not compile against this set | Read the errors the launcher prints (also in `r6/logs/redscript_rCURRENT.log`); remove or fix that mod |
| `The game was updated to a build this RED4ext release does not support yet` | Steam updated the game | The address database must be redone for the new build; play without mods meanwhile |
| `The game binary is not set up for RED4ext` | Steam replaced the executable (update or verify) | `./cp2077-mac.sh install` again |
| `Operation not permitted` while re-signing | The terminal lacks App Management | System Settings → Privacy & Security → App Management → allow your terminal, run `install` again |
| `start Steam and sign in first` | Steam is not running | Start Steam, sign in, run `play` again |
| CET overlay never appears | CET did not start | See `cyber_engine_tweaks.log` for "did not start: …" |
| `The hook requested by '<plugin>' … could not be attached` | A hook failed to attach | Fixed for late hooks (stub page pool); report it with `red4ext/logs` if it appears |
| Game crashes, no `.ips` report appears | macOS's crash reporter can get stuck ("too many open files" in the system log) | `launchctl kickstart -k gui/$(id -u)/com.apple.ReportCrash`, then reproduce |
| Saves do not happen during a mission | Combat or a scene locks saving | Expected; save after the fight |
| Mod keys do nothing on a MacBook | Default keys on a numpad | Change the mod's defaults ([section 5](#5-installing-titanfall-bt-7274)) |

When you report a problem, include the crash report (`.ips`), `red4ext/logs/red4ext-*.log` and the relevant plugin log.
**Remove personal data first:** crash reports and logs contain your macOS user name in paths, and some logs contain your
Steam ID.

---

## 8. What was changed, per component

### 8.1 RED4ext (loader)

Base: jackmaxwil's macOS port of RED4ext. It loads plugins through `DYLD_INSERT_LIBRARIES`, resolves game addresses
only from entries marked **verified** in the address database (anything else resolves to 0, fail-closed), refuses
plugins that contain unverified address hashes, and hooks with its own arm64 hook engine.

Changes here (branch `macos-fixes`):

- **Stub page pool for late hooks.** Plugins also attach hooks during the game (TweakXL's stat hooks run from a game job).
  By then the game has reserved most of the address space within ADRP reach of its code (about 130 GiB in all), the
  engine found no free page for the hook stub, the attach failed (`kErrUnreachable`, -6), and TweakXL aborted the game.
  512 pages within 3 GiB of the game's image are now reserved at the first hook transaction.
- **`red4ext_plugin_check`:** load-command strings are read only inside their command and the file (a malformed Mach-O
  read out of bounds).
- **Test hook only for the test driver:** `RED4ext_TestReport` let any script mod append unbounded text to a log.
- **Launcher exit:** restores the game's own script bundle instead of recompiling, which failed with an error dialog
  once a mod needed a plugin's scripts.
- CMake 3.25 accepted.

Upstream: [jackmaxwil/RED4ext-macos#10](https://github.com/jackmaxwil/RED4ext-macos/pull/10).

### 8.2 RED4ext.SDK (macOS fork)

Changes here (branch `macos-fixes`), both arm64 ABI issues:

- **`IGameSystem::IsSavingLocked` returns its `SaveLock` by value** (through `x8`). The Windows-shaped default wrote the
  lock through `x1`, which holds a bool, and the game crashed every time it checked whether it may save (on autosave,
  manual save, opening the menu). Every plugin with a game system (Codeware) inherited the crash.
- **`IRTTISystem::GetFunction` is vtable slot +0x28 on macOS, not +0x30.** Clang keeps the declaration order of the two
  global-function lookups, MSVC does not. +0x28 searches by full name; +0x30 searches a map keyed by a signature hash
  (short name mixed with parameter types). Every lookup of a script function by full name (`TSQ_ALL;`) failed; natives
  (registered with full name = short name) happened to work.

Upstream: codeman101 found the same two fixes independently and already proposed them:
[jackmaxwil/RED4ext.SDK-macos#3](https://github.com/jackmaxwil/RED4ext.SDK-macos/pull/3) (`IsSavingLocked` and other
x8 returns) and [#6](https://github.com/jackmaxwil/RED4ext.SDK-macos/pull/6) (`GetFunction`).

### 8.3 Address database

`address-db/cyberpunk2077_addresses.json` is the SDK's database for 2.3.1 plus **61 addresses for Codeware** found by
static analysis of the game executable (strings, native registration, vtable slots, call graphs; the reasoning is in
`_work/codeware/re_*.md`). Every code address starts at an entry of `LC_FUNCTION_STARTS`. Ten existing entries had wrong
offsets and were corrected. Five entries are rated medium confidence (`CGameFramework_InitializeGameInstance`,
`InkWidget_TriggerEvent`, `PlayerSystem_OnPlayerSpawned`, `QuestsSystem_CreateContext`,
`VehicleController_ToggleAuxLights`). Details: `address-db/README.md`.

### 8.4 Cyber Engine Tweaks (new macOS port)

CET runs as a RED4ext plugin (`CyberEngineTweaks.dylib`). Full notes: `macos/PORT_NOTES.md` in the CET repository.

| Area | Windows | macOS |
| --- | --- | --- |
| Loading | `version.dll` proxy | RED4ext plugin |
| Hooks | MinHook | RED4ext's hook engine (MinHook-compatible names) |
| Game addresses | CET's resolver | the SDK's verified resolver |
| Ticks | hooks on game-state tick functions | RED4ext game-state callbacks |
| `onInit` | `PlayerSystem::OnPlayerMainObjectSpawned` hook | first `PlayerPuppet::OnGameAttached` |
| Script `Log*` natives, `TDBID.ToStringDEBUG` | hooks on their addresses | their entries in the native handler table are replaced |
| `Observe` / `Override` | xbyak trampolines | a pool of 4096 compiled thunks (no executable memory created) |
| Overlay | Direct3D 12 | Dear ImGui on Metal, drawn into the game's own command buffer |
| Input | window procedure, raw input | NSEvent local monitors; key codes mapped to Windows virtual keys |
| Lua | LuaJIT with JIT | LuaJIT interpreter (`LUAJIT_DISABLE_JIT`, no runtime code generation) |
| SQLite | bundled | system `libsqlite3` |
| Allocator | mimalloc | system malloc |
| Fonts | FreeType | stb_truetype |
| Build | xmake | CMake with git submodules |

Fixes made while getting BT-7274 to run:

- **512 KiB thread stacks.** macOS gives secondary threads 512 KiB; two 512 KiB stack arrays (TweakDB resource list,
  Lua hooks) moved to the heap. Crash at start.
- **`Override` chains.** Lambdas captured the caller's parameters by reference and outlived the call (on arm64 the
  parameters live in the callee's frame). They capture by value now. Crash in `luaL_unref`.
- **`bindings.json`.** `auto config{json::parse(...)}` is brace-initialization of a json from a json, which clang
  resolves to the initializer-list constructor: a one-element array. With a saved key binding CET then failed to start
  ("type must be number, but is object").
- **`exEntitySpawner`.** CET's spawner uses Windows-only structures, and `WorldFunctionalTests.SpawnEntity` is not in the
  macOS game. A Lua implementation on top of Codeware's `DynamicEntitySystem` provides `Spawn`, `SpawnRecord` and
  `Despawn`. The position is read from `WorldTransform.Position` (fixed point, 17 fraction bits): `WorldTransform` has
  no `GetPosition()` in CET.
- **Record names.** macOS ships no `tweakdbstr.kark`, so CET has no TweakDB names. `TDBID.ToStringDEBUG` is now redirected
  (the class is registered as `TDBID`, not only as a script name), and CET remembers the names Lua turns into
  `TweakDBID`s (`TweakDBID.new("…")` and string arguments).

Security changes: see [section 11](#11-security-review).

Not available on macOS: JIT, `WorldFunctionalTests.SpawnEntity`, TweakDB names from `tweakdbstr` (Oodle-compressed and
not shipped), and the TweakDB editor's resource names.

Upstream: codeman101 ported CET to macOS independently, and that port is open as
[maximegmd/CyberEngineTweaks#1068](https://github.com/maximegmd/CyberEngineTweaks/pull/1068). The `Override` and
`bindings.json` fixes above are the same as [#1065](https://github.com/maximegmd/CyberEngineTweaks/pull/1065) and
[#1066](https://github.com/maximegmd/CyberEngineTweaks/pull/1066). The SQLite hardening, which applies to every
platform, is [#1070](https://github.com/maximegmd/CyberEngineTweaks/pull/1070).

### 8.5 Codeware

Ported from psiberx/cp2077-codeware 1.20.5 (branch `macos`). Notes: `macos/PORT_NOTES.md` and `macos/ADDRESSES.md` in
the Codeware repository. Entity spawning (`DynamicEntitySystem`), callbacks (`CallbackSystem`, including `Input/Key`),
reflection, localization and most of the script API work. Features whose game code is inlined or whose layouts are not
verified on macOS are compiled out and log one line when used (Resource/Load events, `Clone`,
`GetWorkspotInstance`, …).

Changes made while getting BT-7274 to run:

- **`Int64` script fields that are native `MemInt`.** On LP64 macOS native fields of C++ type `long` are registered as
  `MemInt`; the script imports (generated from the Windows game) say `Int64`, and script validation stopped the game.
- **redscript 0.5 crash.** The compiler bundled with the macOS game crashes ("constant pool error: definition not
  found") when `@addField` targets a class declared in the same compilation; `meshResource` is declared in the import.
- **UI hooks off.** The game crashed in its ink widget spawn jobs: Codeware appended to a widget library's external
  library list while other worker threads read it without a lock. The list is no longer modified, and Codeware's widget
  spawning and widget building hooks are off on macOS for now. Features that depend on them (custom controllers in
  widget names, `inkWidget/Spawn` events, Codeware's legacy custom controllers) do not work.
- Security review fixes: reflection call buffer bound, clipboard null checks, log size cap, a registry kept alive until
  exit.

Upstream: psiberx prefers not to take macOS support into Codeware yet
([psiberx/cp2077-codeware#55](https://github.com/psiberx/cp2077-codeware/issues/55)), so the port stays on the fork.
The fixes that apply to every platform are proposed:
[psiberx/cp2077-codeware#62](https://github.com/psiberx/cp2077-codeware/pull/62) (reflection bound, `meshResource`;
its issue [#61](https://github.com/psiberx/cp2077-codeware/issues/61) also describes the ink race),
[psiberx/cp2077-red-lib#14](https://github.com/psiberx/cp2077-red-lib/pull/14) (the same bound in RedLib) and
[dacap/clip#102](https://github.com/dacap/clip/pull/102) (clipboard checks).

### 8.6 AudioXL

The Windows AudioXL plays through Wwise's "Audio Input" plugin, which the macOS game does not contain. The macOS port
(branch `macos`) keeps AudioXL's registry, manifests and script API and plays each voice through an `AVAudioSourceNode`
on AVAudioEngine. Notes: `macos/PORT_NOTES.md` in the AudioXL repository.

- **`AudioXL.AudioXLLua`** (new): the script class Lua mods call (`Has`, `Play`, `PlayLine`, `Stop`, `Pause`, `PlayFrom`,
  emitter calls), rebuilt over AudioXL's natives because it is missing from the available sources.
- Only `AudioSystem.Play` and `PlayOnEmitter` are intercepted; soundbanks are not loaded; HTTP streaming is compiled
  out; positional audio is an approximation of Wwise's.
- Security: manifests and sound files are read only inside the game folder; manifest nesting is bounded; bind slots are
  claimed atomically; non-finite positions are refused.
- RedLib is a submodule on a `macos` branch (clang/Itanium fixes from the TweakXL and ArchiveXL ports; the same fixes
  are proposed upstream in [psiberx/cp2077-red-lib#12](https://github.com/psiberx/cp2077-red-lib/pull/12)).
- Published as [andr0s/cp2077-audio-xl](https://github.com/andr0s/cp2077-audio-xl). The original repository is no
  longer on GitHub, so the port starts from its last public state without the commit history (no author e-mail
  addresses are republished); the license and the credit stay with DigitalVixen.

### 8.7 TweakXL and ArchiveXL

jackmaxwil's macOS ports, used unchanged. ArchiveXL defers its ink spawning hooks to Codeware when Codeware is 1.14 or
newer, so with Codeware's UI hooks off on macOS nobody hooks ink spawning (the vanilla behaviour).

### 8.8 redscript

The macOS game ships CD PROJEKT's build of the redscript compiler (`engine/tools/scc`, 0.5.x, older layout than current
redscript). No separate redscript install is needed. The launcher compiles `r6/scripts` plus the staged plugin scripts
before every start.

---

## 9. macOS pitfalls for plugin and mod developers

What broke during this port, as a checklist for anyone porting RED4ext plugins or writing mods that should work on Mac:

1. **Struct returns go through `x8`.** On Windows x64 a function returning a non-trivial struct takes a hidden pointer
   as its first argument (after `this`), which reverse-engineered headers often declare as an explicit out-parameter.
   On arm64 the buffer is in `x8` and the real arguments shift. Declare such functions as returning the struct by value
   on macOS (`IsSavingLocked`, `SpawnEntity`, `GetLayer`, `Handle`-returning getters, …).
2. **Itanium vtables.** Destructors take two slots, so virtuals after the destructor move by 8 bytes; and clang keeps
   the declaration order of overloaded virtuals where MSVC can reverse it (`IRTTISystem::GetFunction`).
3. **Secondary threads have 512 KiB stacks.** Large stack arrays that are fine on Windows (1 MiB) overflow.
4. **Lambdas that outlive the call must not capture parameters by reference.**
5. **`long` is 8 bytes (LP64).** The game registers such fields as `MemInt`, not `Int64` / `Int32`.
6. **`auto x{json_value}` with nlohmann/json makes an array on clang.** Use `=`.
7. **Late hooks need address space.** Reserve what you need early; the game reserves about 130 GiB after start.
8. **Game worker threads are real threads.** Do not modify game-owned arrays (resources, libraries) from a hook while
   the game may read them on another thread.
9. **There are no TweakDB names on Mac.** Do not compare `TDBID.ToStringDEBUG(id)` with a string; compare TweakDBIDs:
   `id == TweakDBID.new("Character.bt7274")` (Lua) or `Equals(id, t"Character.bt7274")` (redscript).
10. **The bundled redscript is 0.5.** Avoid `@addField` on a class declared in the same compilation; the compiler
    crashes.
11. **Fail closed on addresses.** An address that is not verified for the exact game build must resolve to nothing, and
    the feature must switch itself off.

---

## 10. Notes for the BT-7274 author and other mod authors

Thank you for BT-7274: it is a great mod, and it runs on macOS with these frameworks. A few things that would make it
work out of the box on Mac (and are harmless on Windows):

- **Keys without a numpad.** MacBooks (and many laptops) have no numeric keypad, and Mod Settings is not available on
  macOS. Defaults outside the numpad, or a key setting in CET's own bindings (`registerInput` / `registerHotkey`),
  would help.
- **Do not depend on `TDBID.ToStringDEBUG` for logic.** On macOS it only knows names that were created as strings in the
  same session. Comparing TweakDBIDs (`TweakDBID.new(UNITS.bt.record)`) works everywhere.
- **`exEntitySpawner`** is a CET API that is not natively available on macOS; it is emulated here on top of Codeware's
  `DynamicEntitySystem`. Calling `Game.GetDynamicEntitySystem():CreateEntity(spec)` directly would remove the
  dependency.
- **`AudioXL.AudioXLLua`** is not in the public AudioXL sources; if its source or interface could be published, the
  macOS version could follow it exactly.
- Harmless warning seen in 2.3.1: `Function UpdateIcon in class MinimapQuestMappinController does not exist` (one of the
  four mappin classes the quest observes).

---

## 11. Security review

All sources were read before building; no downloader, telemetry, autostart or obfuscated code was found in any of
them. The detailed reports are kept back until the upstream projects have merged the fixes (responsible
disclosure); the fixes themselves are in the pull requests ([Appendix C](#appendix-c-upstream-pull-requests)).
Findings and what was done:

| Finding | Where | What | Done |
| --- | --- | --- | --- |
| CET F1, F2 (High) | CET | A Lua mod could load a native library (`db:load_extension`) and read/write any file through SQL `ATTACH` | Fixed on all platforms: the function is gone; a SQLite authorizer refuses `ATTACH` of files and `load_extension()` |
| CET F3–F5, F8 (High/Medium on macOS) | CET | Lua mod path checks were written for Windows (separators, symlinks, case-insensitive APFS) | Rewritten for macOS (`PathJail.h`), 43 unit tests |
| AXL-1 (Medium) | AudioXL | A manifest could name a file outside the mod folder | Fixed on all platforms |
| AXL-2 (Low) | AudioXL | HTTP sound streaming | Compiled out on macOS |
| L-1 (Low) | `red4ext_plugin_check` | Out-of-bounds read on malformed Mach-O | Fixed |
| L-5 (Low) | RED4ext | A test function let any mod grow a log without limit | Registered only for the test driver |
| H-1, H-2, M-3 | RED4ext installer | `curl \| bash` installed unverified prebuilt binaries | Not used: everything is built from reviewed sources; downloads are pinned by hash |
| L-2 (Low) | RED4ext uninstall | Restored a backup without checking it and deleted the whole `red4ext` folder | `cp2077-mac.sh uninstall` checks CD PROJEKT's signature and removes only its own files |
| CW-1, CW-2 | another Codeware macOS port | Debug writes to `/tmp`, `pbpaste` through `popen` | Not used: Codeware was ported again from psiberx's sources |
| MS-1 (High) | Mod Settings | Stack buffer overflow from mod data | Not ported; do not install it until fixed |

What remains by design:

- The game executable is re-signed ad hoc with three weakening entitlements while RED4ext is installed.
- RED4ext plugins are native code with your user's privileges.
- Lua mods share the global `string`, `table`, … tables (CET F6), as on Windows.

---

## 12. Uninstalling

```bash
./cp2077-mac.sh uninstall
```

It removes exactly the files `install` recorded and restores the original executable (only if its hash matches the one
recorded before re-signing). Mods added with `add-mod` are removed with `remove-mod`. Alternatively, Steam →
Cyberpunk 2077 → Properties → Installed Files → *Verify integrity of game files* restores the original executable.

---

## 13. Known limitations

- Only Cyberpunk 2077 **2.3.1, Steam, Apple silicon**. A game update needs a new address database.
- **Mod Settings** and **Appearance Menu Mod (AMM)** are not ported. Mods fall back to their default settings.
- **Codeware UI hooks are off** on macOS (custom controllers in widget names, `inkWidget/Spawn` events, legacy custom
  controllers).
- **No TweakDB names** (`tweakdbstr` is not shipped on macOS): `ToStringDEBUG` knows only names created as strings.
- CET without JIT: Lua runs in the interpreter (fast enough for the tested mods).
- AudioXL: no soundbanks, approximate positional audio, only `AudioSystem.Play` / `PlayOnEmitter` intercepted.
- Codeware features compiled out on macOS log one line when used (see Codeware's port notes).
- Windows-only mods (`.dll`, ASI, ReShade, …) do not work.

---

## 14. Credits and licenses

- **RED4ext** and **RED4ext.SDK**: Octavian Dima (WopsS) and contributors, MIT. macOS ports: **jackmaxwil**.
- **Cyber Engine Tweaks**: yamashi and the CET team (maximegmd/CyberEngineTweaks), MIT.
- **TweakXL**, **ArchiveXL**, **Codeware**, **RedLib**: Pavel Siberx (psiberx), MIT. TweakXL and ArchiveXL macOS ports:
  **jackmaxwil**.
- **AudioXL**: DigitalVixen, MIT.
- **clip**: David Capello, MIT. Dear ImGui, sol2, spdlog, nlohmann/json, LuaJIT (OpenResty), TiltedCore,
  hopscotch-map, stb, fishhook: their authors, under their licenses.
- **redscript** (as bundled with the game) and **Cyberpunk 2077**: CD PROJEKT RED.
- **Titanfall BT-7274 (Quest and Companion)**: its author on Nexus Mods (mod 35041).
- macOS ports of CET, Codeware and AudioXL, fixes to RED4ext and the SDK, the address database work, the tooling and
  this guide: **andr0s** (<https://github.com/andr0s>), with help from Claude (Anthropic).

All changes made here are under the license of the project they change (MIT for all of the above).

---

## Appendix A: file locations

Game folder (default): `~/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077`

| Path in the game folder | What |
| --- | --- |
| `Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077` | game executable (re-signed while installed) |
| `Cyberpunk2077.orig` | backup of the original executable |
| `launch_red4ext.sh` | the launcher `play` runs |
| `red4ext/RED4ext.dylib` | the loader |
| `red4ext/bin/x64/cyberpunk2077_addresses.json` | the address database |
| `red4ext/bin/red4ext_plugin_check` | the address gate |
| `red4ext/macos/scripts/` | `install_macos.sh`, `codesign_macos.sh`, entitlements |
| `red4ext/plugins/<Plugin>/` | `TweakXL`, `ArchiveXL`, `Codeware`, `AudioXL`, `CyberEngineTweaks` (+ their `Scripts`, data) |
| `red4ext/logs/` | loader and AudioXL logs, hook statistics |
| `bin/x64/plugins/cyber_engine_tweaks/` | CET data: `mods/`, `scripts/`, `fonts/`, settings, logs |
| `r6/scripts/`, `r6/tweaks/`, `archive/pc/mod/` | script mods, tweaks, archives |
| `r6/cache/final.redscripts(.bk)` | compiled scripts and the game's original bundle |

Outside the game folder:

| Path | What |
| --- | --- |
| `~/Library/Application Support/CD Projekt Red/Cyberpunk 2077/saves` | saves |
| `~/Library/Logs/DiagnosticReports/Cyberpunk2077-*.ips` | crash reports |
| `<cp2077-mac-port>/install-state/` | what `install` and `add-mod` copied (used by `uninstall` / `remove-mod`) |

---

## Appendix B: repositories, branches and commits

State of the branches this guide describes:

| Repository (local folder) | Upstream | Branch | Head |
| --- | --- | --- | --- |
| `cp2077-mac-port` (tooling, this guide) | — | `main` | latest |
| `ports/RED4ext-macos` | `jackmaxwil/RED4ext-macos` @ `cd693be` | `macos-fixes` | `de806f2` |
| `ports/CyberEngineTweaks` | `maximegmd/CyberEngineTweaks` @ `bcbe457` | `macos` | `67f1ac8` |
| `…/vendor/RED4ext.SDK-macos` | `jackmaxwil/RED4ext.SDK-macos` @ `f657158c` | `macos-fixes` | `e6cb810b` |
| `ports/Codeware` | `psiberx/cp2077-codeware` @ `a88e8aa5` | `macos` | `e6af1c0e` |
| `…/vendor/clip` | `dacap/clip` @ `4bd90a5` | `macos` | `568c65ed` |
| `ports/AudioXL` | `DigitalVixenSWE/cp2077-audio-xl` @ `344c5a1` (no longer on GitHub) | `macos` | `7819916` |
| `…/plugin/deps/RedLib` | `psiberx/cp2077-red-lib` @ `009d404` | `macos` | `5e0edef` |
| `src/jackmaxwil_cp2077-tweak-xl-macos` | `jackmaxwil/cp2077-tweak-xl-macos` | — | `4ba4565` |
| `src/jackmaxwil_cp2077-archive-xl-macos` | `jackmaxwil/cp2077-archive-xl-macos` | — | `033f7ec` |

The changes are commits with explanations on these branches (`git log <upstream>..HEAD` in each repository). The
forks are at `https://github.com/andr0s/<repository>`. All commits made here use
`298775+andr0s@users.noreply.github.com`.

---

## Appendix C: upstream pull requests

The fixes that upstream projects can take as they are were sent as pull requests, each from a branch based on the
current upstream head, each with an issue:

| Project | Pull request | Issue | What |
| --- | --- | --- | --- |
| RED4ext (macOS) | [jackmaxwil/RED4ext-macos#10](https://github.com/jackmaxwil/RED4ext-macos/pull/10) | [#9](https://github.com/jackmaxwil/RED4ext-macos/issues/9) | stub page pool for late hooks, `plugin_check` bounds, `RED4ext_TestReport` only for tests, launcher restores the script bundle, CMake 3.25 |
| Cyber Engine Tweaks | [maximegmd/CyberEngineTweaks#1070](https://github.com/maximegmd/CyberEngineTweaks/pull/1070) | [#1069](https://github.com/maximegmd/CyberEngineTweaks/issues/1069) | Lua sandbox: SQLite `load_extension` removed, `ATTACH` of files refused (every platform) |
| Codeware | [psiberx/cp2077-codeware#62](https://github.com/psiberx/cp2077-codeware/pull/62) | [#61](https://github.com/psiberx/cp2077-codeware/issues/61) | reflection call bound, `meshResource` in its import; the issue also reports the ink library race |
| RedLib | [psiberx/cp2077-red-lib#14](https://github.com/psiberx/cp2077-red-lib/pull/14) | [#13](https://github.com/psiberx/cp2077-red-lib/issues/13) | reflection call bound |
| clip | [dacap/clip#102](https://github.com/dacap/clip/pull/102) | [#101](https://github.com/dacap/clip/issues/101) | macOS clipboard nil checks |
| CET macOS port ([#1068](https://github.com/maximegmd/CyberEngineTweaks/pull/1068)) | [codeman101/CyberEngineTweaks#1](https://github.com/codeman101/CyberEngineTweaks/pull/1) | — (issues are off on that fork) | `\` separators in mod paths, the mod's database under other spellings |

Not sent, because codeman101, who ports the same frameworks to macOS independently, had already proposed the same
change (or upstream does not want it yet):

| Change here | Upstream |
| --- | --- |
| SDK: `IsSavingLocked` returns by value | [jackmaxwil/RED4ext.SDK-macos#3](https://github.com/jackmaxwil/RED4ext.SDK-macos/pull/3) |
| SDK: `GetFunction` is slot +0x28 | [jackmaxwil/RED4ext.SDK-macos#6](https://github.com/jackmaxwil/RED4ext.SDK-macos/pull/6) |
| CET: `Override` captures by value | [maximegmd/CyberEngineTweaks#1065](https://github.com/maximegmd/CyberEngineTweaks/pull/1065) |
| CET: `bindings.json` copy-initialization | [maximegmd/CyberEngineTweaks#1066](https://github.com/maximegmd/CyberEngineTweaks/pull/1066) |
| CET: the macOS port | a different port is open as [maximegmd/CyberEngineTweaks#1068](https://github.com/maximegmd/CyberEngineTweaks/pull/1068) |
| RedLib: clang fixes | [psiberx/cp2077-red-lib#12](https://github.com/psiberx/cp2077-red-lib/pull/12) |
| Codeware: the macOS port | not wanted upstream yet ([psiberx/cp2077-codeware#55](https://github.com/psiberx/cp2077-codeware/issues/55)) |

AudioXL has no upstream to send fixes to (its original repository is no longer on GitHub); the port is published as
[andr0s/cp2077-audio-xl](https://github.com/andr0s/cp2077-audio-xl). Still open: TweakDB names from Lua strings, which
#1068 does not have. #1068's own path checks hold (`..`,
absolute paths and symlinks cannot leave the mod folder); the two path fixes it lacked are proposed to it in
[codeman101/CyberEngineTweaks#1](https://github.com/codeman101/CyberEngineTweaks/pull/1).
