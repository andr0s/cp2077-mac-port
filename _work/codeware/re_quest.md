# Codeware quest/journal/mappin/community RE: macOS 2.3.1 arm64 (static only)

Binary UUID A6656ADC-FBE2-36A4-9B9D-B4A9DE645089, base 0x100000000. Tools: `re.sh` (func/callers/xref/ptrsat/ptrsto/strrefs) and llvm-objdump.
Every address below is a function start according to `re.sh func`. `1:` = __TEXT offset; `2:` = __DATA_CONST offset (vmaddr 0x106DE0000).
"mac slot" = byte offset from the vtable address point. None of these hashes is in the macOS DB yet.

## Summary table

| item | mac address / slot | DB form | arm64 vs Codeware decl | conf. |
|---|---|---|---|---|
| QuestLoader_PhasePreloadCheck | 0x102ED49E0 (dead out-of-line copy; the live check is **inlined**) | 1:0x2ED49E0 | MATCH, but a hook never fires | high |
| QuestLoader PreloadList / NodePathMap | +0x20 (SortedArray\<QuestNodePath\>\*), +0x48 (HashMap\<ResourcePath,QuestNodePath\>) | – | MATCH | high |
| QuestPhaseInstance_Initialize | 0x102CFBA64 | 1:0x2CFBA64 | MATCH | high |
| QuestPhaseInstance_ExecuteGraph | 0x102CFB178 | 1:0x2CFB178 | MATCH | high |
| QuestPhaseInstance_ExecuteNode | 0x102CFD644 | 1:0x2CFD644 | MATCH | high |
| questPhaseInstance ExecutePath (MSVC 0x120) | mac slot **+0x128** (base 0x102CF7E58, root override 0x102D012EC) | 1:0x2CF7E58 | MATCH | high |
| questPhaseInstance fields 0x30/0x40/0x70/0x80/0x88/0x98 | all confirmed | – | MATCH | high |
| QuestsSystem_CreateContext | 0x102EFA150 | 1:0x2EFA150 | **MISMATCH: sret x8** | medium-high |
| QuestsSystem 0x60/0x68/0x78/0xF8 | all confirmed | – | MATCH | high |
| FactManager vtable | 0x1070FC388: GetFact +0x18, SetFact +0x20 (dtor first) | 2:0x31C388 | MATCH when declared as C++ virtuals | high |
| PointOfInterestMappin_SetPhase | 0x1042EAF14 (POI vtable slot +0x1D0) | 1:0x42EAF14 | MATCH | high |
| PointOfInterestMappin JournalHash | **+0x88** (Codeware uses 0x8C) | – | **MISMATCH** | high |
| MappinSystem SetPoiMappinPhase (MSVC 0x1D0) | mac slot +0x1D8, 0x1042A9934 | 1:0x42A9934 | MATCH | high |
| MappinSystem GetPoiMappinPhase (MSVC 0x3C8) | mac slot **+0x438**, 0x1042AA530 (**not +0x3D0**) | 1:0x42AA530 | MATCH at +0x438 | high |
| MappinSystem MappinsLock / MappinsData | +0x198 / +0x1A0 | – | MATCH | high |
| JournalManager GetEntries (MSVC 0x210) | +0x218, 0x101EB2FA0 | 1:0x1EB2FA0 | MATCH | high |
| JournalManager GetEntryByHash (MSVC 0x220) | **+0x220**, 0x101EB3214 (**not +0x228**) | 1:0x1EB3214 | **MISMATCH: sret x8** | high |
| JournalManager ChangeEntryState (MSVC 0x268) | **+0x268**, 0x101EB378C (**not +0x270**) | 1:0x1EB378C | MATCH | high |
| JournalManager GetEntryState (MSVC 0x2B0) | +0x2B8, 0x101EB60E8 | 1:0x1EB60E8 | MATCH | high |
| JournalManager GetEntryTimestamp (MSVC 0x2C8) | +0x2D0, 0x101EB6318 | 1:0x1EB6318 | MATCH | high |
| CommunitySystem Activate/Deactivate/SetPhase/ResetCommunity | +0x1B8/+0x1C0/+0x1C8/+0x1D0 | – | MATCH | high |
| CommunitySystem Activate/Deactivate/ResetSpawner | +0x1E0/+0x1E8/+0x1F0 | – | MATCH | high |
| CommunitySystem GetCommunity (MSVC 0x228) | +0x230, 0x1014ACA3C | – | **MISMATCH: sret x8, id by value** | high |
| CommunitySystem GetSpawner (MSVC 0x260) | +0x268, 0x1014AE614 | – | **MISMATCH: sret x8** | high |
| CommunitySystem_Update | 0x1014AD374 | 1:0x14AD374 | MATCH | high |
| Community / CommunityEntry layouts | confirmed; the virtual destructor is **slot 4**, not slot 3 | – | partial MISMATCH | high |
| Spawner layout | active/initialized at **0x10/0x11** (Codeware: 0x08/0x09) | – | **MISMATCH** | high |
| CommunityEntrySpawner +0x18 | elements are **0x28 bytes**, not EntityID | – | **MISMATCH** (element type) | high |

