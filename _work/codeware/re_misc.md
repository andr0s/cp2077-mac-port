# Codeware misc functions: macOS 2.3.1 arm64 (static RE)

Binary: `Cyberpunk2077` (UUID A6656ADC-FBE2-36A4-9B9D-B4A9DE645089), image base 0x100000000. Static analysis only, with
`re.sh` / `dis.sh` / `llvm-objdump`. Every address below is a function start per `re.sh func` (LC_FUNCTION_STARTS).
None of these hashes is in `cyberpunk2077_addresses.json` yet. "vt+X" is an offset from the C++ vtable address point.

## Summary

| # | Codeware name (hash) | mac address | DB form | arm64 sig vs Codeware decl | hook notes | conf. |
|---|---|---|---|---|---|---|
| 1 | VehicleController_ToggleAuxLights (1883248814) | 0x1012E6F44 | 1:0x12E6F44 | MATCH `void(ctrl x0, bool w1, ELightType w2)` | 0x164 B, prologue clean | medium-high |
| 2 | VehicleGarageComponent_LoadSummonSetup (1159928678) | 0x10124FE1C | 1:0x124FE1C | MATCH `void(comp x0)` | 0x718 B, clean | high |
| 3 | VehicleSystem_ToggleGarageVehicle (3027836941) | 0x101369A5C | 1:0x1369A5C | MATCH `bool(sys x0, const GarageVehicleID* x1, bool w2)` | 0x8C B, clean; gameVehicleSystem vt+0x2A0 | high |
| 4a | RenderProxy_SetVisibility (1790971865) | 0x1016A78E4 | 1:0x16A78E4 | MATCH `void(proxy x0, bool w1)` | 0x40 B, clean | medium-high |
| 4b | RenderProxy_UpdateVisibility (2124419179) | not determined (2 candidates) | - | see item 4 | - | low |
| 4c | RenderProxy_SetHighlightParams (1093803822) | 0x1016A83B8 | 1:0x16A83B8 | args MATCH; returns void (decl uint8) | 0xCC B, clean | high |
| 4d | RenderProxy_SetScanningState (2838044016) | 0x1016A8484 | 1:0x16A8484 | args MATCH; returns void (decl uint8) | 0x48 B, `b.eq` at +8 (PC-rel in first 4) | high |
| 4e | RenderProxy_SetMaterialOverride (429336746) | 0x1016A85A8 | 1:0x16A85A8 | MATCH `void(proxy, const RenderDataPtr* x1, const RenderDataPtr* x2, bool w3)` | 0x60 B, clean | high |
| 5 | VisionModeSystem_SetBraindanceMode (1070077985) | 0x1015A0544 | 1:0x15A0544 | MATCH `void(gameVisionModeSystem* x0, u32 w1)` | 12 B, `b` at +8: not hookable (call only) | medium |
| 6a | RuntimeSystemWeather_SetWeatherByName (2334794340) | 0x100D5C384 | 1:0xD5C384 | MATCH `bool(sys, CName x1, CName x2, float s0, u32 w3, u32 w4)` | 0x244 B, clean | high |
| 6b | RuntimeSystemWeather_SetWeatherByIndex (1821516328) | 0x100D597A8 | 1:0xD597A8 | **MISMATCH**: a5 is u32 (w3), not float; returns void | 0x79C B, clean | high |
| 6c | RuntimeSystemWeather_GetEnvironmentDefinition (1471948702) | 0x100D5AEE0 | 1:0xD5AEE0 | **MISMATCH**: Handle returned via x8 | 0x4C B, clean | high |
| 6d | RuntimeSystemWeather_GetWeatherState (2679119876) | 0x100D5AA60 | 1:0xD5AA60 | **MISMATCH**: Handle returned via x8 | 0x1C B, tail `b` at +0x18 | high |
| 7 | WorkspotSystem_GetWorkspotInstance (2234523611) | not found (inlined) | - | - | - | high (absence) |
| 8 | PhysicsTraceResult_GetHitObject (2394822845) | 0x104092830 | 1:0x4092830 | MATCH as declared (by-value Handle -> x8; w0 proxyID, w1 index) | 0x104 B, clean | high |
| 9 | VisualController_LoadDependencies (2912168594) | 0x100C8B630 | 1:0xC8B630 | **MISMATCH**: `JobHandle(ctrl x0, u8 w1)`, JobHandle via x8 | 0x530 B, clean | high |
| 10a | MeshComponent::LoadResource, Codeware RawVFunc 0x260 | entMeshComponent vt+0x268 -> 0x100C685FC | - | `(comp x0, JobQueue& x1)`, returns nothing useful | virtual; use offset 0x268 | high |
| 10b | MeshComponent::RefreshAppearance, Codeware RawVFunc 0x280 | entMeshComponent vt+0x288 -> 0x100C699AC | - | `void(comp x0)` | virtual; use offset 0x288 | high |

