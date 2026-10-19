#!/bin/bash
# cp2077-mac.sh: build, install and start the macOS modding stack for Cyberpunk 2077 2.3.1 (Steam, Apple silicon)
# from the reviewed sources in this folder. Everything fetched at build time is pinned: git submodules by commit,
# CMake downloads and ArchiveXL's resource archive by SHA-256.
#
#   ./cp2077-mac.sh doctor      check the Mac, the tools and the game build; changes nothing
#   ./cp2077-mac.sh build       build RED4ext, TweakXL, ArchiveXL, Codeware, AudioXL and Cyber Engine Tweaks into build/
#   ./cp2077-mac.sh install     copy them into the game folder, then back up and re-sign the game executable
#   ./cp2077-mac.sh play        start the game with mods (Steam must be running)
#   ./cp2077-mac.sh add-mod <unpacked mod folder>    copy a mod (data and scripts only) into the game, recorded
#   ./cp2077-mac.sh remove-mod <name>                remove the files add-mod copied for that mod
#   ./cp2077-mac.sh uninstall   remove what install copied and restore the original game executable
#
# The installed layout is RED4ext-macos's release layout (ports/RED4ext-macos/scripts/create_release.sh). Set
# CP2077_GAME_DIR if the game is not in the default Steam library.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME="${CP2077_GAME_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077}"
BIN="$GAME/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077"
BACKUP="$GAME/Cyberpunk2077.orig"
PLUGINS="$GAME/red4ext/plugins"
CET_DATA="$GAME/bin/x64/plugins/cyber_engine_tweaks"
GAME_REAL="$(cd "$GAME" 2>/dev/null && pwd -P || true)"   # symlinks resolved
# Install records live outside the game folder, where a mod archive extracted into the game cannot rewrite them.
STATE="$ROOT/install-state"
MANIFEST="$STATE/installed-files.txt"
EXE_HASH="$STATE/original-executable.sha256"
BUILD="$ROOT/build"
CDPR_TEAM="PL47UP47QQ"

# Sources (README.md says what was reviewed and changed in each)
SDK_DIR="$ROOT/ports/CyberEngineTweaks/vendor/RED4ext.SDK-macos"   # jackmaxwil/RED4ext.SDK-macos f657158c + branch macos-fixes, for every plugin
RED4EXT_DIR="$ROOT/ports/RED4ext-macos"
TWEAKXL_DIR="$ROOT/src/jackmaxwil_cp2077-tweak-xl-macos"
ARCHIVEXL_DIR="$ROOT/src/jackmaxwil_cp2077-archive-xl-macos"
CODEWARE_DIR="$ROOT/ports/Codeware"
AUDIOXL_DIR="$ROOT/ports/AudioXL"
CET_DIR="$ROOT/ports/CyberEngineTweaks"
# The SDK's database plus the 61 Codeware entries found by static analysis (address-db/README.md).
ADDRESS_DB="$ROOT/address-db/cyberpunk2077_addresses.json"

PLUGIN_NAMES=(TweakXL ArchiveXL Codeware AudioXL CyberEngineTweaks)

ARCHIVEXL_BUNDLE_VERSION=1.27.4
ARCHIVEXL_ZIP_SHA256=6c638dc1108a565ff4806835a17f704cbb2ab2afeb8ccf700080797c3f70591c
ARCHIVEXL_ARCHIVE_SHA256=70b2967832555292712cc583753da4823113dc7a3f388030b45c6966909241d3

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

# LC_UUID of a Mach-O file, uppercase hex without dashes (as RED4ext's install_macos.sh reads it).
macho_uuid() {
    perl -e 'open F,"<",$ARGV[0] or exit 1; binmode F; read F,$h,32; ($m,$ct,$st,$ft,$n)=unpack("V5",$h);
        exit 1 unless $m==0xfeedfacf; $o=32; for(1..$n){seek F,$o,0; read F,$c,8; ($cmd,$sz)=unpack("V2",$c);
        if($cmd==0x1b){read F,$u,16; print uc unpack("H*",$u); exit 0} $o+=$sz} exit 1' "$1"
}
db_uuid() { grep -o '"uuid" *: *"[^"]*"' "$1" | head -1 | sed 's/.*"\([^"]*\)"$/\1/' | tr -d '-' | tr '[:lower:]' '[:upper:]'; }

version_at_least() { # have want, both major.minor
    local h1 h2 w1 w2 rest
    IFS=. read -r h1 h2 rest <<<"$1"
    IFS=. read -r w1 w2 rest <<<"$2"
    (( h1 > w1 || (h1 == w1 && h2 >= w2) ))
}