## 1. QuestLoader_PhasePreloadCheck (+ QuestLoader layout)

**The class.** The "QuestLoader" is the 0x80-byte object whose jobs are labelled `"QuestResourceLoader/Finish"`.
- Constructor: 0x102ED40B8 `(this, SortedArray<QuestNodePath>* preloadList)`.
- It is created at:
  - 0x102ED5274 and 0x102ED5608, with preloadList = null;
  - 0x102ED59C4 (inside fn_102ED5910), with preloadList = x4. Each quest's ResourcePath→NodePath is then registered via 0x102ED4400 → 0x102ED4508.

**Layout (from the constructor):**

| offset | field | evidence |
|---|---|---|
| +0x20 | preloadList pointer (**PreloadList/SortedPreloadList**) | `str x20,[x19,#0x20]` @0x102ED4100 |
| +0x28 | inline SortedArray\<QuestNodePath\> of parent paths, filled by 0x102ED4184 | @0x102ED4104..0x102ED411C |
| +0x40 | lock byte | @0x102ED4128 |
| +0x48 | HashMap (**NodePathMap**) | +0x48/+0x58 zeroed, `d8=0xFFFFFFFF..` at +0x68, allocator at +0x70: @0x102ED4134..0x102ED414C |

**NodePathMap @0x48 is `HashMap<ResourcePath, QuestNodePath>`.** The lookup is 0x102ED4878 `bool(loader, ResourcePath x1, QuestNodePath& x2)`:
- It locks +0x40 and reads size [+0x50], buckets [+0x54], index table [+0x48], nodes [+0x58] and stride [+0x64] (@0x102ED48A8..0x102ED48DC).
- The u64 key is at node+8 (@0x102ED4900) and the value DynArray\<u16\> at node+0x10 (@0x102ED490C..0x102ED4920).
- The insert function is 0x102ED4508 (same map, same lock).

**SortedArray layout** = SDK layout {entries, cap, size @0xC, flags @0x10, bit0 = NotSorted}.
- Its search is 0x102ED4A24 `bool(SortedArray<QuestNodePath>*, const QuestNodePath&)`.
- It sorts if bit0 is set (0x102ED4A48, 0x102ED89C4), clears the bit (0x102ED4AD4), then binary-searches by size and then by u16 elements.

**PhasePreloadCheck itself:**
- An exact out-of-line copy exists at **0x102ED49E0** (size 0x44): `return Search([loader+0x20], path) || Search(loader+0x28, path)` (0x102ED49F4..0x102ED4A20).
- Signature `bool(void* loader x0, const QuestNodePath& x1)` → **MATCH**. First instructions are stp/stp/add/mov, with no PC-relative instruction.
- **It has 0 BL/B callers, 0 ADR/ADRP refs and 0 data pointers, so it is dead code. HookAfter would never fire.** The any-of variant 0x102ED4C50 is dead too.
- The live check is inlined at three places:
  - **0x102EDB820** (recursive phase-graph walk, state = {loader @0, ? @8, current QuestNodePath @0x10}). At 0x102EDBA54..0x102EDBA74, `[loader+0x20]` and then `loader+0x28` are searched with the current path. On a hit it calls 0x102D60DEC(phaseNode), which marks the node for preload. This is exactly the Windows call site.
  - 0x102ED78B0 (any-of loop over a path list, at 0x102ED7AB8..0x102ED7AD4).
  - The constructor helper 0x102ED4184 (uses only +0x20).
- **Recommendation for the mac port:** to emulate Codeware's HookAfter, hook the search primitive 0x102ED4A24.
  - 9 call sites; its first instructions are sub/stp only, with no PC-relative instruction.
  - Act only when x0 == some loader's +0x20 list. Capture loaders by hooking the constructor 0x102ED40B8, or record open-world NodePaths by hooking the insert 0x102ED4508.
- Note: Codeware's `PreloadList->PushBack` does not set the NotSorted flag. The next binary search can therefore miss the appended path (same as on Windows).
- Confidence: high for identity and layout; the hook site is "inlined".

## 2. QuestPhaseInstance

**Class registration.**
- `"questPhaseInstance"` is registered at 0x102CFDC14 (CClass size 0x110 @0x102CFDC2C).
- TTypedClass +0xE0 → 0x102CFDC78 → constructor **0x102CF443C**, which stores vtable **0x1070FD088** (2:0x31D088) at 0x102CF4458..0x102CF4460.
- questRootInstance has vtable 0x1070FD348 (2:0x31D348), also 0x110 bytes.

**Field offsets (all confirmed):**