"Clean" means a function of 16 bytes or more with no PC-relative instruction in its first 4 instructions.

Layout differences that need a macOS override (details in items 6, 7 and 9):

| field | Windows | macOS |
|---|---|---|
| workWorkspotSystem SpinLock | 0x50 | **0x4C** |
| workWorkspotSystem InstanceMap | 0x70 | **0x68** |

Every other requested offset is the same on macOS.

---

## 1. VehicleController_ToggleAuxLights -> 0x1012E6F44 (1:0x12E6F44)

**Evidence**
- The vehicleController natives are registered in 0x1012E45E0 (strings "SetLightColor" ... "ToggleLights", "AreDefaultLightsToggled").
  - The handlers (x4 of the CClassFunction ctor) are, in order: 0x1012E76B4, 0x1012E78B0, 0x1012E7A18, 0x1012E7BB4, 0x1012E7D1C, 0x1012E7F04, 0x1012E806C (ToggleLights) and 0x1012E81EC.
- The ToggleLights handler 0x1012E806C reads 5 params (bool on, light type with default 0x2F, float, 8-byte, bool). It then calls `0x1012E6E28(ctrl, w1=on, w2=type)` (0x1012E81D4).
- 0x1012E6E28 is the script-level ToggleLights:
  - If `type & Head` (bit0) is set, it runs the headlight logic (0x1012B3604, 0x1012E5B18). It then calls `0x1012E6F44(ctrl, on, type & ~1)` (0x1012E6F28).
  - Otherwise it tail-calls `b 0x1012E6F44` (0x1012E6E60).
- **0x1012E6F44** is the common light-toggle sink.
  - If `w1` is set it calls `lightComp->Enable(mask=w2)` 0x1012EB304 (0x1012E6FA4/0x1012E7054). Otherwise it calls `Disable(~mask)` 0x1012EB2B8 (`mvn w1,w20` at 0x1012E6FEC).
  - The light component Handle is at ctrl+0x98 and is IsA-checked.
  - 0x1012EB304 has no other callers.
- 0x1012E6F44 is also called directly by 0x1012E6044 (apply light state) with types 1 and 0xE (0x1012E60B0/0x1012E60C0).
- Codeware history (commit 1bf1dd22, "Add vehicle lights callback") moved the hook from `VehicleController_ToggleLights` (3642563117) to `ToggleAuxLights`. So Windows has two distinct functions with this same signature: the outer ToggleLights = 0x1012E6E28, and the inner sink = 0x1012E6F44.

**arm64 signature**
- Body uses x19=x0 (ctrl), `cbz w1` (enable) and x20=x2 (mask).
- MATCH `void(vehicleController*, bool, vehicleELightType)`.

**Hookability**
- Size 0x164. The first 4 instructions are `sub sp / stp / stp / stp`.
- It has a 4-byte alias thunk at 0x1012E6F40 (`b 0x1012E6F44`, no refs). The outer ToggleLights reaches it both by `b` (tail call) and by `bl`.
- A trampoline at 0x1012E6F44 sees all of these entries.

**Confidence**
- Medium-high. The call graph, the signature and the Codeware commit history agree. There is no name string.

## 2. VehicleGarageComponent_LoadSummonSetup -> 0x10124FE1C (1:0x124FE1C)

**Evidence**
- Class `vehicleGarageComponent`: registration 0x10125A478 (size 0x3B0), ConstructCls 0x10125A57C -> ctor 0x10124FA1C, C++ vtable 0x106F115A8.
- 0x10124FE1C reads TweakDB flats of the record id at `this+0x328`:
  - ".simpleSpawn" -> +0x334, ".forceDespawnOnResummon" -> +0x335, ".despawnDistance" (squared) -> +0x330;
  - also ".spawnDistanceMin/Max", ".summonDistanceMin/Max", ".summonPointsOnLaneFilterRange", ..., ".repairCostMultiplier", ".replacementFreeMsgLocKey".
  - That is the summon setup record.
- Callers:
  - OnAttach (vt+0x188 = 0x101250C24) at 0x101250C64.
  - VehicleSystem 0x10136D578 at 0x10136D698, as `LoadSummonSetup([[vehSys+0x1D0]])`. The garage component pointer at VehicleSystem+0x1D0 is the same one item 3 uses.

**arm64 signature**
- Only x0 is used (x19=x0). MATCH `void(vehicleGarageComponent*)`.

**Hookability and confidence**
- Prologue `stp d15,d14,[sp,#-0xa0]!`, clean. Codeware only calls it.
- Confidence: high.

## 3. VehicleSystem_ToggleGarageVehicle -> 0x101369A5C (1:0x1369A5C)

