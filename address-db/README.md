# Address database for Cyberpunk 2077 2.3.1 (macOS, UUID A6656ADC-FBE2-36A4-9B9D-B4A9DE645089)

`cyberpunk2077_addresses.json` is the database from RED4ext.SDK-macos (jackmaxwil, commit f657158c) plus 61 addresses
for Codeware, found by static analysis of the game executable. RED4ext uses only the entries with `"verified": true`.
`cp2077-mac.sh` installs this file into `red4ext/bin/x64/`.

What changed (`merge.log` has the full list):

- **10 corrections.** These hashes were already in the database, but unverified and with wrong offsets: Entity
  Initialize/Attach/Detach/Uninitialize/Dispose/Assemble, EntityBuilder_ExtractComponentsJob,
  IPlacedComponent_SetTransform, MorphTargetManager_ApplyMorphTarget, ScriptValidator_Validate. Without them RED4ext
  refuses to load Codeware.
- **51 new entries.** Without them the matching Codeware functions are switched off.

How they were checked:

- **Source.** Every address comes from `ports/Codeware/macos/addresses_proposed.json` with its evidence: strings,
  native function registration, vtable slots, the call graph. Details: `_work/codeware/re_*.md` and `notes.md`.
- **Function starts.** All 59 code addresses match a function start from `LC_FUNCTION_STARTS`. The two global
  addresses are in `__DATA`.
- **By hand.** Some addresses were rechecked by hand in the disassembly, including that SpawnEntity, GetLayer,
  ForgetResource and GetEnvironmentDefinition return their result through x8. Nothing was run.
- **Medium confidence: 5 entries.** CGameFramework_InitializeGameInstance, InkWidget_TriggerEvent,
  PlayerSystem_OnPlayerSpawned, QuestsSystem_CreateContext and VehicleController_ToggleAuxLights. If something breaks
  in game when a session starts or the player appears, start with the first and the third.

Side effects:

- **RED4ext.** The corrected ScriptValidator_Validate turns on RED4ext's own script validation hook, which writes script
  errors to its log. The `ScriptReport` layout it reads is the same on macOS: the game builds the report on the stack at
  0x103D9622C, the flag is at +0x08, the errors at +0x20 and the limit at +0x28.
- **ArchiveXL and CET.** They also carry some of these hashes, but their code does not call these functions on macOS,
  so their behaviour does not change.

To rebuild the database:

```bash
python3 -I _work/addrdb/merge.py src/jackmaxwil_RED4ext.SDK-macos/cyberpunk2077_addresses.json ports/Codeware/macos/addresses_proposed.json address-db/cyberpunk2077_addresses.json
```