| offset | field | evidence |
|---|---|---|
| 0x30 | Resource handle | constructor `stp q0,q0,[x19,#0x30]` @0x102CF4468; Initialize stores the resource handle `str q0,[x20,#0x30]` @0x102CFBACC/0x102CFBAE8 |
| 0x40 | Graph handle | constructor (same `stp` as 0x30); Initialize stores the graph handle @0x102CFBB3C/0x102CFBB58 |
| 0x70 | NodePath (DynArray\<u16\>, size @0x7C) | constructor @0x102CF4470..0x102CF4488; Initialize copies parentPath and appends nodeID (@0x102CFBB8C..0x102CFBCC0) |
| 0x80 | NodePathHash = FNV-1a32 over the path bytes (prime 0x01000193) | @0x102CFBCCC..0x102CFBD08 `str w8,[x22,#0x80]!`. This is the only writer in the class code |
| 0x88 | Children: DynArray\<Handle\<questPhaseInstance\>\>, stride 0x10, size @0x94 | constructor @0x102CF4494..0x102CF44AC; CreateChild 0x102CF5058 pushes (@0x102CF50B4, 0x102CF510C); ExecutePath scans children by `[child+0x80]` (@0x102CF8178..0x102CF8188) |
| 0x98 | Handlers: HashMap (allocator @0xC0) | constructor @0x102CF44C4..0x102CF44D8; lookup @0x102CF68C0..0x102CF691C uses size @0xA0, buckets @0xA4, index @0x98, nodes @0xA8, stride @0xB4, a **u32 key at node+8** and the value pointer at node+0x10. This is layout-compatible with `HashMap<u32, SharedPtr<Handler>>` (medium confidence on the value type) |

**QuestContext (mac)** — constructor 0x102CF2FF8, reached via thunk 0x102CF313C (size 0x14). The struct is 0x340 bytes and MATCHes Codeware.

| offset | field | evidence |
|---|---|---|
| 0xF8 | phaseStack (size @0x104), pre-reserved to 16 | @0x102CF30BC..0x102CF311C |
| 0x110 | phaseContext; its vtable 0x1070FCDE0 is set after base constructor 0x102CEF43C | @0x102CF30E0..0x102CF30F4 |
| 0x330 | back-pointer | @0x102CF30F8 |

Helpers: 0x102CF3224 = `ldr w0,[x0,#0x104]` (depth); 0x102CF3150 = push; 0x102CF31FC = pop (returns the popped phase).

**QuestNodeSocket** = {CName @0, u8 @8}: `str x22,[sp,#0x10]; strb wzr,[sp,#0x18]` @0x102CF8060.

### Initialize → 0x102CFBA64 (size 0x608)

**Arguments:** `(phase x0, QuestContext& x1, const Handle<questQuestPhaseResource>& x2, Handle<questGraphDefinition>& x3, const QuestNodePath& parentPath x4, QuestNodeID w5)`. w5 is stored as a halfword at 0x102CFBA98.

**Body:**
1. Push the phase onto ctx.
2. Set +0x30, +0x40, +0x70 and +0x80.
3. Register hash→path into QuestsSystem+0x78 through ctx+0x28 (0x102CFBD0C..0x102CFBD1C).
4. Scan the graph nodes. This includes FactManager GetFact(store 3) via ctx+0x138 (@0x102CFBE9C..0x102CFBEAC).
5. Return x0 = pop(ctx), i.e. the phase. That matches the declared `void*`.

**Signature: MATCH.**

**Reachability.** The only reference is a tail call `B` from CreateChild 0x102CF5058:
- CreateChild is `Handle<questPhaseInstance>` returned via x8, args `(parent, ctx, res&, graph&, nodeID w4)`.
- It allocates 0x110 bytes, runs constructor 0x102CF443C, appends to parent+0x88, and passes parentPath = parent+0x70.
- CreateChild has 8 callers. One is QuestRootInstance_Start 0x102D01878, which uses the root's empty path and the quest index as nodeID. Codeware's "root/first phase" test therefore still holds.

**Hooking:** an entry hook is fine. There is no PC-relative instruction in the first 4 instructions (sub/stp×3).

### ExecuteGraph → 0x102CFB178 (size 0x540, 14 callers)

**Arguments:** `(phase x0, QuestContext& x1, const WeakHandle<questNodeDefinition>& x2, const QuestNodeSocket& x3, bool w4, DynArray<QuestNodeSocket>& x5)` → **uint8 in w0**.

**Body:**
- It copies the 16-byte weak node and the socket into an execution queue (weak++ at 0x102CFB214).
- w4 selects the immediate queue (@0x102CFB224).
- It loops step 0x102CFCE70 and returns `w23 & 0xff` (0x102CFB55C).
- ExecutePath calls it as `(this, ctx, &weakNode, &socket, 1, out)` at 0x102CF8084..0x102CF809C.

**Signature: MATCH.**

### ExecuteNode → 0x102CFD644 (size 0x20C)

**Arguments:** `(phase x0, questNodeDefinition* x1, QuestContext& x2, const QuestNodeSocket& x3, DynArray<QuestNodeSocket>& x4)` → uint8 in w0.

**Body:**
1. Check the disabled-node set at phase+0xC8.
2. Call node pre-check 0x102D5A758(node, ctx+0x110, socket).
3. Call listener `[ctx+0x70]` vt+0x1C0/+0x1C8.
4. Execute via 0x102D5A764 and 0x102D5A8A4.
5. Return w19 (0..3).

