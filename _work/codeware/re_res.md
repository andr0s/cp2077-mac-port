# Codeware RE results (macOS 2.3.1 arm64, UUID A6656ADC-...), static only

Tools: `_work/re/re.sh`, `dis.sh`, llvm-objdump range dumps (text grep only). Nothing was run or modified outside
`_work/codeware/`. Addresses are absolute; DB form `1:0x<addr-0x100000000>` (code) / `3:0x<addr-0x107394000>`
(__DATA). Every function address below was checked with `re.sh func` to be a function start.

## Summary

| item | name | mac address | DB form | arm64 sig vs Codeware | confidence |
|---|---|---|---|---|---|
| 1 | ObjectSerializer_Prepare | 0x102161438 | 1:0x2161438 | MATCH | high |
| 1 | ObjectSerializer_WriteToBuffer | 0x1011D6FA4 | 1:0x11D6FA4 | MATCH (DataBuffer via x8) | high |
| 1 | ObjectSerializer_ReadFromStrean | 0x1011D6E58 | 1:0x11D6E58 | **MISMATCH** (Handle via x8; stream x0, type x1) | high |
| 2 | ISerializable_Clone | not found | - | - | - |
| 3 | ResourceSerializer_SchedulePostLoadJobs | no function (inlined); hook lambda 0x102257548 instead | - | different shape | high (inlined) |
| 3 | ResourceSerializer_ScheduleFinalizeJobs | no function (inlined); hook lambda 0x102257830 instead | - | different shape | high (inlined) |
| 3 | ResourceSerializer_ExecutePostLoad | 0x102259888 | 1:0x2259888 (DB 1:0x21AC670 refuted) | MATCH | high |
| 3 | ResourceSerializer_OnResourceReady | no function (inlined, 5 copies) | DB 1:0x20AB510 refuted | - | high (refutation) |
| 3 | ResourceSerializer +0x230 serializables | verified | - | - | high |
| 4 | SoundBankManager_ReadSoundBanksJson | 0x100DC8D0C | 1:0xDC8D0C | MATCH; RegisteredBanks @+0x68 verified | high |
| 5 | ResourceBank_ForgetResource | 0x1021B4AB8 | 1:0x21B4AB8 | **MISMATCH** (Handle via x8; path x1) | high |
| 6 | PersistentObject_InitializeState | 0x101F8DFFC | 1:0x1F8DFFC | MATCH; params layout verified | high |
| 7 | NodeRef_Create | 0x1033409EC (8-byte wrapper; core 0x103340CF8) | 1:0x33409EC | **MISMATCH** (StringView by value x0/w1, NodeRef in x0) | high |
| 8 | EngineTime_Frequency | 0x10900B408 (double, = 1e9) | 3:0x1C77408 | MATCH (double) | high |
| 9 | MeshAppearance_ReleaseReference | 0x100E1EE08 | 1:0xE1EE08 | `void(MeshAppearance*)` | high |
| 9 | RenderData_Release | 0x1016A38D4 | 1:0x16A38D4 | MATCH `void(RenderData*)` | high |
| 10 | CommunitySystem_Update | 0x1014AD374 | 1:0x14AD374 | MATCH `void(CommunitySystem*, bool)` | high |

Hook risk: all hookable targets above start with `sub sp`/`stp` prologues (no PC-relative instruction in the first 4).
Exceptions: NodeRef_Create 0x1033409EC is 8 bytes (`mov w2,#0; b core`) and RenderData_Release is 0x24 bytes; Codeware
only calls these two, so that is fine. Do not hook the ReadSoundBanksJson guard thunk 0x100DC8CF8 (cbz in first 4).

## 1. ObjectSerializer (Prepare / WriteToBuffer / ReadFromStrean)

- Serializer object (stack, 0x50 bytes): ctor 0x102160AC4 (18 callers), dtor 0x102160F5C. Prepare 0x102161438 has 17
  callers; every caller builds the params on the stack right before it, e.g. PersistencySystem_SetPersistentStateData
  0x103FEBC30 (verified in world.md): 0x103FEBC84 `mov w8,#0x10000000; stur w8,[x29,-0xc8]` (flags@0 = the same
  0x10000000 Codeware forces), `sturb wzr,[..-0xc4]` (cooking@4, byte), `stur w22,[..-0xc0]` (sizeLimit@8 = 0x400000),
  `sturh wzr,[..-0xbc]` (+0xC/+0xD), `sturb #1,[..-0xba]` (+0xE), then `bl 0x102161438(x0=&ser, x1=&params)`.