**Evidence**
- VehicleSystem natives are registered in 0x1013650E8. "EnablePlayerVehicle" is the 8th handler: 0x10136965C.
- The handler reads `(String vehicle, Bool enable=1, Bool despawnIfDisabling=1)`, builds a GarageVehicleID (16 B, from 0x10124F768 into x0:x1), and then runs inline:
  `LockShared(this+0x20A50); if ([this+0x1DC]) ok = 0x101251EB4([[this+0x1D0]], &id, enable); if (ok) 0x1012D9B80(this+0xA0); UnlockShared`.
- **0x101369A5C** is the out-of-line copy of exactly this logic, and returns `w0` (bool, 1 on change).
  - It is gameVehicleSystem **vt+0x2A0** (vtable 0x106F23FC8, slot data 0x106F24268, ctor 0x101365EEC). That corresponds to MSVC 0x298.
  - It is also called by 0x10125363C.

**arm64 signature**
- x0=sys, x1=`const GarageVehicleID*` (passed straight to 0x101251EB4), w2=enable, bool in w0. MATCH.

**Inlining note**
- The EnablePlayerVehicle script native does not call this function; it inlines it. Codeware only calls it, so this is harmless.

**Hookability and confidence**
- Size 0x8C, clean.
- Confidence: high.

## 4. RenderProxy (component-side `RenderProxyHandle`)

**Class identity**
- The class is `RenderProxyHandle`; the symbols `red::DynArray<red::SharedStorage<RenderProxyHandle,...>>` confirm the name.
- Vtable 0x106F4C090 holds only D1 0x1016A71D0 and D0 0x1016A71D4. That matches Codeware's `struct RenderProxy { virtual ~RenderProxy(); }`.
- Ctor 0x1016A7038/0x1016A708C:
  - +0x08 = 0;
  - +0x10 = render proxy (AddRef 0x1016A38C4);
  - +0x18 = 0;
  - +0x1F..+0x22 = 0;
  - flags +0x23 = `(flags & 0x80) | 6` (0x1016A706C-0x1016A7078).

**Byte fields**
- +0x21 = highlight id, +0x22 = scanning state, +0x23 = flags.
- Flag bits:
  - bit0 = attached to the render scene (set at 0x1016A7418 by attach 0x1016A7344, cleared with `& 0xF6` at 0x1016A72C4 by detach 0x1016A71E8);
  - bit1 = visibility A;
  - bit2 = visibility B;
  - bit3 = hidden-state latch.

**Flags @+0x23 and IsVisible: MATCH**
- The refresh 0x1016A7924 computes `and w21,w8,#6; cmp w21,#6` (0x1016A7940-0x1016A7948).
- It then calls `renderProxy->vt[+0x30](visible)`.

**Owner offsets (useful for callers)**
- The handle pointer lives in the component's SharedPtr:

| component | handle offset | instruction |
|---|---|---|
| entMeshComponent | +0x140 | 0x100C67394 |
| entSkinnedMeshComponent | +0x1D8 | 0x100C7EDB8 |
| entMorphTargetSkinnedMeshComponent | +0x1E0 | 0x100C701F4 |

**Codeware usage**
- None of these RenderProxy functions is called anywhere in Codeware.
- They were added in commit b941577e and never referenced in git history. ComponentWrapper.cpp does not use them.

**4a SetVisibility -> 0x1016A78E4**
- It does `flags = (flags & ~2) | (w1 ? 2 : 0)`. If `(flags&6)==6` changed, it tail-calls the refresh 0x1016A7924.
- 91 callers. Example: the component visibility helper 0x100C5E144 does `SetVisibility([comp+0x140], isEnabled && !IsHidden())`.
- Signature `(x0 proxy, w1 bool)`: MATCH.
- Hook notes: size 0x40; the first 4 instructions are not PC-relative (`b.eq` at +0x34, `b` at +0x38).

**4b UpdateVisibility: not determined**
- Two mac functions fit, and neither has a caller that names it:
  - **0x1016A7B80**: the same body as 4a but for bit2 (`and #0xFFFFFFFB` at 0x1016A7B9C). Signature `void(proxy, bool)`. It has 0 callers and 0 refs. It is the only writer of bit2 apart from the ctor.
  - **0x1016A7924**: the refresh. It overwrites w1 before reading it (0x1016A7948), so it is effectively `void(proxy)`. It is reached only by tail calls from 4a and 0x1016A7B80.
- Decide from the Windows binary before enabling.

**4c SetHighlightParams -> 0x1016A83B8**
- It reads HighlightParams as `[x1+0]` seeThroughWalls, `[x1+1]` patternType, `[x1+2]` fillIndex, `[x1+3]` outlineIndex, `[x1+4]` float opacity, `[x1+8]` forced.
- If both fill and outline are 0, it releases the highlight id at +0x21. Otherwise it allocates one (0x1016E5234) into +0x21.
- When `(flags&7)==7` it sends the command through 0x1016E43D0.
- Caller: the MeshComponent RenderHighlightEvent handler 0x100C67370 (member fn registered at 0x100C66A1C), with a 12-byte params struct built by 0x100CF3490. 6 callers in total.
- Args MATCH. **Return**: no `w0` is materialized on every path (some paths return with x0 = proxy pointer), so on mac it returns void. Codeware declares uint8. That is harmless only if the result is ignored.