**Caller:** only step 0x102CFCE70, at 0x102CFD0C4: `(x21 phase, x22 rawNode, [state]=ctx, &elem.socket, outSockets)`.

**Signature: MATCH.**

### ExecutePath → MSVC 0x120 = mac slot +0x128

The +8 rule holds: ISerializable's destructor is at MSVC 0x18, and there is no overload here.

**Base: 0x102CF7E58** `(this, QuestContext& x1, const QuestNodePath& x2, QuestNodeID w3, CName x4, DynArray<QuestNodeSocket>& x5)` → uint8.
- If depth < path.size, it finds the child by FNV(hash@0x80 + nodeID) and recurses through `child->vt[+0x128]` with the same 6 arguments (0x102CF8270..0x102CF8290).
- Otherwise it finds node w3 (with an id-remap map at +0xE0), builds socket {x4, 0} and calls ExecuteGraph.

**Root override (vtable 0x1070FD348 slot +0x128): 0x102D012EC.**
- It takes child = root->children[path[0]] (0x102D01320..0x102D0132C; **no bounds check**).
- It pushes the root (0x102CF4FB8), calls `child->vt[+0x128]` (0x102D01364..0x102D01384) and pops.

The sibling at slot +0x120 (0x102CF75D8) has the same signature but also creates missing children. Do not confuse the two.

**Signature: MATCH.** Use `RawVFunc<0x128>` on mac.

## 3. QuestsSystem_CreateContext + QuestsSystem/FactManager

### CreateContext → 0x102EFA150 (size 0x44)

**Body:** it loads `[qs+0xE8]`, `qs+0xA8`, `qs+0xB8`, `qs+0x78`, `qs+0x1D0`, `[qs+0x2C8]` and `[qs+0x40]`, then tail-calls the QuestContext constructor with **x0 = x8**, passing w1..w4 through unchanged.

**Signature:**
- mac: `QuestContext (questIQuestsSystem* x0, i32 a3 w1, i8 a4 w2, i32 a5 w3, i32 a6 w4)`, with the **result via sret x8**.
- The constructor stores a3 as a byte at +0 and +1, a4 as a byte at +2, and a5/a6 at +4/+8 (0x102CF301C..0x102CF3028).
- Codeware's `void*(*)(sys, QuestContext*, a3..a6)` is the MSVC hidden-return form. **MISMATCH**: on mac pass `x8 = &context, x0 = sys, w1..w4 = 1, 0, -1, -1`.

**Identity evidence:**
- The same member set is inlined in 18 QuestsSystem methods (e.g. mac vt+0x218 0x102EFA06C passes 3,0,-1,-1; vt+0x220 0x102EFA194 passes 5,0,-1,-1). This is the out-of-line copy.
- It has one caller, 0x102F030B0 (`x8=sp`, w1..w4 = 11,0,-1,-1). That caller is itself unreferenced.
- Confidence: medium-high. The Windows identity is inferred from the shape.

**Extra note:** the game destroys the context with 0x102F00294 after use (e.g. at 0x102F0310C). Codeware does not do this; it is the same as on Windows.

### QuestsSystem offsets

The QuestsSystem constructor is 0x102EF4E58 (vtable store at 0x102EF4E84).

| offset | field | evidence |
|---|---|---|
| 0x60 | Mutex | `strb wzr,[x0,#0x60]` @0x102EF4E90; exclusive lock 0x1000020C0 / `ldclralb #0x80` around root calls @0x102EFA0E8 / 0x102EFA100 |
| 0x68 | RootPhase handle | `stur q0,[x0,#0x68]` @0x102EF4E94; `ldr x1/x0,[x20,#0x68]` @0x102EFA0D4 / 0x102EFA0EC |
| 0x78 | NodePathHashMap: `HashMap<u32, QuestNodePath>` | constructor @0x102EF4EA4..0x102EF4EB8; lookup @0x102EFA1E0..0x102EFA244 (size +0x80, buckets +0x84, nodes +0x88, stride +0x94, u32 key at node+8, DynArray\<u16\> at node+0x10) |
| 0xF8 | FactManager\* | getter = QuestsSystem vt+0x278 0x102EFFFF0 `ldr x0,[x0,#0xf8]`; created in vt+0x1A8 0x102EF5900 (`new(0x60)` then constructor 0x102CE1BE8, `str x21,[x19,#0xf8]` @0x102EF59FC) |

### FactManager vtable → 0x1070FC388 (2:0x31C388)

| mac slot | function | address |
|---|---|---|
| +0x00 | D1 | 0x102CE22DC |
| +0x08 | D0 | 0x102CE2340 |
| +0x10 | sub_08 | 0x102CE1CD8 |
| **+0x18** | **GetFact** `int32(this, u32 store w1, u32 fact w2)` | 0x102CE1D10 (`[this+8+(store-1)*8]` → 0x102CE046C) |
| **+0x20** | **SetFact** `(this, store w1, fact w2, i32 value w3)` | 0x102CE1D24 |

Proof from the script natives:
- GetFact handler 0x102EFF278 calls `fm->vt[+0x18](fm,1,hash)` (0x102EFF334..0x102EFF344).
- SetFact handler 0x102EFF494 calls `vt[+0x20](fm,1,hash,value)` (0x102EFF5A0..0x102EFF5B8).

