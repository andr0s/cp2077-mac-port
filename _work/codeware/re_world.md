# re_world: Codeware world / entity / wardrobe layout offsets on macOS 2.3.1 arm64

The binary is `Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077` (UUID A6656ADC-FBE2-36A4-9B9D-B4A9DE645089, image base 0x100000000).
- This was static analysis only, with `re.sh`, `dis.sh` and llvm-objdump.
- All addresses are absolute. Every function cited below is a `LC_FUNCTION_STARTS` start (each was checked with `re.sh func`).
- "fn X" means the function that starts at X.
- "Proof" lists the load/store instruction addresses.
- Scratch output is in `_work/codeware/re_world_tmp/`, about 645 MB, mostly a regenerable full disassembly. It is safe to delete.

## Summary

| # | Field (Codeware decl) | Windows | macOS | Verdict |
|---|---|---|---|---|
| 1 | worldINodeInstance Transform | 0x30 | 0x30 | CONFIRMED |
| 1 | Scale (Vector3) | 0x50 | 0x50 | CONFIRMED |
| 1 | Node (Handle<worldNode>) | 0x60 | 0x60 | CONFIRMED |
| 1 | SetupInfo (`CompiledNodeInstanceSetupInfo**`) | 0x70 | 0x70 | CONFIRMED (null for sector-path instances, see notes) |
| 1 | NodeID (u64) | 0x80 | 0x80 | CONFIRMED |
| 1 | State (u16 bitfield) | 0x88 | 0x88 | CONFIRMED (bits 0,1,2,3,5,9 checked) |
| 1 | worldMeshNodeInstance Mesh | 0x98 | 0x98 | CONFIRMED |
| 1 | worldBendedMeshNodeInstance Mesh | 0xA0 | **0xC0** | **DIFFERENT** |
| 1 | worldStaticDecalNodeInstance Material | 0xA0 | 0xA0 | CONFIRMED |
| 1 | worldFoliageNodeInstance Mesh | 0xB8 | 0xB8 | CONFIRMED |
| 1 | worldEntityNodeInstance Entity | 0xA0 | 0xA0 | CONFIRMED |
| 2 | CompiledNodeInstanceSetupInfo size / node / globalNodeID / nodeIndex | 0x90 / 0x50 / 0x58 / 0x78 | same | CONFIRMED (nodeIndex is a 20-bit bitfield) |
| 2 | other setup fields Codeware uses (transform 0, scale 0x20, secondaryRefPointPosition 0x2C, streamingRefPoint 0x38, proxyNodeID 0x60, secondaryRefPointDistance 0x70, streamingDistance 0x74) | — | same | CONFIRMED (0x2C only as part of a block copy) |
| 2 | worldStreamingSector NodeBuffer | 0x40 | 0x40 | CONFIRMED |
| 2 | NodeBuffer nodeSetups / nodes / nodeRefs | 0x00 (0x28) / 0x28 / 0x38 | same | CONFIRMED |
| 3 | RuntimeSystemWorldStreaming Handle<worldStreamingWorld> | 0x270 | 0x270 | CONFIRMED (class is 0x620 on mac vs 0x3E0, but this field did not move) |
| 4 | gameWardrobeSystem ItemStore HashMap<CName,ItemID> | 0x48 | 0x48 | CONFIRMED |
| 4 | gameWardrobeSystem ItemStore lock | 0xF4 | 0xF4 | CONFIRMED (1-byte lock with the mac encoding; the SDK's mac SharedSpinLock matches it) |
| 5 | entEntity AppearanceName / TemplatePath / ComponentsStorage / TransformComponent / Scene | 0x50 / 0x60 / 0x70 / 0xB0 / 0xB8 | same | CONFIRMED |
| 6 | Raw::EntityID::ToStringDEBUG | — | — | unused (no references) |

## 1. worldINodeInstance and subclasses

### Anchors

**worldINodeInstance**
- Registration: fn 0x1033E6A98. It calls the abstract CClass ctor 0x102196C80 with size 0x90 (`mov w2,#0x90` at 0x1033E6B50). The class global is 0x1090D4678.
- C++ vtable: 0x1071AE2B8. Slot 0 is 0x1033E317C, which returns [0x1090D4678].
- ctor: fn 0x1033E3354, with a duplicate at 0x1033E33B8. D1: fn 0x1033E341C.

**Initialize** — `bool(inst x0, CompiledNodeInstanceSetupInfo* x1, ctx* x2)`, fn 0x1033E354C (1:0x33E354C).
- It is called from:
  - the sector node-instance creation, fn 0x103426538 at 0x1034266D8;
  - the streaming-grid path, fn 0x10337CC2C at 0x10337CD7C.
- It calls vt+0x168 (OnInitialize) at 0x1033E3694.

**Attach / Detach**
- Attach: fn 0x1033E3754 (vt+0x170 inside).
- Detach: fn 0x1033E37A0 (vt+0x178 inside).

**GetNodeTransform** static native: handler fn 0x101F4251C.
- It is registered by CClassStaticFunction 0x10217395C at 0x101F30C3C, right after the "GetNodeTransform" string ref at 0x101F2F734.

### Proof per field

**Transform @0x30**
- Initialize:
  - 0x1033E3578 `ldr q0,[x1]` → 0x1033E357C `str q0,[x0,#0x30]`
  - 0x1033E3580 `ldr q0,[x1,#0x10]` → 0x1033E3584 `str q0,[x0,#0x40]`
- GetNodeTransform 0x101F4251C:
  - FindNode `[scene+0x230]` vt+0x198 with x8 sret (0x101F42640–0x101F42654);
  - then 0x101F42660 `ldr q0,[x8,#0x30]` and 0x101F42674 `ldr q0,[x8,#0x40]`.
- ctor 0x1033E3374–0x1033E3388: zeroes 0x30–0x4B, sets 1.0f at 0x4C (orientation.r) and 0 at 0x50–0x5B. The constant comes from 0x104A62660.

**Scale @0x50**
- 0x1033E3588 `ldr q0,[x1,#0x20]`
- 0x1033E3590 `st1.s {v0}[2],[x0+0x58]`
- 0x1033E3594 `str d0,[x0,#0x50]`

**Node @0x60**
- 0x1033E3598 `ldr x0,[x1,#0x50]` (setup->node), then make Handle 0x1021864F8, then 0x1033E35FC `str q0,[x19,#0x60]`.
- D1 0x1033E3438 `ldr x8,[x0,#0x68]`: releases the handle at 0x60 (its refcount word is at 0x68).

**SetupInfo @0x70**
- 0x1033E3680 `ldur q0,[x20,#0x8]` → 0x1033E3684 `str q0,[x19,#0x70]`. This copies ctx+8 into 0x70 and ctx+0x10 into 0x78.
- Streaming-grid caller:
  - 0x10337CD34 `stp x19,x22,[sp,#0x8]`: ctx+8 = the grid entry.
  - 0x10337CD54 `ldp x1,x0,[x19]`: entry[0] is the setup passed as x1.
- Accessors:
  - 0x10337C344 `ldr x8,[x0]; ldr x0,[x8,#0x50]` (entry → setup → node);
  - 0x10337C794 `ldr x0,[x0]` (entry → setup).
- So `*(inst+0x70)` is the entry and `**(inst+0x70)` is the setup. This is the `CompiledNodeInstanceSetupInfo**` semantics Codeware expects.
- 0x1033E633C also reads `[x0,#0x70]` and then `[entry+0x25]` flags.

**NodeID @0x80**
- 0x1033E3678 `ldr x9,[x21,#0x58]` (setup->globalNodeID) → 0x1033E367C `str x9,[x19,#0x80]`.
- ctor 0x1033E338C/0x1033E3398: GlobalNodeID ctor 0x10333BB50 (`str xzr,[x0]`) on +0x80.

**State @0x88 (u16)**
- ctor 0x1033E339C–0x1033E33A4: `ldrh`, `and #0xf800`, `strh`.
- Initialize:
  - 0x1033E356C–0x1033E3574: sets bit 0 (isInitializing).
  - 0x1033E3664–0x1033E3674: sets bit 9 (isVisible) from ctx+0x19. The sector path fills ctx+0x19 from worldNode.isVisibleInGame @0x30: 0x103426698 `ldrb w20,[x8,#0x30]` → 0x1034266BC.
  - 0x1033E36A8–0x1033E36C0: clears bit 0 and sets bit 1 (isInitialized).
- Attach: 0x1033E3764–0x1033E378C sets `|=0x84`, then clears bit 2 and sets 0x30 (bit 5 = isAttached).
- Detach: 0x1033E37B0–0x1033E37E4 tests bit 5, sets `|=0x88` (bit 3 = isDetaching), then `&=0xFFD7`.
- These match `WorldNodeInstanceNodeState`.

### Subclasses

Each C++ vtable's slot 0 returns the class global that the named registration writes:

| Class | Slot 0 | Class global | Written at |
|---|---|---|---|
| mesh | 0x10334E154 | 0x1090D32F8 | 0x1033516B4 |
| bended | 0x1045E72C0 | 0x10910BF08 | 0x1045E993C |
| decal | 0x1045D5BC0 | 0x10910BE68 | 0x1045D6CC4 |
| foliage | 0x103331734 | 0x1090D3280 | 0x103334530 |
| entity node | 0x100D0E0C4 | 0x108DCBD60 | 0x100D0F6AC |

**worldMeshNodeInstance: Mesh @0x98, CONFIRMED.**
- Registration fn 0x10335166C (size 0x100). vtable 0x1071A0E40. ConstructCls fn 0x103351770 (TTypedClass vtable 0x1071A1020, slot +0xE0).
- The ctor zeroes 0x98–0xB7 (0x1033517A8 `stur q0,[x19,#0x98]`).
- OnInitialize (vt+0x168) is fn 0x10334EFF0:
  1. IsA worldMeshNode: getter 0x10334B8DC → [0x1090D32E0], which the "worldMeshNode" registration 0x10334DBF8 writes.
  2. Reads mesh path node+0x38 (0x10334F060) and loads it through ResourceLoader 0x1021B88B8.
  3. IsA CMesh: getter 0x100E12A0C → [0x108DCCB20], which the "CMesh" registration 0x100E1B3C4 writes.
  4. 0x10334F160 `ldp x9,x8,[x20,#0x98]` / 0x10334F168 `stur q0,[x20,#0x98]`. The old value is released as a Handle.

**worldBendedMeshNodeInstance: Mesh is at +0xC0 on mac (DIFFERENT).**
- Registration fn 0x1045E98F4 (size 0x100). vtable 0x107335A00. ConstructCls fn 0x1045E99F8 (slot 0x107335CA0).
- The Handle<CMesh> is stored by fn 0x1045E8B68 at 0x1045E8B9C and 0x1045E8BB8 (`str q0,[x19,#0xc0]`). That function is called:
  - from fn 0x1045E7A9C at 0x1045E7B50, after IsA CMesh (0x1045E7ADC → 0x100E12A0C);
  - from fn 0x1045E7BAC at 0x1045E7C94, with token->GetResource 0x1021C4D44.
- Both paths are reached from OnInitialize vt+0x168, fn 0x1045E7664 (IsA worldBendedMeshNode at 0x1045E76A4 → [0x10910BF00]).
- The vt+0x150 getter fn 0x1045E75F8 returns a Handle copy of +0xC0 through x8. Every other node-instance class uses the empty default 0x10093FB88 for that slot.
- The D1 fn 0x1045E7478 shows the member layout:
  - +0xC0 is released as a Handle (0x1045E7528–0x1045E755C);
  - +0xA8 is released as a SharedPtr<ResourceToken> (0x1045E7560–0x1045E7594);
  - +0x98 is released as a SharedPtr<ResourceToken> (0x1045E7598–0x1045E75CC), stored at 0x1045E7BEC.
- **On mac, +0xA0 is the refcount pointer of the SharedPtr at +0x98, not a Handle<CMesh>.**
- The layout uses tail padding: ctor 0x1045E9A20 `stur d0(-1),[x19,#0x8c]` puts the first derived u32 at 0x8C, and OnInitialize writes `str w0,[x20,#0x90]` at 0x1045E76FC.
- Under MSVC, that member order (u32, u32, SharedPtr@0x98, SharedPtr@0xA8, u64@0xB8, Handle@0xC0) would also give 0xC0. The Windows value 0xA0 may therefore be stale, but that cannot be verified here.

**worldStaticDecalNodeInstance: Material @0xA0, CONFIRMED.**
- Registration fn 0x1045D6C80 (size 0xD0). vtable 0x107333308. ConstructCls fn 0x1045D6D78, which zeroes 0x90–0xBF and sets u16 0xFFFF at 0xC0.
- OnInitialize fn 0x1045D5FF8:
  - IsA worldStaticDecalNode (0x1045D45A0 → [0x10910BE60]);
  - stores the material token at +0x90 (0x1045D60B0);
  - schedules job fn 0x1045D6DDC (adr at 0x1045D627C/0x1045D6280).
- The job:
  1. token→GetResource (0x1045D6E4C/0x1045D6E50);
  2. IsA IMaterial (0x102F24D0C → [0x1090D12E8], which the "IMaterial" registration 0x102F27C40 writes);
  3. 0x1045D6EC4 and 0x1045D6F5C `str q0,[x22,#0xa0]`.

**worldFoliageNodeInstance: Mesh @0xB8, CONFIRMED.**
- Registration fn 0x1033344E8 (size 0x130). vtable 0x10719F070. ConstructCls fn 0x1033345EC → ctor fn 0x103331834, which zeroes 0xA8–0xC7 (0x103331884).
- The chain to the store:
  1. OnInitialize vt+0x168 is fn 0x103331E04.
  2. It calls fn 0x103331F28, which stores a load-state SharedPtr at +0xA8 and schedules job fn 0x1033346BC.
  3. The job takes resource A from token [[inst+0xA8]+0] and uses it as a CMesh: 0x103334794 `bl 0x100E199D8`, a CMesh GetAppearance-style helper.
  4. At 0x10333496C it passes A as x1 to fn 0x1033325CC.
  5. fn 0x1033325CC stores it at 0x103332648 and 0x103332664 (`stur q0,[x20,#0xb8]`).

**worldEntityNodeInstance: Entity @0xA0, CONFIRMED.**
- Registration fn 0x100D0F664 (size 0xF0). vtable 0x106EDCD78. ConstructCls fn 0x100D0F768, which zeroes 0x90–0xE7.
- vt+0x170 is fn 0x100D0EC58:
  - 0x100D0EE28 `bl 0x100CE0E50` returns a Handle through x8;
  - 0x100D0EE3C `str q0,[x19,#0xa0]`;
  - then 0x100D0EEA4 `ldr x8,[x19,#0xa0]` and 0x100D0EEAC `ldrb w8,[x8,#0x156]`, which reads the entEntity status byte. So the field holds an entEntity.

None of `Raw::WorldMeshNodeInstance`, `Raw::WorldBendedMeshNodeInstance`, `Raw::WorldStaticDecalNodeInstance`, `Raw::WorldFoliageNodeInstance` or `Raw::WorldEntityNodeInstance` is used anywhere in `ports/Codeware/src` outside `Red/WorldNode.hpp`. The bended difference therefore has no effect today.

## 2. CompiledNodeInstanceSetupInfo and the worldStreamingSector node buffer

**Sector NodeBuffer @+0x40: CONFIRMED.**
- StreamingSector PostLoad, fn 0x103426204 (vtable 0x1071B3490 slot +0x30): 0x1034262B8 `add x0,x19,#0x40` → 0x1034262BC `bl 0x1032BCE38`.
- The sector node-instance creator, fn 0x103426538: 0x103426564 `add x22,x0,#0x40` → 0x10342656C `bl 0x1032BC4F8`, the setup range.

**NodeBuffer layout**

The ctor is fn 0x1032BC754 (thunk 0x1032BC80C). It builds:
- a DataBuffer at +0 (0x1032BC778 `bl 0x10225FAF8`);
- DynArrays at +0x28, +0x38 and +0x48;
- `-1` at +0x58.

The dtor is fn 0x1032BC810 (thunk 0x1032BC914).

| Field | Proof |
|---|---|
| nodeSetups DataBuffer @0x00, 0x28 bytes | GetData 0x10225FEC0 is `ldr x0,[x0]` and GetSize 0x10225FF80 is `ldr w0,[x0,#8]`; both are used on buffer+0 at 0x1032BCE48/0x1032BCE54. The dtor tail-calls the DataBuffer dtor 0x10225FCE8 on +0 (0x1032BC910). The next member is at +0x28. |
| nodes DynArray<Handle<worldNode>> @0x28 | PostLoad fn 0x1032BCE38 at 0x1032BCE70: `ldr x8,[x19,#0x28]`, indexed by 16 at 0x1032BCE8C. The dtor at 0x1032BC878–0x1032BC8F0 releases Handles at stride 0x10 and calls realloc with elem size 0x10 / align 8. fn 0x1032BCEE4 reads `[x20,#0x28]` and `[x20,#0x34]` (size). |
| nodeRefs DynArray<u64> @0x38 | dtor 0x1032BC850–0x1032BC86C: `add x20,x19,#0x38`, capacity at +0x40, size at +0x44, realloc 8/8. Serialize fn 0x1032BCA14 at 0x1032BCA48–0x1032BCB00 sorts and dedups the 8-byte keys at `[x20,#0x38]` (count `[x20,#0x44]`), then serializes +0x38 at 0x1032BCB1C. |

**CompiledNodeInstanceSetupInfo**

| Field | Proof |
|---|---|
| size/stride 0x90 | PostLoad 0x1032BCE58 `cmp w20,#0x90`; 0x1032BCE60–0x1032BCE6C divide by 0x90; loop 0x1032BCE90 `str x11,[x9],#0x90`. Range getter fn 0x1032BC4F8 at 0x1032BC524/0x1032BC528 (`mov w8,#0x90; umaddl`). Sector loop 0x1034265D4 `add x22,x22,#0x90`. |
| node @0x50 | 0x1032BCE74 `add x9,x0,#0x50` and 0x1032BCE90: setup->node = nodes[idx].instance. Also 0x1034265E0 and 0x1033E3598 `ldr x0,[x1,#0x50]`. |
| globalNodeID @0x58 | 0x1033E3678 (copied to inst+0x80). Proxy match 0x1033DB5DC `ldr x8,[x0,#0x58]` compared with another setup's +0x60. The ctor runs GlobalNodeID ctor 0x10333BB50 on +0x58 (0x1032BC6A4/0x1032BC6A8). |
| proxyNodeID @0x60 | fn 0x1033DB25C: 0x1033DB5CC `ldr x21,[x0,#0x60]`, then 0x1033DB5E0 `cmp` with another entry's setup+0x58. A setter writes it at 0x1033A77FC. |
| nodeIndex @0x78 | The u64 at 0x78 is a bitfield and the index is its **low 20 bits**. SetNodeIndex fn 0x1032BC3BC: `ldr x8,[x0,#0x78]; and w9,w1,#0xfffff; and x8,#~0xfffff; orr; str`. PostLoad 0x1032BCE80/0x1032BCE84 reads `[setup+0x78] & 0xFFFFF`. Codeware's `uint16_t nodeIndex` reads and writes only the low 16 bits; that is correct while the index is below 65536, the same as on Windows. |
| transform @0, scale @0x20 | Initialize 0x1033E3578/0x1033E3580/0x1033E3588. The ctor-plus-init fn 0x1033E6CE4 sets orientation (0,0,0,1) at +0x10 and scale (1,1,1) at +0x20 (0x1033E6D18; constants 0x104A623E0 and 0x104A62F60). |
| secondaryRefPointPosition @0x2C | Proven only as part of the block copy in fn 0x1033694B8 (`ldr q0,[x23,#0x20]` / `ldr d9,[x23,#0x30]`). |
| streamingRefPoint @0x38 | fn 0x1033694B8: `ldr x20,[x23,#0x38]` / `ldr s10,[x23,#0x40]`. The ctor fn 0x1032BC66C fills 0x38–0x4F with FLT_MAX (0x1032BC698/0x1032BC69C). |
| secondaryRefPointDistance @0x70 | Getter fn 0x1032BC438 `ldr s0,[x0,#0x70]`. HasSecondary fn 0x1032BC3F4 compares it with FLT_MAX. |
| streamingDistance @0x74 | Getter fn 0x1032BC414 `ldr s0,[x0,#0x74]`. Also fn 0x10336FAE8 at 0x10336FB28 and 0x103374138. |

## 3. worldRuntimeSystemWorldStreaming → Handle<worldStreamingWorld> @+0x270: CONFIRMED

**Class**
- Registration fn 0x10341FFE4: CClass ctor with size **0x620** at 0x1034200A8 (Windows is 0x3E0).
- ConstructCls fn 0x1034200E8 → ctor fn 0x103418D20. The ctor stores vtable 0x1071B2D20 and zero-inits the handle at 0x103418F40 `str q0,[x19,#0x270]`.

**Accessor and users**
- fn 0x10341FDFC is `add x0,x0,#0x270; ret` (1:0x341FDFC). It has 11 callers, including:
  - PersistencySystem OnStreamingWorldLoaded fn 0x103FF0F6C: 0x103FF0F8C `ldr x0,[x1,#0x1b0]` (scene index 27) → 0x103FF0F90. It then reads world+0x98 (persistentStateData) at 0x103FF0F9C.
  - MappinSystem fn 0x1042A71A4: 0x1042A71BC/0x1042A71C0, then world+0xE0 (mappinResource) at 0x1042A71E0.
- Both offsets match the mac RTTI layout of worldStreamingWorld.

**Writer**
- Job fn 0x103421DEC is scheduled by "LoadStreamingWorldResource" fn 0x10341AB90 (adr at 0x10341AE2C/0x10341AE30).
- It IsA-checks against 0x10342900C → [0x1090D4DF8], which the "worldStreamingWorld" registration fn 0x10342B938 writes at 0x10342B980.
- It then stores the handle: 0x103421E88 and 0x103421ED4 `str q0,[x23,#0x270]`.

## 4. gameWardrobeSystem: ItemStore @0x48 and lock @0xF4, both CONFIRMED

**Class**
- Registration fn 0x103CE0F24: CClass ctor with size 0xF8 at 0x103CE0FE0.
- ConstructCls fn 0x103CE101C (TTypedClass vtable 0x1072649F8 slot +0xE0). It stores vtable 0x1072646C8 and builds:
  - a HashMap at +0x48: 0xFFFFFFFF at +0x68 (0x103CE1060) and the allocator at +0x70 (0x103CE1068);
  - HashMaps at +0x78 and +0xC0;
  - `strh wzr,[x19,#0xf4]` at 0x103CE10EC.

**Script natives** (from registration fn 0x103CDD3C4) map to these virtuals:

| Native | Handler | Virtual | Implementation |
|---|---|---|---|
| StoreUniqueItemID | 0x103CE02A4 | vt+0x1B8 | fn 0x103CDF470 |
| GetStoredItemID | 0x103CE0354 | vt+0x1C0 | fn 0x103CDF544 |
| GetStoredItemIDs | 0x103CE03F0 | vt+0x1C8 | fn 0x103CDF670 |

**Proof**
- StoreUniqueItemID impl 0x103CDF470:
  - 0x103CDF48C `add x19,x0,#0xf4` and 0x103CDF494 `bl 0x1000020C0` take the write lock (`casalb 0→0x80`);
  - 0x103CDF4BC `add x0,x21,#0x48` → 0x103CDF4CC inserts (map, &CName key, &ItemID) through 0x1021C26FC;
  - 0x103CDF52C `ldclralb #0x80` releases.
- GetStoredItemID impl 0x103CDF544:
  - read lock 0x100002098 on +0xF4 (0x103CDF560/0x103CDF568);
  - probes size +0x50 (0x103CDF588), capacity +0x54 (0x103CDF59C), indexTable +0x48 (0x103CDF5A8), nodes +0x58 (0x103CDF5B8) and stride +0x64 (0x103CDF5BC). That is the RED4ext HashMap layout based at 0x48.
- GetStoredItemIDs impl 0x103CDF670: read lock on +0xF4 (0x103CDF688), 0x103CDF694 `add x0,x19,#0x48`, unlock at 0x103CDF6A4.

**Lock encoding**
- The lock is a 1-byte mac-encoded RW spinlock: writer bit 0x80, readers counted in the low bits.
- The SDK's `SharedSpinLock-inl.hpp` `__APPLE__` branch implements exactly this encoding, so Codeware's `std::unique_lock` on `Red::SharedSpinLock` at +0xF4 interoperates.

## 5. entEntity fields: all CONFIRMED

**AppearanceName @0x50**
- GetCurrentAppearanceName native, fn 0x100C9B7D8: 0x100C9B7E8 `ldr x8,[x0,#0x50]`.
- PrefetchAppearanceChange native, fn 0x100C9B3E8: 0x100C9B474 compares it.
- fn 0x100C97E28 writes it at 0x100C97E4C (from request appearanceName).
- (GetCurrentColorVariantName fn 0x100C9B7F4 reads +0x58.)

**TemplatePath @0x60**
- fn 0x100C97E28 (Entity setup from template; re_entity.md calls it Assemble): 0x100C97E40/0x100C97E44 `ldr x8,[x1]; str x8,[x0,#0x60]`.
- Its caller, fn 0x100CA14B8 (EntityBuilder), builds `params[0]` like this:
  1. 0x100CA16F0 `ldr x0,[x20,#0x90]` (builder->entityTemplate);
  2. 0x100CA16F4 `bl 0x1021B3708`, CResource path getter `ldr x0,[x0,#0x30]`;
  3. 0x100CA16F8 `str x0,[sp,#0x38]`;
  4. then the call at 0x100CA17E8.
- The entity vtable slot +0xC8, fn 0x100C99EB8, is `ldr x0,[x0,#0x60]; ret`.

**ComponentsStorage @0x70**
- The entEntity ctor (ConstructCls fn 0x100C9C120) runs 0x100C9C150 `add x0,x19,#0x70` → 0x100C9C164 `bl 0x100B8C1C8`.
- 0x100B8C1C8 is the entComponentsStorage ctor. It stores vtable 0x106EB2AA8, whose slot 0 returns [0x108DCAE90]; the "entComponentsStorage" registration 0x100B8CE98 writes that global at 0x100B8CEE0.
- GetComponents fn 0x100C91CC8 is `add x0,x0,#0x70` → 0x100B8CCBC `add x0,x0,#0x30`, so components are at entity+0xA0.

**TransformComponent @0xB0**
- GetWorldPosition native, fn 0x100C9BBD4: 0x100C9BBF0 `ldr x8,[x0,#0xb0]`, then component+0xE0.
- GetWorldOrientation native, fn 0x100C9BC4C: 0x100C9BC70.
- Entity::Initialize, fn 0x100C985EC: 0x100C98768 `str x8,[x20,#0xb0]` stores a raw pointer.
- SetWorldTransform, fn 0x100C97A90: `ldr x0,[x0,#0xb0]`.

**Scene @0xB8 (a pointer)**
- Initialize: 0x100C98650 `ldr q0,[x2,#0x40]` → 0x100C98658 `stur q0,[x0,#0xb8]`.
- Attach, fn 0x100C99368: 0x100C993E8 `ldr x8,[x20]` → 0x100C993EC `str x8,[x19,#0xb8]`.
- Dispose, fn 0x100C999E0: 0x100C99A08 `ldr x0,[x19,#0xb8]; cbz`. If the pointer is set it calls scene-dispose 0x100CFB73C; otherwise it uninitializes inline. This is the same check as Codeware's OnDispose.

**Already known, re-proven**
- entityID @0x48: GetEntityID fn 0x100C9AF40 at 0x100C9AF50.
- status @0x156: IsAttached fn 0x100C9B988 at 0x100C9B998.

## 6. Raw::EntityID::ToStringDEBUG (`Red/Debug.hpp`, Windows RVA 0x0E75FC4)

It is unused:
- `grep` over `ports/Codeware/src` and `lib` finds only its own definition.
- No file includes `Red/Debug.hpp`, and `pch.hpp` does not either.
- The only other `ToStringDEBUG` hit is `Red/TweakDB.hpp:125`, which is a script call by name (`CallStatic("gamedataTDBIDHelper","ToStringDEBUG",...)`) and has no raw address.

## Side notes (no change made)

- **Sector-path node instances have no SetupInfo.** fn 0x103426538 builds the Initialize ctx with ctx+8 = 0 (0x1034266B0 `stp x26,xzr,[sp]`), so their SetupInfo is null. Codeware already null-checks it; GetProxyNodeID and GetStreamingDistance return 0 for those instances.
- **The OnDispose scene check never fires.** `Raw::Entity::Scene` is `OffsetPtr<0xB8, Red::world::RuntimeScene>`, with a non-pointer T. So `!scene` in `EntityUninitializeHook::OnDispose` tests the field address (entity+0xB8, never null), not the stored pointer, and that event is never dispatched. This is not a mac offset issue; the offset itself is right.