**4d SetScanningState -> 0x1016A8484**
- `if ([x0+0x22] != w1) { [x0+0x22] = w1; if ((flags&7)==7) tail 0x1016E45C8 }`.
- Caller: the MeshComponent RenderScanEvent handler 0x100C673C8 (`b 0x1016A8484` with `w1 = event+0x40`).
- Args MATCH. Returns void on mac (decl uint8).
- **Hook risk**: `b.eq` is the 3rd instruction.

**4e SetMaterialOverride -> 0x1016A85A8**
- It forwards `(renderCmd vt+0x330)(cmd, proxy+0x10, x1, x2, w3)`.
- Caller: the MeshComponent RenderOverlayEvent handler 0x100C673DC. It creates the RenderDataPtr via 0x102F254A4 into sp+8, sets a null RenderDataPtr at sp, and passes `w3 = byte comp+0x1D0`. 4 callers in total.
- MATCH `void(RenderProxy*, const RenderDataPtr&, const RenderDataPtr&, bool)`.

**Event handler identity**
- The RenderHighlight/RenderScan/RenderOverlay handlers are the `RegisterEventConnector<ent::MeshComponent, ent::Render*Event>` closures. Their s_vt objects are 0x1073AC710 / 0x1073AC738 / 0x1073AC760.
- The closures are set in 0x100C6617C: function pointers 0x100C67370, 0x100C673C8 and 0x100C673DC.

## 5. VisionModeSystem_SetBraindanceMode -> 0x1015A0544 (1:0x15A0544)

**Evidence**
- gameVisionModeSystem:
  - registration 0x1015A4A9C, size 0x2430;
  - ctor 0x10159E948;
  - vtable 0x106F38688.
- 0x1015A0544 is **vt+0x240** (slot data 0x106F388C8), which corresponds to MSVC 0x238. Its body is:
  `str w1,[x0,#0x2378]; ldr x0,[x0,#0x2400]; b 0x1015A842C`.
- Field +0x2378 is serialized as "currentFocusMode" (0x10159FCB8/0x1015A007C in save/load).
- The helper 0x1015A842C does the following:
  - stores `byte49 = (mode != 1)`;
  - sets render-scene flag bit1 = `(mode == 1)` via 0x1016E4264;
  - if focus mode is active, pushes or pops the regular focus environment layer (0x1015A838C).
- The only runtime caller found is scene-system listener vfunc 0x1019C9178 (listener created at 0x101B362FC, stored at scene+0xFD8). It handles entries of section type 0x11:
  - On the first entry it sends `scnRewindableSectionEvent{active=1}` to the player and calls `visionSys->vt[+0x240](1)` (0x1019C91EC-0x1019C91F4).
  - On the last removal it sends active=0 and calls `vt[+0x240](0)` (0x1019C9388).
  - Rewindable scene sections are the braindance playback mechanism, so mode 1 = braindance.

**arm64 signature**
- x0 = gameVisionModeSystem* (the object from `GetSystem(gameIVisionModeSystem)`), w1 = mode. MATCH `void(void*, uint32)`.

**Hookability**
- 12 bytes; the 3rd instruction is a PC-relative `b`. It can be called but not hooked.
- Alternatively, call it virtually through vt+0x240.

**Confidence**
- Medium. The call-site semantics (rewindable section -> 1/0) are solid. The name mapping is inferred.

## 6. RuntimeSystemWeather (class worldRuntimeSystemWeather)

**Class**
- Registration 0x100D5ECA0 (size 0x230). ConstructCls -> ctor 0x100D59590. C++ vtable 0x106EE0670, secondary vtable at +0x48.
- Weather "state block" (0x28 bytes): u32 index +0, CName source +8, u32 priority +0x10, floats +0x14..+0x1C, u32 a5 +0x20.
  - Current block at +0x90, previous block at +0xB8.
  - Override: byte +0x84 selects the index at +0x88 instead of +0x90.

### 6a SetWeatherByName -> 0x100D5C384

**Body**
1. Gets the current state (`csel 0x88/0x90`). If `state->name == x1 && [sys+0x98] == x2`, it returns false (0x100D5C3EC-0x100D5C3FC).
2. Otherwise it loops over the states (count 0x100D5A584, GetStateByIndex 0x100D5BF98).
3. On a name match it calls `SetWeatherByIndex(sys, i, x2, s0, x3, x4)` (0x100D5C440-0x100D5C458) and returns `w26&1` (true).