Codeware's `virtual ~FactManager(); sub_08; GetFact; SetFact` compiled with clang yields exactly D1/D0, +0x10, +0x18, +0x20 → **MATCH**.

The store area is 10 FactStores at +0x8..+0x50 plus a u32 count at +0x58 (constructor 0x102CE1C0C..0x102CE1CB8). This is size-compatible with `FactStore* data[11]`, which Codeware does not use.

## 4. PointOfInterestMappin_SetPhase + MappinSystem

### POI class and SetPhase

- **POI class:** `"gamemappinsPointOfInterestMappin"` is registered at 0x1042ED2EC (size 0x100). ConstructCls 0x1042ED34C stores vtable **0x1072DD040** (2:0x4FD040).
- **IMappin base:** constructor 0x1042E2B14 writes fields up to the byte at +0x84, so the mac dsize is 0x85.

**SetPhase → 0x1042EAF14** = POI vtable slot **+0x1D0** (data 0x1072DD210, the only reference). Size 0x1D0; no PC-relative instruction in the first 4 instructions.

How the call reaches it:
1. MappinSystem SetPoiMappinPhase queues request type 5 (vtable 0x1072DA168).
2. The request's Execute (slot +0x30) is 0x1042B85AC: `x8=[x1]` (mappin), `w1=[req+0x1c]` (phase), `br [vt+0x1d0]`.
3. The update loop dispatches type 5 at 0x1042B95B8.

Body:
- `x20 = [this+0xB0]->vt[+0x38](data, phase)` (0x1042EAF38..0x1042EAF44).
- It looks up the saved state by the journal hash `[this+0x88]` (0x1042EAF58, 0x1042AA568 → 0x1042BF908 at system+0x110).
- It writes state.phase (u16 @0) if changed (0x1042EAFC0..0x1042EAFDC).
- If phase == 0 (Completed), it runs extra completion logic (0x1042EAFE0..0x1042EB0C0).
- It returns `x0 = x20` (bool).

**Signature `bool(mappin x0, gamedataMappinPhase w1)`: MATCH.**

Do not confuse it with its sibling **vt+0x1C8, 0x1042EADCC = SetVariant**:
- It uses data->vt[+0x30] and writes state u16 @2.
- Runtime data +0x10 defaults to 0x94 = MappinVariant::Invalid (constructor 0x1042FB734).
- It is invoked by MappinSystem vt+0x1D0 0x1042A9784 = ChangeMappinVariant(u64 id, variant). The script native "ChangeMappinVariant" 0x1042B43F0 calls it at 0x1042B44A0.

### JournalHash: mac **+0x88** (Codeware 0x8C) → MISMATCH

The cause is Itanium tail-padding reuse: the POI's first member (a bool) is packed at +0x85.

| offset | field | evidence |
|---|---|---|
| +0x85 | bool | POI constructor 0x1042EA968 |
| +0x88 | the POI's own journal-path hash (u32) | constructor `stp w20,wzr,[x22,#0x88]` @0x1042EA96C; SetPhase/SetVariant key @0x1042EAF58 / 0x1042EAE10; vt+0x150 0x1042EAC64 `ldr w8,[x0,#0x88]; cmp w8,w1` |
| +0x8C | hash of the JournalPointOfInterestMappin.questPath | written @0x1042EBAD8..0x1042EBAF8 |
| +0x90 | bool | |

### MappinSystem (vtable 0x1072D9888, 2:0x4F9888)

**SetPoiMappinPhase, MSVC 0x1D0 → mac +0x1D8 = 0x1042A9934** (+8 rule holds).
- Proof:
  - The quest node questJournalChangeMappinPhase_NodeType::Execute 0x102D3D28C calls `[phaseCtx+0x128]->vt[+0x1d8](ms, hash, [node+0x4c] phase)` (0x102D3D318..0x102D3D334).
  - phaseCtx+0x128 = GetSystem(gamemappinsIMappinSystem); it is stored at 0x102CF0AD8 using class getter 0x102007518 → registration 0x102009C88 "gamemappinsIMappinSystem".
- Body: request type 5 with `str w1,[req+0x18]` (32-bit hash, 0x10429D418) and `str w2,[req+0x1c]` (0x1042A9988), queued at this+0x2C0. The call is **deferred**: SetPhase runs later on the mappin update.
- Signature `void(this, u32 hash w1, phase w2)`: **MATCH**.

**GetPoiMappinPhase, MSVC 0x3C8 → mac +0x438 = 0x1042AA530** (size 0x38; the last slot, data 0x1072D9CC0).
- Body: `found = 0x1042BF908(this+0x110, w1 hash, &state, 0)`; return found ? state.phase (u16 @0) : **5 (Invalid)** (0x1042AA53C..0x1042AA558).
- Game caller: fn_102F09618 (QuestsSystem; uses `"q115_point_of_no_return"`). At 0x102F09734..0x102F09748 it does `ms->vt[+0x438](ms, hash); cmp w0,#5`.
- Signature `gamedataMappinPhase(this, u32 w1)`: **MATCH at +0x438**.
- **The +8 rule fails here.** mac +0x3D0 is 0x1042AF1B4 = `void(this, u64 id, bool)`, which sets or clears bit1 in HashMap\<u64,u8\>@0x138 (lock +0x168). Calling it would corrupt state.