- Prepare body reads exactly those fields: `ldrb [x20,#0xe]` 0x1021614A0, `ldr w0,[x20,#0x8]` (sizeLimit) 0x1021614B4,
  `ldrb [x20,#0xd]` 0x102161514, `ldr w8,[x20]` (flags) 0x1021615A4, `ldrb [x20,#0x4]` (cooking) 0x1021615AC/0x102161618,
  `ldrb [x20,#0xc]` 0x102161620. mac extra field: `ldr x8,[x20,#0x10]` 0x102161628 (u64/pointer, all callers zero it,
  so the real struct is 0x18 bytes; Codeware only writes +0/+8 in its HookBefore, so its 0x10-byte struct is still
  compatible). Return value is garbage (void); fine for HookBefore. Size 0x218.
- WriteToBuffer 0x1011D6FA4: `mov x22,x8` (sret), ser ctor, params {flags=w1 @0x1011D6FE4, cooking 0, sizeLimit=w2
  @0x1011D6FEC, 0,0,1,0}, Prepare (0x1011D700C), add root object `0x1021616C8(&ser, obj, cls)` (obj=[x0], cls from
  obj->GetType), write `0x102161AB8(&ser, w1=sizeLimit, x8=tmp)` (0x80-byte result, DataBuffer at +0x58), then
  DataBuffer move-ctor 0x10225FC9C(x22, tmp+0x58) and dtor 0x10225FCE8. DataBuffer on mac = 0x28 bytes (RawBuffer 0x20
  + ptr @0x20; ctor 0x10225FAF8 sets alignment 0x10) = SDK size.
  Callers: 0x103A63590 (`x8=out, x0=&handle, w1=-1, w2=0x10000`), 0x103AB2B28.
  Codeware typedef `DataBuffer (*)(const Handle&, uint64_t, uint32_t)`: clang returns the non-trivial DataBuffer via x8,
  x0=&obj, x1=a3 (used as flags), w2=sizeLimit -> MATCH. The Prepare-hook trick works as on Windows. Note: the
  sizeLimit argument is also passed separately to the writer 0x102161AB8 (x1); the hook only changes the params copy.