**Callers**
- Quest node questPlayEnv_SetWeather 0x102E106AC calls `(sys=[ctx+0x1C8], name, source=[node+0x38], blend=[node+0x40], w3=0, prio=[node+0x48])`, then `SetCycleWeather(sys,0)`. That is exactly Codeware's flow.
- Helper 0x102E10664.

**Signature**
- MATCH `bool(sys, CName weather x1, CName source x2, float blend s0, u32 a5 w3, u32 priority w4)`.

### 6b SetWeatherByIndex -> 0x100D597A8

**Body**
- Copies the current block into the previous block (0x100D597E8 `ldr w1,[x24,#0xb8]!`, 0x100D5987C/0x100D59880).
- Then sets:
  - +0x98 = x2 (0x100D5988C);
  - +0x90 = `w1 < count ? w1 : 0` (0x100D598A0);
  - +0xB0 = w3 (0x100D598A4);
  - +0xA0 = `min(w4,7)` (0x100D598A8-0x100D598B4).
- Blend s0 goes to +0xA8/+0xAC.

**Callers**
- Photo mode 0x10097AEF0: `(w1=i, x2=src, s0=0, w3=2, w4=6)`.
- RestoreState 0x100D5E558.
- Resets 0x100D5DEAC and 0x100D5E004: `(0, defaultSource, 0.0/-1.0, w3=1, w4=0)`.

**Signature: MISMATCH (two problems)**
1. The 5th parameter is an **integer in w3**, not a float. With the Windows declaration, clang puts a5 in s1, priority in w3, and leaves w4 (priority) as garbage. On Windows x64 both sit in the same stack slot, so `float 0.0` there behaves like int 0.
2. It **returns void**. No `w0` is set at the 0x100D598C4 epilogue. Codeware's `if (!SetWeatherByIndex(...)) return false;` therefore reads garbage.

Use `void(*)(worldRuntimeSystemWeather*, uint32_t index, CName source, float blend, uint32_t a5, uint32_t priority)` on mac.

### 6c GetEnvironmentDefinition -> 0x100D5AEE0

**Body**
- `x19=x8; env=[sys+0x60]; if (!env) {[x8]=0;} else { h = *(env+0x150) (0x10338D9A0 = add x0,#0x150); [x8]=h; ++ref }`.
- worldEnvironmentDefinition.weatherStates is at 0x188 per the RTTI dump. GetStateByIndex 0x100D5BF98 reads `def+0x188/+0x194`, which confirms the handle type.

**Callers**
- worldWeatherAreaNotifier code 0x1045D97C4: `x0=[scene+0x280]; x8=sp+8; bl`. Also 0x1045F55DC.

**Signature**
- **MISMATCH**: mac returns `Handle<worldEnvironmentDefinition>` through **x8**; only x0 is an argument.
- Use `Red::Handle<Red::worldEnvironmentDefinition> (*)(Red::worldRuntimeSystemWeather*)`.
- The current declaration passes `&out` in x1 (ignored), and the callee writes through garbage x8.

### 6d GetWeatherState -> 0x100D5AA60

**Body**
- `idx = byte[+0x84] ? [+0x88] : [+0x90]; b GetStateByIndex 0x100D5BF98`. x8 is passed through as sret.
- GetStateByIndex returns `Handle<worldWeatherState>` from `def->weatherStates[idx]` after an IsA check against worldWeatherState (0x10330A67C).

**Caller**
- questWeather_ConditionType 0x102C2A248 (`x8=sp`), which compares `state->name` with the condition name.

**Signature**
- **MISMATCH** (sret in x8, same fix as 6c).

**Hookability**
- 28 bytes, no prologue, 7th instruction is a tail `b`. Calling it is fine.

### Offsets (all MATCH Windows)

**Weather system fields**

| field | offset | evidence |
|---|---|---|
| CycleWeather | +0x8C | ctor `strb w22(=1)` 0x100D595DC; setter 0x100D5AED8 `strb w1,[x0,#0x8c]` (quest "Resume Weather" -> 1, SetWeather -> 0); RestoreState 0x100D5E6C0 |
| CurrentStateIndex | +0x90 | 0x100D598A0 (write), 0x100D5AA60 (read) |
| CurrentSource | +0x98 | 0x100D5988C (write), 0x100D5C3F4 (compare) |
| PreviouseStateIndex | +0xB8 | 0x100D597E8 / 0x100D59880; GetPreviousState 0x100D5C1D8 `ldr w1,[x0,#0xb8]; b 0x100D5BF98` |

**RuntimeScene weather handle +0x280**
- GetWeatherSystem native 0x101F4337C: `[gi+0x78]` -> scene (0x101F43468), `ldr x8,[scene,#0x280]` (0x101F4346C).
- worldWeatherAreaNotifier 0x1045D97C4: `ldr x0,[x2,#0x280]`.
- Photo mode 0x10097AEC4: `[x8+0x280]` -> weather system.