**MappinsLock @0x198 / MappinsData @0x1A0: MATCH.**
- Constructor: `strb wzr,[x19,#0x198]` @0x1042A3FE0, SortedArray @0x1A0 @0x1042A3FD8.
- GetMappin 0x1042B1B98 takes a shared lock with 0x100002098(this+0x198) (0x1042B1BB4) and finds the entry in +0x1A0 via 0x1042AAD2C. The handle is at entry+8 (stride 0x18 = {u64 id, Handle}). It shared-unlocks with `ldaddalb -1` (0x1042B1C08).
- Exclusive users: 0x1042B135C..0x1042B1398 (lock 0x1000020C0 / `ldclralb 0x80`).
- Update loop: +0x1A0 entries, +0x1AC count, +0x1B0 flags (0x1042B94B8, 0x1042B95C8..0x1042B95DC).

## 5. JournalManager RawVFuncs (vtable 0x106FA5A90, 2:0x1C5A90)

Overloads are reversed by MSVC: GetEntry(hash/path), ChangeEntryState(entry/path) and GetEntryState(path/entry/hash). **The +8 rule is wrong for GetEntryByHash and ChangeEntryState.** All slots below are proven by the game's own script natives (registration fn_101EAA70C).

| Codeware (MSVC) | mac slot | address | arm64 signature | verdict |
|---|---|---|---|---|
| GetEntries 0x210 | **+0x218** | 0x101EB2FA0 | `void(this, const gameJournalRequestContext& x1, DynArray<WeakHandle<gameJournalEntry>>& x2)`: clears x2; records @+0x108 (stride 0x14, count +0x114); `vt[+0x220](x8=tmp, w1=hash)`; filters state bytes ctx+0..3 / class CName ctx+8 / filter ctx+0x10 | MATCH (no sret) |
| GetEntryByHash 0x220 | **+0x220** | 0x101EB3214 | **`Handle<gameJournalEntry>` via sret x8, (this x0, u32 hash w1)**; HashMap @+0x70, spin byte +0x68. Native GetEntry 0x101EC34F0 (0x101EC355C..0x101EC3570) and ChangeEntryStateByHash 0x101EC1D24 (`w1=hash; x8=sp; blr [vt+0x220]` @0x101EC1E08..0x101EC1E1C) | **MISMATCH (sret)**. +0x228 (0x101EB3300) is GetEntry(const Handle\<JournalPath\>&), which would treat the hash as a path pointer |
| ChangeEntryState 0x268 | **+0x268** | 0x101EB378C | `bool(this, Handle<gameJournalEntry>& x1, state w2, notify w3, u32 w4)`. Native ChangeEntryStateByHash: `x1=&entry, w2=state, w3=notify, w4=1; blr [vt+0x268]` @0x101EC1E28..0x101EC1E40; state switch @0x101EB37FC/0x101EB3820; returns entry!=null @0x101EB385C | MATCH. +0x270 (0x101EB45D8) is the **path** overload (it calls +0x228 and then +0x268) |
| GetEntryState 0x2B0 | +0x2B8 | 0x101EB60E8 | `state(this, Handle<Entry>& x1)` = vt[+0x2C0](this, vt[+0x238](x1)). Native GetEntryState 0x101EC11F0 calls vt[+0x2B8] with x1=&handle (0x101EC1298..0x101EC12A8). +0x2B0 = path overload, +0x2C0 = by hash (default 1 = Inactive) | MATCH |
| GetEntryTimestamp 0x2C8 | +0x2D0 | 0x101EB6318 | `u32(this, Handle<Entry>& x1)` = record+8 u32 (0x101E9E8D0). Native GetEntryTimestamp 0x101EC1318 calls vt[+0x2D0] and stores w0 (0x101EC13C4..0x101EC13EC). +0x2C8 = 48-bit (time<<16 \| seq) variant | MATCH |

Also used: +0x238 = GetEntryHash(Handle&) → u32 (native GetEntryHash 0x101EC1568 → vt[+0x238]).

For GetEntryByHash the mac declaration must be `Red::Handle<gameJournalEntry> (gameIJournalManager::*)(uint32_t)`, called at slot 0x220.

## 6. CommunitySystem (vtable 0x106F2F670, 2:0x14F670)

**Class:** `"gameCommunitySystem"` registration 0x1014AF1A8 → ConstructCls 0x1014AF1FC → constructor 0x1014AB9DC.

**Object layout:**

| offset | field |
|---|---|
| +0x48 | request DynArray, stride 0x20 = {CName entry, CName phase, u64 id, u8 type @0x18} |
| +0x68 | sorted ids (dirty flag +0x88) |
| +0x78 | SharedPtr\<Community\> array |
| +0xA0 / +0xA1 / +0xA2 | locks |
| +0xD8 | spawner manager (0x2090 bytes) |
| +0x109 | force flag |

