# Cyberpunk 2077 mods on macOS (Apple silicon)

Native macOS builds of the Cyberpunk 2077 modding frameworks (RED4ext, Cyber Engine Tweaks, TweakXL, ArchiveXL,
Codeware, AudioXL and redscript), built from reviewed sources by one script, so that mods like
[Titanfall BT-7274 (Quest and Companion)](https://www.nexusmods.com/cyberpunk2077/mods/35041) run on a Mac.

**Full guide: [MACOS_GUIDE.md](MACOS_GUIDE.md)**: requirements, build, install, mods, troubleshooting, and what was
changed in each framework and why.

**Status:** tested on Cyberpunk 2077 **2.3.1** (Steam), macOS 26.5, Apple silicon. The BT-7274 quest was played with
all frameworks loaded: BT is called and spawned, the link shard is taken, and the fights, saving and BT's keys work.

## Components

| Component | macOS source |
| --- | --- |
| RED4ext | jackmaxwil's macOS port plus fixes: [andr0s/RED4ext-macos](https://github.com/andr0s/RED4ext-macos) (`macos-fixes`) |
| RED4ext.SDK | jackmaxwil's macOS fork plus fixes: [andr0s/RED4ext.SDK-macos](https://github.com/andr0s/RED4ext.SDK-macos) (`macos-fixes`) |
| redscript | ships with the macOS game (`engine/tools/scc`, by CD PROJEKT RED) |
| TweakXL, ArchiveXL | jackmaxwil's macOS ports, unchanged |
| Codeware | ported from psiberx/cp2077-codeware 1.20.5: [andr0s/cp2077-codeware](https://github.com/andr0s/cp2077-codeware) (`macos`) |
| Cyber Engine Tweaks | ported as a RED4ext plugin (Metal overlay, NSEvent input, LuaJIT without JIT): [andr0s/CyberEngineTweaks](https://github.com/andr0s/CyberEngineTweaks) (`macos`) |
| AudioXL | ported; plays through AVAudioEngine, because the macOS game has no Wwise "Audio Input" plugin: [andr0s/cp2077-audio-xl](https://github.com/andr0s/cp2077-audio-xl) (`macos`) |
| Address database | the SDK's database plus 61 addresses for Codeware: [address-db/](address-db/README.md) |
| AMM, Mod Settings | not needed by BT-7274; not ported |

## Quick start

You need the Command Line Tools (`xcode-select --install`) and CMake 3.25 or newer (`brew install cmake`). Clone this
repository, clone the source repositories next to it as listed in
[section 4.2 of the guide](MACOS_GUIDE.md#42-get-the-sources), then:

```bash
./cp2077-mac.sh doctor
```

```bash
./cp2077-mac.sh build
```

```bash
./cp2077-mac.sh install
```

```bash
./cp2077-mac.sh add-mod ~/Downloads/BT7274
```

```bash
./cp2077-mac.sh play
```

`doctor` checks the Mac and changes nothing. `install` backs up the game executable and re-signs it so that RED4ext can
load. `play` starts the game with mods (Steam must be running); Steam's own Play button starts it without them.
`./cp2077-mac.sh uninstall` removes exactly what `install` added and restores the original executable. Back up your
saves before the first start.

## Security

All sources were read before building. No downloader, telemetry, autostart or obfuscated code was found. The issues
found in the ported code are fixed in the ports, and the fixes upstream projects can take were sent to them as pull
requests.
Details: [section 11 of the guide](MACOS_GUIDE.md#11-security-review).

What remains by design: while RED4ext is installed, the game executable is re-signed ad hoc with entitlements that
allow library injection; RED4ext plugins are native code with your privileges, so install only plugins you trust.

## Upstream

Pull requests and issues sent to the upstream projects, and the fixes that were not sent because someone else had
already proposed them: [Appendix C of the guide](MACOS_GUIDE.md#appendix-c-upstream-pull-requests).

## Repository layout

- `cp2077-mac.sh`: build, install, play, add and remove mods, uninstall.
- `MACOS_GUIDE.md`: the full guide.
- `address-db/`: the address database and how it was built.
- `_work/`: analysis notes and helper scripts.
- `ports/`, `src/`: the source repositories (cloned separately, see the guide).

## Credits

RED4ext and RED4ext.SDK: WopsS and contributors; macOS ports by jackmaxwil. Cyber Engine Tweaks: yamashi and the CET
team. TweakXL, ArchiveXL, Codeware and RedLib: psiberx; TweakXL and ArchiveXL macOS ports by jackmaxwil. AudioXL:
DigitalVixen. redscript and Cyberpunk 2077: CD PROJEKT RED. Titanfall BT-7274: its author on Nexus Mods. All projects
are MIT licensed; see [section 14 of the guide](MACOS_GUIDE.md#14-credits-and-licenses).

macOS ports of CET, Codeware and AudioXL, the fixes, the tooling and the guide: **andr0s**
(<https://github.com/andr0s>).