**WeatherScriptInterface system pointer +0x40**
- Size 0x50.
- The weather ctor allocates 0x50 bytes and calls ctor 0x100D614B8 (`stp x19,xzr,[x0,#0x40]`, x19 = system). The handle is kept at weather+0x198 and returned by GetWeatherSystem.
- The natives GetRain* 0x100D61768/0x100D617A0/0x100D617D8/0x100D61810 all do `ldr x0,[x0,#0x40]` and then call weather methods.

**Native names**
- The game's WeatherSystem natives are only RegisterWeatherListener, UnregisterWeatherListener, GetRainMoisture, GetRainPuddles, GetRainIntensity and GetRainIntensityType.
- SetWeather, GetWeatherState and the others are Codeware additions, so they are not usable as entry points.

## 7. WorkspotSystem_GetWorkspotInstance: not found (inlined); offsets differ on mac

**Class**
- workWorkspotSystem: registration 0x1015FC06C (size 0xAE0, the same as Windows), ConstructCls -> ctor 0x1015F4134, vtable 0x106F3E5A0.

**Layout on mac (tail-padding reuse after worldIRuntimeSystem)**
- +0x44 owner thread id, +0x48 u16, +0x4A u8, +0x4B exclusive-lock byte. This is a recursive lock; ctor 0x1015F4158/0x1015F415C.
- **+0x4C SharedSpinLock**: ctor `strb wzr,[x19,#0x4c]` at 0x1015F4164.
- +0x50, +0x58, +0x60.
- **+0x68 HashMap**: ctor zeroes +0x68..+0x87 (0x1015F4180/0x1015F4184), stores `0xFFFFFFFF` at +0x88 (0x1015F418C) and the allocator at +0x90 (0x1015F4194).

**Map usage**
- Lookup 0x1015FA160 (`x1` = entity hash):
  - `add x19,x0,#0x4c; bl LockShared 0x100002098` (0x1015FA178);
  - `ldr w8,[x21,#0x70]` size, `[#0x74]` capacity, `[#0x68]` index table, `[#0x78]` nodes, `[#0x84]` stride;
  - `node+4 == (u32)key`, `node+8 == key`, value at `node+0x10` (0x1015FA1E0);
  - unlock `ldaddalb -1` at 0x1015FA1F0.
- Insert 0x1015F5428: exclusive lock (0x1000020C0) on +0x4C, then HashMap insert 0x100D38F8C on +0x68.
- Remove: 0x1015F5A58.

**Absence of a standalone GetWorkspotInstance**
- A whole-binary scan found every LockShared(+0x4C) followed by a +0x68/+0x70 map read: 24 sites (21 in 0x1015F5..0x1015FA, plus 0x101C8A780, 0x101C8AB98, 0x101C8B00C).
- None of them returns the node value directly. All are inlined lookups, or wrappers that call into the instance.
- No function takes `const EntityID&` (`ldr [x1]`) followed by this lookup.

**Required macOS overrides**
- `SpinLock = 0x4C` (Windows 0x50) and `InstanceMap = 0x68` (Windows 0x70).

**Extra finding (lookup hash)**
- The game hashes this map with **`(uint32)key`**:
  - insert 0x100D38F8C: `ldr x24,[x1]; udiv w10,w24,w9`, node hashedKey compared with w24;
  - lookup 0x1015FA190/0x1015FA1CC: the same.
- The SDK `HashMapHash<uint64_t>` is FNV1a32 (SDK commit c409cd71, "Fix HashMap for uint64 keys"). So `instanceMap.Get(aEntityID.hash)` computes the wrong bucket and hashedKey and will not find instances.
- Use a custom hasher (`return (uint32_t)key;`) or a manual walk. The Windows binary probably behaves the same; that was not checked.

## 8. PhysicsTraceResult_GetHitObject -> 0x104092830 (1:0x4092830)

**Evidence**
- Global physics proxy manager `[0x1090DF0B0]`. Lookup `0x104055F44(mgr, proxyID, x8=SharedPtr<proxy>)`.
- 0x104092830:
  - `x19=x8`;
  - looks up the proxy for `w0`;
  - `if (w1 < [proxy+0x28])`, locks the WeakHandle at `proxy + 8 + w1*16` (casal on the strong count);
  - stores the `Handle<ISerializable>` to `[x8]`, otherwise stores zero.
- Its sibling 0x10409274C returns the WeakHandle instead.
- 41 callers, mostly in pairs with index 0 and 1. Example: 0x1009CDF5C tries index 0 then 1 and IsA-checks, like Codeware's GetHitEntity.
- The same user-data list (count +0x20 in the `proxy+8` view) is read by the post-filter lambda `physics::FilterUserDataType<CarBaseObject,BikeBaseObject>::GetPostFilterFunction` (0x101318730 via 0x1040929C0).