**The +8 rule holds for all requested slots:**

| Codeware (MSVC) | mac slot | address | evidence | verdict |
|---|---|---|---|---|
| ActivateCommunity 0x1B0 | +0x1B8 | 0x1014ACE68 | `(this, u64 id x1, CName x2)`. If registered (0x1014ACF0C: binary search +0x68, shared lock +0xA2), lock +0xA0 and push {entry, 0, id, **type 0**} (0x1014ACEE4..0x1014ACEEC). Update dispatches type 0 → 0x101489ED0 → 0x101488CEC (entry.active 0→1, vt+0x20) | MATCH |
| DeactivateCommunity 0x1B8 | +0x1C0 | 0x1014AD028 | type 1 (0x1014AD0AC) → 0x101489F74 → 0x101488D10 (active→0, vt+0x28) | MATCH |
| SetCommunityPhase 0x1C0 | +0x1C8 | 0x1014AD0D0 | `(id x1, entry x2, phase x3)` → {entry, phase, id, type 2} (0x1014AD150..0x1014AD15C) → 0x10148A1AC | MATCH |
| ResetCommunity 0x1C8 | +0x1D0 | 0x1014AD17C | type 3 (0x1014AD200) → 0x101489FE4 | MATCH |
| ActivateSpawner 0x1D8 | +0x1E0 | 0x1014AD234 (8-byte thunk `ldr x0,[x0,#0xd8]; b`) | → 0x1015545FC queues {id, type 2}; handler 0x101555664 (jump table 0x106B4C298) → 0x101F05608 (if initialized@0x11 && !active@0x10 → active=1, vt+0x40) | MATCH |
| DeactivateSpawner 0x1E0 | +0x1E8 | 0x1014AD23C (thunk) | → 0x1015547A0 (type 3) → 0x101F055EC (active→0, vt+0x48) | MATCH |
| ResetSpawner 0x1E8 | +0x1F0 | 0x1014AD244 (thunk) | → 0x101554944 (type 4) → 0x10154D430 | MATCH |
| GetCommunity 0x228 | +0x230 | 0x1014ACA3C | **WeakPtr\<Community\> via sret x8; EntityID by value in x1** (compared directly with 0x101F05CE8 `cmp x0,x1`); result copied from +0x78 with weak++ (0x1014ACB38..0x1014ACB6C). +0x228 0x1014ADCF8 is an identical twin; game call 0x10148C618..0x10148C62C passes `x1=[id]`, `x8=&out`. +0x218/+0x220 are converting wrappers | **MISMATCH**: Codeware passes (x1=&out, x2=&id) |
| GetSpawner 0x260 | +0x268 | 0x1014AE614 (thunk → 0x101554E2C) | **WeakPtr\<Spawner\> via sret x8, id by value x1** (0x101554E44, 0x101554EA4, result 0x101554F3C..0x101554F70). +0x250..+0x260 are converting wrappers with the same ABI | **MISMATCH (sret)** |

Mac declarations for the two mismatches:
- `Red::WeakPtr<Community> (ICommunitySystem::*)(uint64_t id)`
- `Red::WeakPtr<Spawner> (ICommunitySystem::*)(uint64_t id)`

### CommunitySystem_Update → 0x1014AD374 (size 0x328, 1:0x14AD374)

**Signature:** `void(ICommunitySystem* x0, bool force w1)` → **MATCH**.

**Body:**
- force → time budget DBL_MAX, else 1 ms (0x1014AD3B0..0x1014AD3D0).
- Drains queue +0x58.
- Under lock +0xA0, drains the community requests at +0x48 and dispatches types 0..3 (jump table 0x106B4BF40; 0x1014AD500..0x1014AD654).

**Callers:**
- The "CommunitySystem/Update" job lambda 0x1014AF748, with `w1=[this+0x109]` (0x1014AF7A8..0x1014AF7B0).
- The dead full-update 0x1014AD304.

**Note:** spawner requests are drained by spawner-manager update 0x101553D14 (in the job, before Update). `Update(sys,true)` therefore does **not** flush ActivateSpawner, DeactivateSpawner or ResetSpawner on mac; they apply on the next frame.

### Struct layouts

**Community** (constructor 0x1014897A4; base constructor 0x101F05558) → MATCH for the data fields:

| offset | field | evidence |
|---|---|---|
| 0x00 | vptr 0x106F2D9B8 | |
| 0x08 | **id** | `stp x8,x1,[x0]` |
| 0x10 / 0x11 | active / initialized | `strh wzr,[x0,#0x10]` |
| 0x18 | entries | @0x1014897D8 |
| 0x28 | unk28 | @0x1014897FC |
| 0x38 | template\_ handle | @0x101489828 |
| 0x48 | bool | |