# Output is captured before matching: `codesign ... | grep -q` races under pipefail (grep exits, codesign gets SIGPIPE).
is_cdpr_signed() { [[ "$(codesign -dv "$1" 2>&1)" == *"TeamIdentifier=$CDPR_TEAM"* ]]; }
has_red4ext_entitlements() { [[ "$(codesign -d --entitlements - --xml "$1" 2>/dev/null)" == *allow-unsigned-executable-memory* ]]; }

built_dylib() { # plugin name -> path of the built dylib
    case "$1" in
    RED4ext) echo "$BUILD/RED4ext/libs/RED4ext.dylib" ;;
    *) echo "$BUILD/$1/$1.dylib" ;;
    esac
}

record() { printf '%s\n' "$1" >> "$MANIFEST"; }

# True if the deepest existing folder on the path resolves (symlinks followed) to the game folder or below. Mod
# archives can contain symlinks; nothing is written or deleted through one that leads out of the game folder.
inside_game() {
    local dir
    dir="$(dirname "$1")"
    while [[ ! -d "$dir" ]]; do dir="$(dirname "$dir")"; done
    dir="$(cd "$dir" && pwd -P)" || return 1
    [[ -n "$GAME_REAL" && ( "$dir" == "$GAME_REAL" || "$dir" == "$GAME_REAL"/* ) ]]
}

install_file() { # source destination
    [[ -f "$1" ]] || die "missing $1 (run ./cp2077-mac.sh build)"
    inside_game "$2" || die "not writing $2: a folder on the way leads out of the game folder"
    mkdir -p "$(dirname "$2")"
    if [[ -L "$2" ]]; then rm -f "$2"; fi   # a planted symlink is replaced, never written through
    cp -f "$1" "$2"
    record "$2"
}

install_tree() { # source-dir destination-dir [subdir of source to leave out]
    local src="$1" dst="$2" skip="${3:-}" f rel
    [[ -d "$src" ]] || die "missing $src"
    while IFS= read -r -d '' f; do
        rel="${f#"$src"/}"
        [[ -n "$skip" && ( "$rel" == "$skip" || "$rel" == "$skip"/* ) ]] && continue
        [[ "$(basename "$rel")" == .DS_Store ]] && continue
        install_file "$f" "$dst/$rel"
    done < <(find "$src" -type f -print0)
}

remove_installed() { # deletes the files the last install recorded, nothing else
    [[ -f "$MANIFEST" ]] || return 0
    local f
    while IFS= read -r f; do
        [[ -n "$f" && "$f" == "$GAME/"* && "$f" != *"/../"* ]] || continue
        if ! inside_game "$f"; then say "skipped $f: its folder leads out of the game folder"; continue; fi
        rm -f "$f"   # on a symlink this removes the link, not what it points to
    done < "$MANIFEST"
    rm -f "$MANIFEST"
}

cmd_doctor() {
    local ok=1
    [[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]] || { say "- not an Apple silicon Mac"; ok=0; }
    if [[ -f "$BIN" ]]; then
        local game_uuid want_uuid
        game_uuid="$(macho_uuid "$BIN" || true)"
        want_uuid="$(db_uuid "$ADDRESS_DB" 2>/dev/null || true)"
        if [[ -n "$game_uuid" && "$game_uuid" == "$want_uuid" ]]; then say "+ game build matches the address database (2.3.1)"
        else say "- game executable $game_uuid is not the build the address database covers ($want_uuid)"; ok=0; fi
        if has_red4ext_entitlements "$BIN"; then say "+ game executable is set up for RED4ext"
        elif is_cdpr_signed "$BIN"; then say "  game executable is CD PROJEKT's original (install re-signs it)"
        else say "- game executable is neither CD PROJEKT's original nor set up for RED4ext: Steam > Verify integrity"; ok=0; fi
    else
        say "- game not found at $GAME (set CP2077_GAME_DIR)"; ok=0
    fi
    local clang_major sdk cmake_version
    clang_major="$(xcrun clang --version 2>/dev/null | sed -nE 's/^Apple clang version ([0-9]+).*/\1/p')"
    if [[ -n "$clang_major" && "$clang_major" -ge 16 ]]; then say "+ Apple clang $clang_major"
    else say "- Apple clang ${clang_major:-not found}, 16 or newer needed: softwareupdate --list, then install Command Line Tools"; ok=0; fi
    sdk="$(xcrun --show-sdk-version 2>/dev/null || true)"
    if [[ -n "$sdk" && "${sdk%%.*}" -ge 14 ]]; then say "+ macOS SDK $sdk"; else say "- macOS SDK ${sdk:-not found}, 14 or newer needed"; ok=0; fi
    cmake_version="$(cmake --version 2>/dev/null | sed -nE 's/^cmake version ([0-9]+\.[0-9]+).*/\1/p')"
    if [[ -n "$cmake_version" ]] && version_at_least "$cmake_version" 3.25; then say "+ CMake $cmake_version"
    else say "- CMake ${cmake_version:-not found}, 3.25 or newer needed (brew install cmake)"; ok=0; fi
    if pgrep -q steam_osx; then say "+ Steam is running"; else say "  Steam is not running (needed for play)"; fi
    local d
    for d in "$SDK_DIR" "$RED4EXT_DIR" "$TWEAKXL_DIR" "$ARCHIVEXL_DIR" "$CODEWARE_DIR" "$AUDIOXL_DIR" "$CET_DIR"; do
        [[ -d "$d" ]] || { say "- missing source folder $d"; ok=0; }
    done
    if [[ $ok == 1 ]]; then say "Ready."; else say "Fix the items marked '-' first."; return 1; fi
}

CXX_STD_FLAGS=""   # set by cmd_build

cmake_build() { # name source-dir [extra cmake args...]
    local name="$1" src="$2"
    shift 2
    say "== $name"
    cmake -S "$src" -B "$BUILD/$name" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="$CXX_STD_FLAGS" -DCMAKE_OBJCXX_FLAGS="$CXX_STD_FLAGS" "$@"
    cmake --build "$BUILD/$name" -j"$(sysctl -n hw.ncpu)"
}

fetch_archivexl_bundle() {
    # ArchiveXL's packed resources are built with WolvenKit on Windows; the archive format is the same on macOS.
    local out="$BUILD/ArchiveXL.archive" tmp
    [[ -f "$out" && "$(shasum -a 256 "$out" | cut -d' ' -f1)" == "$ARCHIVEXL_ARCHIVE_SHA256" ]] && return 0
    tmp="$(mktemp -d)"
    curl -fsSL --proto '=https' -o "$tmp/axl.zip" \
        "https://github.com/psiberx/cp2077-archive-xl/releases/download/v$ARCHIVEXL_BUNDLE_VERSION/ArchiveXL-$ARCHIVEXL_BUNDLE_VERSION.zip"
    [[ "$(shasum -a 256 "$tmp/axl.zip" | cut -d' ' -f1)" == "$ARCHIVEXL_ZIP_SHA256" ]] || die "ArchiveXL-$ARCHIVEXL_BUNDLE_VERSION.zip checksum mismatch"
    unzip -q -o "$tmp/axl.zip" red4ext/plugins/ArchiveXL/Bundle/ArchiveXL.archive -d "$tmp/x"
    [[ "$(shasum -a 256 "$tmp/x/red4ext/plugins/ArchiveXL/Bundle/ArchiveXL.archive" | cut -d' ' -f1)" == "$ARCHIVEXL_ARCHIVE_SHA256" ]] \
        || die "ArchiveXL.archive checksum mismatch"
    mv "$tmp/x/red4ext/plugins/ArchiveXL/Bundle/ArchiveXL.archive" "$out"
    rm -rf "$tmp"
}

cmd_build() {
    cmd_doctor >/dev/null || die "./cp2077-mac.sh doctor reports problems; fix them first"
    # Apple's compilers even when another clang (Homebrew LLVM) comes first in PATH.
    CC="$(xcrun -f clang)"; CXX="$(xcrun -f clang++)"
    export CC CXX OBJC="$CC" OBJCXX="$CXX"
    # Command Line Tools installed over an old copy keep its libc++ headers in usr/include/c++/v1, which clang
    # searches before the SDK's (no <ranges>, no <concepts>). Use the SDK's libc++ headers then.
    local sdk_cxx stale_cxx
    sdk_cxx="$(xcrun --show-sdk-path)/usr/include/c++/v1"
    stale_cxx="$(dirname "$CXX")/../include/c++/v1"
    if [[ -d "$stale_cxx" && -d "$sdk_cxx" ]]; then
        CXX_STD_FLAGS="-stdlib++-isystem $sdk_cxx"
        say "Using the SDK's C++ headers (old ones found in $stale_cxx)"
    fi

    git -C "$RED4EXT_DIR" submodule update --init deps/fmt deps/red4ext.sdk deps/simdjson deps/spdlog deps/toml11 \
        deps/ordered-map deps/redscript deps/fishhook
    git -C "$TWEAKXL_DIR" submodule update --init
    git -C "$ARCHIVEXL_DIR" submodule update --init
    git -C "$CODEWARE_DIR" submodule update --init vendor/clip vendor/nameof vendor/semver
    git -C "$CET_DIR" submodule update --init vendor/RED4ext.SDK-macos vendor/luajit vendor/sol2 vendor/imgui vendor/spdlog \
        vendor/json vendor/TiltedCore vendor/hopscotch-map vendor/stb vendor/fishhook

    cmake_build RED4ext "$RED4EXT_DIR"
    cmake_build TweakXL "$TWEAKXL_DIR" -DRED4EXT_SDK_DIR="$SDK_DIR"
    cmake_build ArchiveXL "$ARCHIVEXL_DIR" -DRED4EXT_SDK_DIR="$SDK_DIR"
    cmake_build Codeware "$CODEWARE_DIR" -DRED4EXT_SDK_DIR="$SDK_DIR"
    # Codeware's install rules leave out the scripts that declare Windows-only game classes; stage them here.
    rm -rf "$BUILD/stage/Codeware"
    cmake --install "$BUILD/Codeware" --prefix "$BUILD/stage/Codeware"
    cmake_build AudioXL "$AUDIOXL_DIR/macos" -DRED4EXT_SDK_DIR="$SDK_DIR"
    cmake_build CyberEngineTweaks "$CET_DIR" -DRED4EXT_SDK_DIR="$SDK_DIR"
    ctest --test-dir "$BUILD/CyberEngineTweaks" --output-on-failure
    fetch_archivexl_bundle

    local dylibs=() name bin id foreign
    for name in "${PLUGIN_NAMES[@]}"; do
        bin="$(built_dylib "$name")"
        [[ -f "$bin" ]] || die "$name: $bin was not produced"
        dylibs+=("$bin")
    done
    # Players' Macs have only the system libraries (same check as RED4ext's release script).
    for bin in "$(built_dylib RED4ext)" "$BUILD/RED4ext/bin/red4ext_plugin_check" "${dylibs[@]}"; do
        id="$(otool -D "$bin" 2>/dev/null | tail -n +2)"
        foreign="$(otool -L "$bin" | tail -n +2 | awk '{print $1}' | grep -v -e '^/usr/lib/' -e '^/System/' | grep -vxF "${id:-none}" || true)"
        [[ -z "$foreign" ]] || die "$bin links libraries macOS does not have: $foreign"
    done
    # RED4ext's address gate: a plugin that uses a game address the database does not mark verified is not loaded.
    "$BUILD/RED4ext/bin/red4ext_plugin_check" "$ADDRESS_DB" "${dylibs[@]}" \
        || die "a plugin uses game addresses that are not verified for this game build; RED4ext would refuse it"
    say "Build finished. Next: ./cp2077-mac.sh install"
}

cmd_install() {
    [[ -f "$BIN" ]] || die "game not found at $GAME"
    [[ -f "$(built_dylib RED4ext)" ]] || die "run ./cp2077-mac.sh build first"
    [[ "$(macho_uuid "$BIN")" == "$(db_uuid "$ADDRESS_DB")" ]] || die "the installed game is not the build the address database covers"
    if [[ ! -f "$BACKUP" ]] && ! is_cdpr_signed "$BIN"; then
        die "the game executable is not CD PROJEKT's original and there is no backup; run Steam > Verify integrity first"
    fi

    # A reinstall first removes what the previous install copied, so files dropped from the sources do not linger.
    remove_installed
    mkdir -p "$STATE"
    : > "$MANIFEST"

    install_file "$(built_dylib RED4ext)" "$GAME/red4ext/RED4ext.dylib"
    install_file "$BUILD/RED4ext/bin/red4ext_plugin_check" "$GAME/red4ext/bin/red4ext_plugin_check"
    install_file "$ADDRESS_DB" "$GAME/red4ext/bin/x64/cyberpunk2077_addresses.json"
    install_file "$RED4EXT_DIR/scripts/launch_red4ext.sh" "$GAME/launch_red4ext.sh"
    local f
    for f in install_macos.sh codesign_macos.sh red4ext_entitlements.plist; do
        install_file "$RED4EXT_DIR/scripts/$f" "$GAME/red4ext/macos/scripts/$f"
    done
    chmod +x "$GAME/launch_red4ext.sh" "$GAME/red4ext/macos/scripts/install_macos.sh" "$GAME/red4ext/macos/scripts/codesign_macos.sh"

    # Plugin scripts go in each plugin's Scripts folder: launch_red4ext.sh compiles them only for plugins RED4ext will
    # load, so starting the game from Steam without mods keeps working.
    install_file "$(built_dylib TweakXL)" "$PLUGINS/TweakXL/TweakXL.dylib"
    install_tree "$TWEAKXL_DIR/scripts" "$PLUGINS/TweakXL/Scripts" r6
    install_tree "$TWEAKXL_DIR/scripts/r6" "$GAME/r6"
    install_tree "$TWEAKXL_DIR/data" "$PLUGINS/TweakXL/Data"

    install_file "$(built_dylib ArchiveXL)" "$PLUGINS/ArchiveXL/ArchiveXL.dylib"
    install_tree "$ARCHIVEXL_DIR/scripts" "$PLUGINS/ArchiveXL/Scripts"
    install_tree "$ARCHIVEXL_DIR/bundle/source/resources" "$PLUGINS/ArchiveXL/Bundle"
    install_file "$BUILD/ArchiveXL.archive" "$PLUGINS/ArchiveXL/Bundle/ArchiveXL.archive"

    install_tree "$BUILD/stage/Codeware/red4ext/plugins/Codeware" "$PLUGINS/Codeware"

    install_file "$(built_dylib AudioXL)" "$PLUGINS/AudioXL/AudioXL.dylib"
    install_tree "$AUDIOXL_DIR/r6/scripts/AudioXL" "$PLUGINS/AudioXL/Scripts"

    install_file "$(built_dylib CyberEngineTweaks)" "$PLUGINS/CyberEngineTweaks/CyberEngineTweaks.dylib"
    install_tree "$CET_DIR/scripts" "$CET_DATA/scripts"
    install_tree "$CET_DIR/fonts" "$CET_DATA/fonts"

    # Locally built code is signed ad hoc, as RED4ext's release does.
    local signed=("$GAME/red4ext/RED4ext.dylib") name
    for name in "${PLUGIN_NAMES[@]}"; do signed+=("$PLUGINS/$name/$name.dylib"); done
    "$GAME/red4ext/macos/scripts/codesign_macos.sh" dylib "${signed[@]}"
    codesign -f -s - "$GAME/red4ext/bin/red4ext_plugin_check"

    # RED4ext's own setup step: checks the game build against the database, clears quarantine flags, backs the
    # executable up once (Cyberpunk2077.orig) and re-signs it with the entitlements RED4ext needs. The hash of the
    # original is kept here so uninstall restores exactly that file.
    if [[ ! -f "$BACKUP" ]]; then shasum -a 256 "$BIN" | cut -d' ' -f1 > "$EXE_HASH"; fi
    "$GAME/red4ext/macos/scripts/install_macos.sh"
    say "Installed. Start the game with ./cp2077-mac.sh play (Steam's Play button starts it without mods)."
}

# Folders a mod archive may write to. Everything else (the game app, engine/, RED4ext itself, bin/x64 outside CET's
# mods folder) is refused, and so is native code: a RED4ext plugin needs its own review, not an unpacked archive.
MOD_PLACES=("archive/pc/mod/" "bin/x64/plugins/cyber_engine_tweaks/mods/" "mods/" "r6/scripts/" "r6/tweaks/" "r6/input/"
    "red4ext/plugins/")

cmd_add_mod() { # unpacked mod folder
    local src="${1:?usage: ./cp2077-mac.sh add-mod <unpacked mod folder>}" name f rel list place ok
    [[ -d "$src" ]] || die "not a folder: $src"
    src="$(cd "$src" && pwd -P)"
    name="$(basename "$src")"
    [[ -f "$MANIFEST" ]] || die "run ./cp2077-mac.sh install first"
    # Check every file before copying any.
    while IFS= read -r -d '' f; do
        rel="${f#"$src"/}"
        [[ "$(basename "$rel")" == .DS_Store ]] && continue
        ok=0
        for place in "${MOD_PLACES[@]}"; do [[ "$rel" == "$place"* ]] && ok=1; done
        [[ $ok == 1 ]] || die "$name: $rel is outside the folders mods may change; not installing anything"
        case "$rel" in *.dylib|*.so|*.dll|*.exe|*.asi|*.bundle|*.app/*) die "$name: $rel is native code; not installing anything" ;; esac
        grep -qxF "$GAME/$rel" "$MANIFEST" && die "$name: $rel would replace a file of the installed frameworks; not installing anything"
        inside_game "$GAME/$rel" || die "$name: a folder on the way to $rel leads out of the game folder"
    done < <(find "$src" -type f -print0)
    if find "$src" -type l | grep -q .; then say "Symlinks in $name are skipped."; fi

    mkdir -p "$STATE/mods"
    list="$STATE/mods/$name.txt"
    : > "$list"
    while IFS= read -r -d '' f; do
        rel="${f#"$src"/}"
        [[ "$(basename "$rel")" == .DS_Store ]] && continue
        mkdir -p "$(dirname "$GAME/$rel")"
        if [[ -L "$GAME/$rel" ]]; then rm -f "$GAME/$rel"; fi
        cp -f "$f" "$GAME/$rel"
        printf '%s\n' "$GAME/$rel" >> "$list"
    done < <(find "$src" -type f -print0)
    say "Installed $(wc -l < "$list" | tr -d ' ') files of $name. To remove: ./cp2077-mac.sh remove-mod \"$name\""
}

cmd_remove_mod() { # name given by add-mod
    local name="${1:?usage: ./cp2077-mac.sh remove-mod <name>}" list f
    [[ "$name" != */* && "$name" != .* ]] || die "invalid mod name: $name"
    list="$STATE/mods/$name.txt"
    [[ -f "$list" ]] || die "no mod named $name was installed with add-mod"
    while IFS= read -r f; do
        [[ -n "$f" && "$f" == "$GAME/"* && "$f" != *"/../"* ]] || continue
        if ! inside_game "$f"; then say "skipped $f: its folder leads out of the game folder"; continue; fi
        rm -f "$f"
    done < "$list"
    rm -f "$list"
    say "Removed $name."
}

cmd_play() {
    [[ -x "$GAME/launch_red4ext.sh" ]] || die "run ./cp2077-mac.sh install first"
    cmp -s "$GAME/launch_red4ext.sh" "$RED4EXT_DIR/scripts/launch_red4ext.sh" \
        || die "launch_red4ext.sh in the game folder is not the reviewed one (a mod may have replaced it); run install again"
    pgrep -q steam_osx || die "start Steam and sign in first"
    exec "$GAME/launch_red4ext.sh" "$@"
}

cmd_uninstall() {
    pgrep -qx Cyberpunk2077 && die "quit the game first"
    remove_installed
    find "$GAME/red4ext" "$CET_DATA" -type d -empty -delete 2>/dev/null || true
    if inside_game "$GAME/r6/scripts/zz_red4ext_plugins/x"; then rm -rf "$GAME/r6/scripts/zz_red4ext_plugins"; fi
    # The game's own script bundle again, as launch_red4ext.sh leaves it when the game exits.
    if [[ -f "$GAME/r6/cache/final.redscripts.bk" ]] && inside_game "$GAME/r6/cache/final.redscripts"; then
        cp -f "$GAME/r6/cache/final.redscripts.bk" "$GAME/r6/cache/final.redscripts"
    fi
    say "Removed the installed files. Your own mods, CET's settings and mods, and AudioXL's sounds folder stay."
    if [[ -f "$BACKUP" ]]; then
        if [[ -f "$EXE_HASH" && "$(shasum -a 256 "$BACKUP" | cut -d' ' -f1)" == "$(cat "$EXE_HASH")" ]]; then
            # A new file renamed over the old one: overwriting a signed executable in place can get it killed at launch.
            cp -p "$BACKUP" "$BIN.restoring" && mv -f "$BIN.restoring" "$BIN" && rm -f "$BACKUP" "$EXE_HASH"
            say "Restored the original game executable."
        else
            say "Cyberpunk2077.orig is not the file install backed up: use Steam > Cyberpunk 2077 > Properties >"
            say "Installed Files > Verify integrity to restore the executable."
        fi
    fi
}

case "${1:-}" in
doctor) cmd_doctor ;;
build) cmd_build ;;
install) cmd_install ;;
play) shift; cmd_play "$@" ;;
add-mod) cmd_add_mod "${2:-}" ;;
remove-mod) cmd_remove_mod "${2:-}" ;;
uninstall) cmd_uninstall ;;
*) sed -n '2,15p' "$0"; exit 2 ;;
esac