**TraceResult ResultID @+0x48: MATCH**
- The mac TraceResult size is 0x60.
- 0x1009CC5FC-0x1009CC61C iterates query results with stride 0x60 and does `ldr w0,[x22,#0x48]` -> 0x1009CDF5C (entity from proxy).
- More `ldr w0,[x,#0x48]` -> proxy-function sites: 0x100C004CC, 0x1041C2DA4, 0x101CAFA78.

**arm64 signature**
- w0 = proxyID, w1 = index, x8 = sret Handle. Handle is non-trivial, so Codeware's existing by-value declaration `Handle<ISerializable>(*)(uint32,uint32)` puts the result in x8 automatically. MATCH.

**Hookability and confidence**
- Size 0x104, clean (`adrp` is the 8th instruction).
- Confidence: high.

## 9. VisualController_LoadDependencies -> 0x100C8B630 (1:0xC8B630)

**Evidence**
- VisualControllerComponent: registration 0x100C8D6D0, ConstructCls/ctor 0x100C8D7D4, vtable 0x106ED28C8.
- CreateController 0x100C8CDC8 (component, u8 priority):
  1. if `[comp+0xD8]==0`, allocates 0x80 bytes and initializes the VisualController;
  2. stores the SharedPtr in comp+0xD8 (0x100C8CEC4-0x100C8CED4);
  3. calls 0x100C8AFCC(ctrl, params). That function sets the dependencies, loads loose dependencies and the unk50/unk60 tokens, dispatches job "VisualControllerComponent_ScheduleDependencyStreaming" with lambda 0x100C8DB68, and stores the JobHandle at ctrl+0x70.
- The lambda 0x100C8DB68 locks `self` and calls **`0x100C8B630(ctrl, w1=priority, x8=&JobHandle)`** (0x100C8DBFC). If the job is done it sets status +0x78 = 1; otherwise it waits and then dispatches "OnStreamingDone".
- 0x100C8B630 does the following:
  - constructs the JobHandle at `[x8]` (0x1009D4824, 0x100C8B65C-0x100C8B668);
  - locks `ctrl+0x7C`;
  - for each dependency (+0x10 data, +0x1C count, stride 0x30) with a null token, calls `LoadAsync(&dep.mesh(+0x20), priority)` (0x1021C63A4) into `dep.token` (+0x00);
  - per mesh it creates "VisualControllerComponent_OnMeshLoaded" jobs.

**arm64 signature: MISMATCH**
- mac is `JobHandle LoadDependencies(VisualController* x0, uint8 w1)`, with the result via **x8**.
- The Windows `uintptr(ctrl, JobHandle& out, uint8)` is the MSVC member-sret form.
- Declare it on mac as `Red::JobHandle (*)(Red::VisualController*, uint8_t)`. Red::JobHandle has a destructor, so clang uses x8.
- The current declaration would put &out in x1 (read as the priority) and leave x8 as garbage.

**Hookability and confidence**
- Size 0x530, clean. It has a single game caller (the job lambda).
- Confidence: high.

**Offsets (all MATCH VisualController.hpp / Windows)**

| field | evidence |
|---|---|
| size 0x80 | alloc `mov w0,#0x80` at 0x100C8CE00 |
| self WeakPtr +0x00/+0x08 | written 0x100C8CEA4-0x100C8CEB8; locked 0x100C8B670 |
| dependencies +0x10 (count +0x1C, stride 0x30) | dep token +0, mesh RaRef +0x20 (0x100C8B790), appearanceName +0x28 (0x100C8B78C, 0x100C8B0B4-0x100C8B0C0) |
| looseDependencies +0x20 | ctor 0x100C8CE38; 0x100C8B1E4 |
| unk30..4F | zeroed 0x100C8CE68, 0x100C8B0D8 |
| unk50 / unk60 SharedPtr<ResourceToken> | 0x100C8B19C / 0x100C8B194 |
| job JobHandle +0x70 | ctor 0x100C8CE5C; assigned 0x100C8B328 |
| status +0x78 | 0x100C8CE74; `swpal` 0x100C8B014/0x100C8B040/0x100C8DC14 |
| lock SharedSpinLock +0x7C | ctor `strh wzr` 0x100C8CE78; lock 0x100C8B000/0x100C8B70C; unlock 0x100C8B01C/0x100C8B0E4 |
| VisualControllerComponent controller SharedPtr +0xD8 | ctor zero 0x100C8D850; read 0x100C8CDDC/0x100C8CF6C |

## 10. Mesh component vtable slots (Codeware RawVFunc 0x260 / 0x280) and MeshAppearance offsets