- ReadFromStream 0x1011D6E58: `mov x19,x8` (sret Handle), `x21=x0` stream, `x20=x1` CClass*. DataBuffer tmp ctor
  0x10225FAF8, `DataBuffer::Serialize(&tmp, stream)` 0x10225FFB8 (read mode = stream flags bit1: ReadWrite vt+0x18 with
  4 bytes (size) then ReadWrite(data,size), exactly what Codeware's DataBufferProxyStream answers), package reader
  (0x10214FA0C ctor, 0x10214FA2C init(data,size), ReadHeader 0x10214FA38), loader 0x102164070/0x1021642CC; if the
  package has exactly one object (0x1011D6F18 `cmp x8,#0x8`): CreateInstance(type) 0x102197AAC, Handle ctor
  0x102104788 into [x8], deserialize into it 0x102164578; else `stp xzr,xzr,[x19]`.
  Callers 0x103A63840, 0x103AB3008, 0x103AB3090 (paired with the two WriteToBuffer users).
  MISMATCH vs `void*(Handle& aObject, BaseStream&, CClass*)`: mac needs
  `Red::Handle<Red::ISerializable> (*)(Red::BaseStream& aStream, Red::CClass* aType)` (x8 = result); the result must be
  constructed into empty storage (the game writes a fresh Handle and does not release the old contents).

## 2. ISerializable_Clone: not found

Searched and rejected (no generic out-of-line `Handle<ISerializable>(x8) Clone(const Handle<ISerializable>& x0)`):
- No function combines the ObjectSerializer write path (ctor 0x102160AC4 / heap factory 0x102162D64 / writers
  0x102161AB8, 0x102161B1C) with any read path (loader ctor 0x102164070, ReadHeader 0x10214FA38, extractor factory
  0x102151414). All their callers were enumerated: they are save/load pairs of individual systems, entity-template
  and appearance code. WriteToBuffer/ReadFromStream are only used by the 0x103A6xxxx/0x103AB2xxx system pair.
- The ISerializable unit 0x102185000-0x102187600 has no such function (feature scan: no sret+CreateInstance+serializer).
- All 47 sret callers of CreateInstance 0x102197AAC were checked: the clone-like ones (0x1013A3DDC, 0x1034615C0,
  0x103DF91D8, 0x1030912C0, ...) are per-class virtual `Clone()` methods taking a raw `this` and doing a shallow
  `cls->vt[+0x58]` Assign; none takes a Handle or deep-copies.
- No "Clone" strings except "OnClone" (TargetFilter_Script; its user 0x102029740 is a field-wise CloneInto).
- The serializer helpers 0x102166BC8/0x102166C4C/0x102166CD0/0x102166D6C (write) and 0x102166E0C/0x102166FDC (read)
  have no callers.
Suggestion only (not an address): compose it from item 1, i.e. `ReadFromStream(DataBufferProxyStream(
WriteToBuffer(obj, 0, 0x539 with the Prepare hook)), obj->GetType())`, which is the same object-serializer round
trip that Codeware's SaveToFile/ReadFromFile already uses.

## 3. ResourceSerializer job scheduling, ExecutePostLoad, OnResourceReady, +0x230

- Async pipeline (serializationBinaryLoader.cpp area): Load 0x102255B4C -> read job -> 0x10225646C (Deserialize
  0x10225921C with x6 = ctx+0x230, then `JobQueue q; q.Wait(depsCounter)` 0x102256830 and dispatch of the lambda
  **0x102257548** "Serialization/WaitForDependencies") -> lambda runs the throttler 0x1021BBAE8 with
  fn = OnDependenciesReady 0x1022575C0, arg = ctx -> OnDependenciesReady calls ExecutePostLoad(ctx+0xB0, ctx+0x1D0,
  &q) 0x102257610 and, when it returns true, dispatches lambda **0x102257830** "Serialization/WaitForEditorPostLoadJobs",
  which invokes the completion callback `cb(ctx+0x230, ctx+0x10, ctx+0x1D0, ctx+0x20, x)` 0x102257884 and destroys ctx.
- SchedulePostLoadJobs / ScheduleFinalizeJobs: both dispatches are inlined (PoolJobs2Data 8-byte capture
  `str ctx,[capture]`, `bl 0x1009D46F0(inst, prio, q+0x10, q+0x18)`, `bl 0x1009D451C`) at 0x102256834..0x1022568B0 and
  0x102257714..0x102257794. These are the only two serializer job dispatches (only 3 `bl 0x1009D46F0` in
  0x102250000-0x102270000; the third is DeferredDataBuffer). They correspond to the Windows SchedulePostLoadJobs (job
  after the dependency wait, before PostLoad) and ScheduleFinalizeJobs (job after the post-load jobs, before the
  callback), matching the Codeware wiki semantics of Resource/Load and Resource/PostLoad.
- Hook alternatives on mac (both handlers are `void(ResourceSerializer** capture x0, const job::RunContext& x1)`;
  the capture is a heap block whose [0] is the ctx and which the handler frees at the end):
  - Resource/Load: HookBefore 0x102257548 (0x78 bytes) or the verified OnDependenciesReady 0x1022575C0 (x0 = ctx).
    No JobQueue argument exists: the listener must run synchronously, or build its own queue from x1
    (JobQueue_ctor_FromGroup 0x1009D4374), which would not be ordered before PostLoad.
  - Resource/PostLoad: HookBefore 0x102257830 (0xA0 bytes). Not reached when ExecutePostLoad returns false (request
    flag +0x58 bit1): OnDependenciesReady then calls the callback directly (0x1022576C4).
- ExecutePostLoad **0x102259888** `bool(x0 = ctx+0xB0, x1 = request (ctx+0x1D0), x2 = JobQueue&)`: iterates the package
  objects (+0x38, count +0x44, stride 0x28) in reverse, calls `obj->vt[+0x30]` (PostLoad) with {disablePreInit =
  request[0x39]}, registers resources in the bank (0x1021B9308) and `q.Wait(res->vt[+0x140]())` 0x102259A34; returns
  `!(request[0x58] & 2)`. Callers 0x102256DFC (sync loader 0x102256CA8) and OnDependenciesReady 0x102257610.
  DB 1:0x21AC670 refuted: it is the RTTI pointer-wrapper parser ("Cannot cross pointer boundary using the RTTI
  wrappers", "Failed to load resource '%hs'").
- OnResourceReady: no out-of-line `void(ResourceSerializer*)`; the finalize sequence (callback + ctx dtor 0x102257F3C
  + Free<PoolResource>) is inlined in 0x10225646C, 0x1022575C0, 0x102257830, 0x102257AFC, 0x1022578D0 (all callers of
  the ctx dtor). DB 1:0x20AB510 refuted: "Scene was not ready" / "Game instance was not ready" code.
- **+0x230 serializables verified**: 0x102256798 `add x22,x20,#0x230` -> `mov x6,x22` -> Deserialize `mov x19,x6`
  0x102259240; append at 0x10225940C-0x102259438 (DynArray_Realloc elem 0x10, align 8, GOT 0x106E25720 =
  `DynArray<THandle<ISerializable>>::MoveAfterReallocation`, size `str w28,[x19,#0xc]`); OnDependenciesReady reads
  `ldr w8,[x19,#0x23c]` 0x102257618 / `ldr x21,[x19,#0x230]` 0x102257620; callback gets `add x0,x19,#0x230` 0x1022576B8.

## 4. SoundBankManager_ReadSoundBanksJson -> 0x100DC8D0C

- `bool(SoundBankManager* x0)`, x1 unused. Only caller is the guard thunk 0x100DC8CF8 (`ldrb w8,[x0,#0x1a8]; cbz ->
  b 0x100DC8D0C; else return 0`, 20 bytes), called from audio init 0x103EB0FF8 with `x0 = audioSystem->[+0x48]`.
- Body loads the static ResourcePath ref 0x108DCC8D8 (initialized by 0x100DCB73C from
  "base\sound\event\soundbanks.json", config "Audio/SoundBanks"), opens it via ResourceDepot::Get 0x1021C6820, parses
  JSON keys "Name", "IsResident", "ResourcePath", builds "base\sound\soundbanks\%s.bnk", allocates 0x18-byte entries from
  PoolAudioSystems {CName @0, bool @8 (`strb w21,[x23,#8]`), ResourcePath @0x10} = SoundBankEntry, wraps them in
  SharedPtr (PoolRefCount {1,1}) and inserts them into the map; returns `parseError == 0` (0x100DC8F18 cset ->
  `ldr w19,[sp,#0x14]` -> `mov x0,x19`).
- **RegisteredBanks @ this+0x68 verified**: 0x100DC8D30 `mov x20,x0`; 0x100DC8FE8 `add x8,x20,#0x68` -> [sp+0x18];
  0x100DC93C4 `ldr x0,[sp,#0x18]` -> insert 0x100DCB1A8(map, &CName, &SharedPtr). The insert uses the SDK HashMap layout
  (indexTable @0, size @8, capacity @0xC, nodes @0x10, stride @0x1C, node {next, hash, key @8, value @0x10}) and the SDK
  CName hasher `lo ^ hi` (0x100DCB1D0).

## 5. ResourceBank_ForgetResource -> 0x1021B4AB8

- Body: exclusive spinlock bank+0x82, removes the entry for `path` from the GameCacheEntry array at bank+0x60 (stride
  0x18, compare via 0x1021B3708, erase 0x1021B627C, `[+0x6c]--`); then locks bank+0x80, looks up the path in the HashMap
  at bank+0 (hash `hi^lo`), upgrades the cached weak handle to a strong Handle into [x8] (casal on the refcount,
  0x1021B4C4C..0x1021B4C7C), unlinks/frees the node (0x1009184B8) and `size--`; on a miss `stp xzr,xzr,[x8]`.
- Callers pass exactly `x0 = loader->[+0x48]` (Codeware's `loader->unk48`), `x1 = path`, `x8 = &out`:
  0x1021B84E8-0x1021B84F4 (in 0x1021B8334, 3 callers) and 0x1021B8EFC-0x1021B8F08 (in 0x1021B8ECC).
- MISMATCH: the Codeware typedef `uintptr_t(uintptr_t bank, Handle<CResource>& out, ResourcePath)` is the MSVC
  hidden-return form. On mac it would pass &out as the path and leave x8 garbage (16-byte write through x8). Use
  `Red::Handle<Red::CResource> (*)(uintptr_t aBank, Red::ResourcePath aPath)`. Size 0x24C.

## 6. PersistentObject_InitializeState -> 0x101F8DFFC

- `void(PersistentObject* x0, PersistentStateParams* x1)`, MATCH. Size 0x318.
- Callers: the gameObject virtual 0x101F87A64 (in 26 vtables) builds the params at sp+0x20 and calls it for the
  object, 0x101F87B70 `add x0,x19,#0x160` (gameObject PersistentObject sub-object; SDK persistentState @0x168), and for
  each component IsA gameComponent, 0x101F87C20 `add x0,comp,#0x90` (gameComponent ConstructCls 0x101F2A148 stores the
  secondary vptr 0x106FB1A98 (offset-to-top -0x90) at +0x90 and zeroes the handle at +0x98). Device components reach
  the hook through this second call, as Codeware expects.
- Params layout verified in the callee: system @0 (`ldr x0,[x20]` 0x101F8E0B0 -> vcall `[vt+0x1b8]` =
  gamePersistencySystem::GetPersistentState 0x103FED0B4, sret x8), mergeStates @8 (`ldrb w8,[x20,#0x8]` 0x101F8E0D4),
  objectType @0x10 (`ldr x22,[x1,#0x10]` 0x101F8E020; scripted classes look up "<Name>PS"), persistentId @0x18
  (`add x22,x20,#0x18` 0x101F8E0B4 -> x1 of GetPersistentState), unk28 @0x28 (`ldr x2,[x20,#0x28]` 0x101F8E0E8,
  `ldr x4,[x20,#0x28]` 0x101F8E178). Caller frame: system 0x101F87AE4, mergeStates 0x101F87AE8, objectType 0x101F87B54,
  PersistentID 0x101F87B68 (`str q0,[sp+0x38]`), unk28 0x101F87B6C (`str xzr,[sp+0x48]`): 0x30 bytes, as in Codeware.
- PersistentObject on mac: vptr @0 (D1/D0 at +0/+8, GetPersistentStateType at +0x10: `ldr x8,[x19]; ldr x8,[x8,#0x10]`
  0x101F8E090), Handle<PersistentState> @8.

## 7. NodeRef_Create -> 0x1033409EC

- `NodeRef(const char* x0, u32 length w1)` = StringView by value (12-byte packed SDK StringView -> x0/x1); the hash comes
  back in x0. 8 bytes: `mov w2,#0; b 0x103340CF8`. 36 callers, e.g. 0x1009C8FE0 (`x0=c_str, w1=len`, result in x0).
- Core parser 0x103340CF8 (1:0x3340CF8, 0x1B4 bytes) `NodeRef(ptr, len, bool w2)`: len 0 -> 0; first char '#'
  (alias lookup 0x10333F75C), '$' (global root), '~' (relative root), anything else -> 0; then '/'-separated segments
  resolved through the NodeRef registry 0x10333C9C0. w2=1 is used only by NodeRef serialization 0x103340BF8. Bool
  variant: 0x103340B94 `bool(ptr, len, NodeRef* x2)`.
- Identity: the game's own global script native "ParseNodeRef" (registration 0x103341DCC, CGlobalFunction thunk
  0x102173AD4, handler 0x1033416BC) does `bl 0x103340CF8(c_str, length, 0)` 0x10334178C and writes x0 to the result.
- MISMATCH: Codeware's `NodeRef*(NodeRef& out, StringView& str)` is the MSVC constructor shape. On mac use
  `Red::NodeRef (*)(const char* aStr, uint32_t aLength)` (or `(Red::StringView)` by value) and assign the result.

## 8. EngineTime_Frequency -> 0x10900B408 (`__DATA,__common`), 3:0x1C77408

- EngineTime natives (registration 0x10214BDF4, CClassStaticFunction thunk 0x10217395C): FromFloat 0x10214C558
  `ldr d0,[0x10900B408]; fmul; fcvtzs`, ToFloat 0x10214C5F0 `scvtf; ldr d1,[0x10900B408]; fdiv; fcvt s0`, ToDouble
  0x10214C694 divides by the same global.
- Only writer: 0x10214C1F0 (called once from 0x1020B5628): `clock_gettime(4)` (GOT 0x106E49F50), stores
  `0x3B9ACA00` to 0x10900B400 and `0x41CDCD6500000000` (= 1e9) to 0x10900B408. Codeware's `RawPtr<.., double>` read
  matches; GetFrequency returns 1000000000 on mac (ticks are nanoseconds).

## 9. MeshAppearance_ReleaseReference -> 0x100E1EE08, RenderData_Release -> 0x1016A38D4

- MeshAppearance ctor 0x100E1F098 (ConstructCls 0x100E20F54 from the "meshMeshAppearance" TTypedClass vtable
  0x106EF0EF8; C++ vtable 0x106EF0D00) and dtor body 0x100E1F5F8 confirm Codeware's Mesh.hpp offsets on mac:
  MaterialData RenderDataPtr @0x48 and Owner @0x50 (zeroed 0x100E1F0F0), MaterialMap @0x68 (DynArrays +0x68/+0x78),
  MaterialJob Counter @0x90 (ctor 0x1009D4824 at 0x100E1F150, dtor 0x1009D4A00 at 0x100E1F614), MaterialLock spinlock
  @0x98, material ref-count u32 @0x9C, ForceCache @0xA0.
- RenderData_Release 0x1016A38D4 `void(RenderData*)`, MATCH: `ldaddal -1` on [x0+8]; at zero tail-calls `[vt+0x8]`
  (deleting dtor). 2528 callers; AddRef twin 0x1016A38C4. The MeshAppearance dtor uses it on +0x48:
  0x100E1F648 `ldr x0,[x19,#0x48]; cbz; bl 0x1016A38D4`, which is the same pattern as Codeware's ResetMaterialCache.
- MeshAppearance_ReleaseReference 0x100E1EE08 `void(MeshAppearance* x0)`: if !ForceCache(+0xA0): exclusive lock
  +0x98, `--refCount(+0x9C)`; at zero: take +0x48, null it, RenderData_Release it (0x100E1EE6C), clear the material map
  (+0x74/+0x84/+0x88, handle releases); unlock (0x100E1EEE0). Counterpart: LoadMaterialSetupAsync 0x100E1F7B4 increments
  +0x9C under the same lock (0x100E1F898-0x100E1F8AC). Callers 0x100E1EDC0, 0x100E1EF10 (token dtor `x0=[x0]`),
  0x100E1EFB4. Not used by current Codeware code (hash only in Library.hpp).

## 10. CommunitySystem_Update -> 0x1014AD374

- `void(CommunitySystem* x0, bool w1)`, MATCH. Size 0x328.
- gameCommunitySystem (registration 0x1014AF0F8, ctor 0x1014AB9DC, single vptr 0x106F2F670, no this-adjust from the
  GameInstance system pointer).
- Called by the "CommunitySystem/Update" job handler 0x1014AF748 (`x0 = system, w1 = [system+0x109]`, 0x1014AF7B0) and
  by 0x1014AD304. Body: lock +0xA1; time budget = DBL_MAX if w1 else 0.001 s (0x1014AD3B0-0x1014AD3D0); processes the
  stream-in/out request array at +0x58 (stride 0x28; GOT 0x106E270D0 =
  `DynArray<game::CommunitySystem::StreamInOutRequestEntry>::MoveAfterReallocation`) until the budget is spent; then,
  under lock +0xA0, drains the request queue at +0x48 (stride 0x20) and dispatches on the type byte 0..3 (0x1014AD5E4).
- That queue is filled by the ICommunitySystem virtuals, e.g. mac vt+0x1C0 0x1014AD028 appends {CName entry @0,
  0 @8, communityID @0x10, type 1 @0x18} under lock +0xA0. So Codeware's "queue request, then Update(system, true)"
  applies the change immediately. Note: Codeware's RawVFunc offsets in CommunitySystem.hpp (0x1B0..0x260) are MSVC
  values; on mac they are +8 (outside this task, not verified slot by slot).