**Virtual layout (base vtable 0x106FACAA8):**
- There are **4 pure virtuals before the destructor**, D1/D0 are at slots 4/5, and the pure virtuals at +0x40/+0x48 are OnActivate/OnDeactivate.
- Community slots 0..3 are 0x10148A234, 0x10148A2F8, 0x10148A398 and 0x10148A400; D1 0x101489D50 and D0 0x101489D54 are at +0x20/+0x28.
- Spawner slots 0..3 are 0x10154D8F8, 0x10154E528, 0x10154E554 and 0x10154E630; D1/D0 0x10154D178/0x10154D184 are at +0x20/+0x28.
- Codeware declares 3 virtuals before `~Community()`/`~Spawner()`. Slots 0..2 (GetActiveEntityIDs etc.) are correct, but a virtual delete would call slot 3 instead of the destructor. **MISMATCH.** Add a 4th dummy virtual before the destructor on mac if SharedPtr\<Community/Spawner\> can be the last owner (e.g. through the script wrappers).

**CommunityEntry** (0x50 bytes; constructor 0x101493D48, base constructor 0x101488AE0, vtable 0x106F2DF30, destructor first) → **MATCH**:

| offset | field | evidence |
|---|---|---|
| 0x08 | phases | @0x101488B10..0x101488B60 |
| 0x18 | communityID | @0x101488B6C |
| 0x20 | name | @0x101488B6C |
| 0x28 | active (i32) | @0x101488B70 |
| 0x2C | unk2C: u16 current phase index, 0xFFFF = none | @0x101488B78 |
| 0x2E | unk2E | @0x101488B7C |
| 0x30 | spawner | @0x101493DBC; spawner allocated and set @0x101493E34..0x101493E54 |
| 0x38 | unk38 | @0x101493DBC |
| 0x40 | spawnEntry | @0x101493DC4 |

Entry matching by name `[entry+0x20]` is seen at 0x101489EF4.

**CommunityEntrySpawner** (0xB8 bytes; constructor 0x101497AF0, destructor 0x101497DB4, vtable 0x106F2E888, destructor first):
- +0x08 is a 16-byte record Handle. Codeware's `DynArray<void*> unk08` there is unused and the same size.
- DynArrays at +0x18/+0x28/+0x38/+0x48 (0x101497B40..0x101497BC4).
- +0x28, +0x38 and +0x48 hold 8-byte EntityIDs: the destructor frees them with elemSize 8, and slot 2 0x10149B3AC pushes an EntityID into +0x38.
- **+0x18 has 0x28-byte elements:** the destructor iterates with stride 0x28 and frees with elemSize 0x28; each element has a trivial head plus a SharedPtr at elem+0x18 (0x101497ED0..0x101497F4C). Codeware's `DynArray<EntityID> restoredEntityIDs @0x18` reads the wrong stride. **MISMATCH** (offset OK, element type not).

**Spawner** (constructor 0x10154CD40, destructor 0x10154D050; vtable 0x106F359D8, secondary vptr @0x18 = 0x106F35A48):

| offset | field | evidence | verdict |
|---|---|---|---|
| 0x08 | id | base constructor | |
| 0x10 / 0x11 | **active / initialized** | base constructor; 0x101F05608 / 0x101F055EC | **MISMATCH** (header 0x08/0x09) |
| 0x20 | transform | 0x10154CD98..0x10154CDB4 | MATCH |
| 0x40 | recordID | 0x10154CDBC | MATCH |
| 0x48 | appearanceName | 0x10154CDC0 | MATCH |
| 0x50..0x56 | unk50/54/55/56 | 0x10154CDC4 | MATCH |
| 0x60 | record Handle | 0x10154CDC8 | MATCH |
| 0x70 | unk70 | 0x10154CDD4 | MATCH |
| 0x78 / 0x88 / 0x98 | restored / spawned / reserved DynArray\<EntityID\> | 0x10154CDD8..0x10154CE38; destructor elemSize 8 | MATCH |

## Hook and ABI notes

- None of the reported functions starts with a PC-relative instruction in its first 4 instructions.
- The community spawner thunks (+0x1E0/+0x1E8/+0x1F0/+0x268) are 8-byte `ldr; b` stubs. They are only called through the vtable, never hooked.
- Codeware hooks on mac:
  - **Initialize 0x102CFBA64** and **POI SetPhase 0x1042EAF14** are safe entry hooks.
  - **PhasePreloadCheck is inlined**; see item 1 for a substitute.
- **RawVFunc offsets to use on mac** (several are not MSVC+8):

  | function | mac offset |
  |---|---|
  | questPhaseInstance ExecutePath | 0x128 |
  | MappinSystem SetPoiMappinPhase | 0x1D8 |
  | MappinSystem GetPoiMappinPhase | **0x438** |
  | JournalManager GetEntries | 0x218 |
  | JournalManager GetEntryByHash | **0x220** |
  | JournalManager ChangeEntryState | **0x268** |
  | JournalManager GetEntryState | 0x2B8 |
  | JournalManager GetEntryTimestamp | 0x2D0 |
  | CommunitySystem functions | MSVC+8 (see item 6) |

- **sret fixes needed:** CreateContext, JournalManager GetEntryByHash, CommunitySystem GetCommunity (also: id by value) and GetSpawner.