**Vtables and slots**
- Address points:
  - entMeshComponent 0x106ECE998 (ctor via ConstructCls 0x100C6A0C0);
  - entSkinnedMeshComponent 0x106ED0178;
  - entMorphTargetSkinnedMeshComponent 0x106ECF1A8;
  - entGarmentSkinnedMeshComponent 0x106ECF998.
- The slots follow the Itanium +8 rule (MSVC + 8). The macOS slots are the same in all four vtables:

| slot | Mesh | Skinned | Garment | MorphTarget |
|---|---|---|---|---|
| **LoadResource vt+0x268** | **0x100C685FC** | 0x100C7D024 | 0x100C7D024 | 0x100C6E710 |
| **RefreshAppearance vt+0x288** | **0x100C699AC** | 0x100C7EDA4 | 0x100C75330 | 0x100C701DC |

**LoadResource**
- Does what Codeware expects:
  1. a base call (0x100C832EC);
  2. `token = LoadAsync(&this->mesh (+0x150), 0)`;
  3. `queue.Wait(token job)` (0x1009D4588 with x0 = **x1 = JobQueue&**);
  4. dispatches the job labelled "MeshComponent_OnMeshLoaded" (respectively "SkinnedMeshComponent_OnMeshLoaded" and "MorphTargetSkinnedMeshComponent_OnMeshLoaded") into that queue.
- x0 is left as whatever the last call returned: the uint64 result is meaningless on mac (Codeware ignores it).

**RefreshAppearance**
- Mesh 0x100C699AC:
  1. 0x100C69548(this) destroys or releases the render proxy at +0x140 (through the scene when attached, flag +0x12C);
  2. `0x100C688FC(this, false)` re-creates it from the mesh at +0x158 and the current appearance, and re-applies visibility.
- The skinned/morph variants reset their proxy SharedPtr at +0x1D8 / +0x1E0 first.
- In-game virtual use: 0x100C699FC calls `[vt+0x288]`.

**Action**
- On mac Codeware must use **`Core::RawVFunc<0x268, ...>`** and **`<0x288, ...>`**. The current MSVC offsets 0x260 and 0x280 hit different virtuals:

| current offset | mac target | what it is |
|---|---|---|
| 0x260 | 0x100C699D4 | `mov w0,#1; ret` |
| 0x280 | 0x100C69518 | `ldr w0,[x0,#0x184]` getter |

**MeshAppearance offsets (all MATCH)**
- Evidence is from LoadMaterialSetupAsync 0x100E1F7B4 and its job lambda 0x100E2117C.

| field | evidence |
|---|---|
| ForceCache +0xA0 | `ldrb` 0x100E1F890, `strb 1` 0x100E1F8D8, 0x100E21260 |
| MaterialLock +0x98 | lock 0x100E1F8A0/0x100E21254, unlock `ldclralb #0x80` 0x100E1FCC0/0x100E2139C; a u32 counter at +0x9C is updated under the lock (0x100E1F8A4, 0x100E21258) |
| MaterialData +0x48 | read 0x100E1F8B0; replaced 0x100E21374/0x100E21378, old one released with 0x1016A38D4 |
| MaterialJob +0x90 | IsDone 0x100E1F8C0; assign 0x100E1FC40 |
| Owner (CMesh*) +0x50 | 0x100E1F8C8, 0x100E1F938 |
| MaterialMap +0x68 | keys DynArray<CName> +0x68 (cap +0x70); values DynArray<Handle> +0x78 (cap +0x80, size +0x84); sorted flag +0x88 (0x100E2131C); insert 0x100E2032C (0x100E21308) |

## Related observations (not requested; verify before use)

**RenderData_Release (2996308699)**
- Candidate 0x1016A38D4: `--[x0+8]`; at zero it tail-calls `vt[+0x8]`. It has 2528 callers. It releases MeshAppearance.MaterialData at 0x100E21380 and RenderDataPtr temporaries at 0x100C674C0.
- AddRef is 0x1016A38C4.
- Confidence: medium.

**MeshAppearance_LoadMaterialSetupAsync (DB 1:0xE1F7B4, marked verified)**
- The mac body is `(appearance x0, u8 w1, sret x8 -> Handle<MeshAppearance>)`: `mov x19,x8` at 0x100E1F7DC, and the handle is stored to `[x19]` at 0x100E1F844.
- Codeware's declaration `(MeshAppearance&, Handle&, uint8)` therefore MISMATCHES. Its only user is commented out in ComponentWrapper.cpp.

**MeshComponent Handle<CMesh> at +0x158**
- 0x100C68920 reads `[comp+0x158]` as the mesh, consistent with Codeware's `unk158` use in ResetMaterialCache.

**Vehicle.hpp RecordID@0x638**
- Not checked.

**Scratch files**
- My disassembly scratch was in `_work/codeware/re_misc_tmp/` and has been deleted.
- `dis/pop_reg_rm.txt` belongs to another agent.
