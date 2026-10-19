# Codeware entity group: macOS 2.3.1 arm64 (static RE only)

Binary: Steam `Cyberpunk2077` (UUID A6656ADC). Every address below is a function start according to `re.sh func`.
DB form = `1:0x<addr-0x100000000>`. Signature checks compare register use on arm64 with the Codeware (Windows) declaration.
Scratch dumps and helper scripts are in `_work/codeware/re_entity/`. `full.dis.gz` (155 MB, full `__text` disassembly) can be deleted.

## Summary

| name | hash | DB value | mac address (DB form) | arm64 vs Codeware decl | conf. |
|---|---|---|---|---|---|
| Entity_Assemble | 2182550867 | 1:0xCCEDEC refuted | **0x100C97E28 (1:0xC97E28)** | args MATCH; returns void (decl bool, harmless) | high |
| Entity_ResolveRecordID | 525805301 | — | **0x10378A7E8 (1:0x378A7E8)** | **MISMATCH**: `TweakDBID(x0) (Entity* x0 unused, Param* x1)` | high |
| EntityBuilder_ExtractComponentsJob | 489494088 | 1:0xD4DC20 refuted | **0x100CA41E0 (1:0xCA41E0)** | MATCH `(Params* x0, RunContext& x1)` | high |
| IComponent_Toggle | 3370191745 | — | **0x100CA633C (1:0xCA633C)** | MATCH `(IComponent* x0, bool w1)` | high |
| IPlacedComponent_SetTransform | 1828854026 | 1:0x1443324 refuted | **0x100C417F0 (1:0xC417F0)** | MATCH `(IPlacedComponent* x0, const WorldTransform* x1)` | high |
| MorphTargetManager_ApplyMorphTarget | 633871992 | 1:0x1C5226C refuted | **0x100C6C0C4 (1:0xC6C0C4)** | MATCH `(x0, CName x1, CName x2, float s0, bool w3)` | high |
| PlayerSystem_OnPlayerSpawned | 2050111212 | — | **0x103FD4D70 (1:0x3FD4D70)** | MATCH (4 pointer args, return unused) | medium |
| EntitySpawner_SpawnEntity | 3068279668 | — | **0x100CDD130 (1:0xCDD130)** | **MISMATCH**: SharedPtr returned via **x8**; request in x1 | high |
| EntitySpawnerToken_dtor | 552341017 | — | **0x100CE0D28 (1:0xCE0D28)**; game-used alias 0x100CE0DD0 (1:0xCE0DD0) | MATCH (returns this) | high |
| EntitySpawnerToken_GetWaitHandle | 2091718968 | — | **0x100CE0EB0 (1:0xCE0EB0)** | `const JobHandle* (Token* x0)` in x0 (no Codeware decl) | high |
| RuntimeEntityRegistry_RegisterEntity | 2840271332 | — | **0x100D13C5C (1:0xD13C5C)** | MATCH (returns bool, ignorable) | high |
| RuntimeEntityRegistry_UnregisterEntity | 3878623943 | — | **0x100D14084 (1:0xD14084)** | MATCH (returns bool, ignorable) | high |
| RuntimeSystemEntity_AttachEntity | 1576214475 | — | **0x100D25E50 (1:0xD25E50)** | MATCH; 8-byte thunk | high |
| RuntimeSystemEntity_DetachEntity | 1737563910 | — | **0x100D25E58 (1:0xD25E58)** | MATCH; 8-byte thunk | high |
| RuntimeScene_AttachEntity / DetachEntity | 3800374702 / 1732384813 | — | **not found** (see 10c) | no decl in Codeware | — |
| TagSystem_AssignTag | 1239092911 | — | **0x1020CDE7C (1:0x20CDE7C)** | MATCH `(redTagSystem*, EntityID x1, CName x2)` | high |
| TagSystem_UnassignTag | 1875775378 | — | **0x1020CE358 (1:0x20CE358)** | MATCH | high |

Layout checks:
- **gamePuppetBase recordID is at +0x25C on mac, not +0x260 (MISMATCH).**
- vehicleBaseObject recordID +0x638: MATCH.
- EntityBuilder offsets (0x20/0x80/0x90/0xA0/0xB0/0xC0/0xF8/0x110, size 0x118): all MATCH.
- EntityBuilderRequest (instanceData 0xD8, componentPredicate 0xE8): MATCH.
- EntitySpawnerRequest (0x00/0x10/0x30/0x38/0x48/0x78/0xE1, size 0xF0): MATCH.
- EntitySpawnerToken (0x28; entity 0x10, detached 0x20, failed 0x21, aborted 0x22): MATCH.
- RuntimeSystemEntity::Spawner +0x58: MATCH.
- RuntimeSystemEntityAttachRequest (Handle@0, Handle@0x10, Handle@0x20, u16@0x30): MATCH.
- GameTagSystem→redTagSystem +0x48 (embedded): MATCH.

## 1. Entity_Assemble → 0x100C97E28 (1:0xC97E28), size 0xB0

**DB value refuted.** DB 1:0xCCEDEC is the function that holds the `entityAssembler.cpp` assert path (ref at 0x100CCF0AC). Its x0 is a sorted key/value array (`[x0]` keys, `[x0+0x10]` values, count `+0xC`), not an Entity.

**Evidence**
- The job lambda 0x100CBA5E8 has descriptor label "EntityTemplate_Assemble" (0x100CB33EC in dispatcher 0x100CB3134; descriptor 0x10759C3E0). It locks the builder WeakPtr and calls the builder assemble step 0x100CA14B8 (`Handle<Entity>` sret x8, builder x0).
- 0x100CA14B8 builds a stack AssembleParams block:

  | params offset | content | source |
  |---|---|---|
  | +0x00 | template path | `0x1021B3708(builder+0x90)` |
  | +0x08 / +0x10 | appearance names | req+0x28 / req+0x30 |
  | +0x18 | locked instanceData | req+0xD8/0xE0 |
  | +0x20 | componentPredicate copy | req+0xE8 |
  | +0x50 | components | moved from builder+0xC0 |
  | +0x60 | visual tags | moved from builder+0xF8 |

  It then calls `0x100C97E28(builder->entity@0xB0, &params)` at 0x100CA17E8.
- 0x100C97E28 writes entity+0x60 (templatePath), +0x50 (appearanceName) and +0x58, and assigns the visual tags to entity+0x138 (0x1020CCCFC).
- It then calls vt+0x1B0 (base 0x100C97F20). That adds params+0x50 components into ComponentsStorage entity+0x70 (0x100B8C73C), filtered by the predicate. After that it applies instance data.
- It does not write entityID (+0x48). Initialize 0x100C985EC sets that later, which matches Codeware's "Entity ID not assigned yet".
- Second caller: proxy-entity vt+0x1B0 override 0x100CABB20 calls it for its child at 0x100CABC74.

**Signature:** `void (Entity* x0, const AssembleParams* x1)`. The args MATCH Windows `bool(Entity*, uintptr_t)`. On mac w0 is not set on all paths. Both callers (0x100CA17EC, 0x100CABC78) ignore w0, so the HookAfter return value is harmless.

**Hook:** prologue `sub sp / stp / stp / add x29`, no PC-relative instruction in the first four.

## 2. Entity_ResolveRecordID → 0x10378A7E8 (1:0x378A7E8), size 0x94

**Body:**
```
if (!x1) return 0;
for (h in param->modifiers /* DynArray<Handle>: data +0x30, size +0x3C */)
    if (IsA(h, gameRecordIdSpawnModifier)) return h->recordID /* +0x30 */;
return 0;
```
x0 is never read. Class getters:
- gameRecordIdSpawnModifier: 0x101407738 (global 0x108FEEEC0, registration 0x101407898);
- populationPopulationSpawnParameter: 0x101406EEC (0x108FEEEB8, registration 0x1014073D0).

**Callers**
- gamePuppet vt+0x148 0x103778574. Entity::Initialize calls vt+0x148 at 0x100C988D8 with `&request->populationParam` (req+0x90, loaded at 0x100C988A0).
- gamePuppet vt+0x150 (OnInitialize) 0x10377865C: `SetRecordID(this, ResolveRecordID(this, param))` at 0x1037786D4/0x1037786E0.
- 0x104137570: same pattern in another puppet subclass.
- The vehicle code inlines the same loop: 0x1012A8654 writes `[this+0x638]` at 0x1012A86C8.

**Signature MISMATCH.**
- mac: `TweakDBID (Entity* x0 /*unused*/, PopulationSpawnParameter* x1)`, result in x0.
- Windows `void(Entity*, TweakDBID& out, Param*)` is the MSVC hidden return of a member function.
- With the Windows typedef, x1 = &recordID is read as the param, so it dereferences `[&id+0x3C]`.
- Fix on mac: `RecordID = fn(entity, param)`.

**Hook:** the first instruction is `cbz x1` (PC-relative). Codeware only calls this function.

**Record ID offsets**

| class | Codeware | mac | evidence |
|---|---|---|---|
| gamePuppetBase | 0x260 | **0x25C (MISMATCH)** | ctor 0x10378A2D4: `add x8,x0,#0x25c; str xzr,[x8]` (0x10378A2F4)<br>native "GetRecordID" handler 0x10378A87C: `add x8,x0,#0x25c; ldr x8,[x8]` (0x10378A88C), registered by 0x1037899FC for class 0x1087D8258 (gamePuppetBase, size 0x268)<br>setter 0x10378A7D0: `str x1,[x0+0x25c]` (0x10378A7E0) |
| vehicleBaseObject | 0x638 | **0x638 (MATCH)** | native GetRecordID 0x1012C4840: `ldr x8,[x0,#0x638]` (0x1012C4850), registered in 0x10129EF18 (strings "archetype", "isVehicleOnStateLocked", "GetRecordID")<br>init store at 0x1012A86C8 |

- Why the puppet offset moves: Itanium tail padding. gameTimeDilatable's data ends at 0x25C.
- Effect of using +0x260 on mac: an 8-byte write corrupts the upper half of the record ID plus 4 padding bytes, and the EntityTarget record-ID filters never match.

## 3. EntityBuilder_ExtractComponentsJob → 0x100CA41E0 (1:0xCA41E0), size 0x1218

**DB value refuted.** DB 1:0xD4DC20 is 0x100D4DC20, a RuntimeSystemEntityTransforms vtable function (string "RuntimeSystemEntityTransforms: %u components scheduled…"). It is also wrong for Entity_RequestComponents.

**Evidence**
- ScheduleExtractComponentsJob 0x100CA08A0 (label "EntityBuilder_ExtractComponents" at 0x100CA0A64, descriptor 0x1075993A0) stores 0x100CA41E0 as the JobInstance handler (adrp/add 0x100CA0998/0x100CA099C).
- The job target is a heap 0x18-byte params block (alloc 0x100CA09B0, fill 0x100CA09BC–0x100CA09E4; weak count incremented at 0x100CA0974):
  - `{EntityBuilder* @0, WeakPtr{instance @8, refCount @0x10}}`.
- The params are dispatched via 0x1009D46F0. There are no BL callers; the job runner calls the handler through the stored pointer, so patching the entry is valid.
- The body is the JobShim with the lambda inlined:
  - RunContext prologue (`[x1+0x20]` → 0x1009D4AF8 / 0x1009E5F94);
  - locks the params WeakPtr (0x100CA4234–0x100CA425C) and skips everything if it is expired;
  - extracts the objects and fills the builder fields listed below;
  - **frees the params at exit (0x100CA53B0).**

**Signature:** `void (EntityBuilderJobParams* x0, const job::RunContext& x1)`, MATCH. HookBefore (Codeware's choice) is fine. A HookAfter would read freed params.

**Hook:** clean prologue.

**EntityBuilder layout (all MATCH)**
- self WeakPtr @0: 0x100CA08C4 / 0x100CA08F0.
- entityExtractor @0x10: 0x100CA42A8.
- appearance @0x20: definition @0x20 (0x100CA49A8, 0x100CA4A84), resource @0x30 (0x100CA49C0), extractor @0x40 (0x100CA46E8).
- appearances @0x80: stride 0x30, count @0x8C, element extractor @+0x20 (0x100CA4B2C, 0x100CA4B84–0x100CA4B90).
- entityTemplate @0x90: set at 0x100C9FCD4 in init 0x100C9FC98; read at 0x100CA42C0.
- request @0xA0: 0x100C9FD44 / 0x100CA14E8.
- entity @0xB0: 0x100CA430C; getter 0x100CA193C.
- components @0xC0: 0x100CA43BC / 0x100CA1768.
- visualTags @0xF8: 0x100CA4A80 (TagList add 0x1020CC8C4) / 0x100CA17B4.
- flags @0x110: set at 0x100C9FDCC; `tbz #4` InitializeVisualComponents at 0x100CA5014; `tst #0xC` at 0x100CA1870.
- size 0x118: allocation at 0x100CA1988.

**EntityBuilderRequest (MATCH)**
- byte @0xD0: 0x100CA14EC, 0x100CE2908.
- instanceData WeakHandle @0xD8, refcount @0xE0: 0x100CA16AC–0x100CA16D4; dtor 0x100C9FDB0.
- componentPredicate FlexCallback 0xE8..0x118 (ops @0x100/0x108, flags @0x110): 0x100CA152C–0x100CA153C; dtor 0x100C9FD88.
- base EntityInitializeRequest dtor: 0x100AE5918.
- A byte at 0x11A (inside unk118) is read at 0x100CB319C.

## 4. IComponent_Toggle → 0x100CA633C (1:0xCA633C), size 0x28

**Body:**
```
if (c->isEnabled@0x8B != w1) {
    c->isEnabled@0x8B = w1;
    if (c->flags@0x88 & 2 /*attached*/) tail-call c->vt[+0x1E8](c, w1);
}
```
It has 128 BL/B callers.

**Script natives** (entIComponent registration 0x100CA56EC):
- "IsEnabled" handler 0x100CA7100 returns byte +0x8B;
- "Toggle" handler 0x100CA711C contains the same sequence **inline** (0x100CA7180–0x100CA71B0).

So on mac, a HookAfter does not see toggles called from script. 0x100CA62D8 is another inline copy.

**Signature:** `void (IComponent* x0, bool w1)`, MATCH.

**Hook risk:** the function is 40 bytes, and its 3rd instruction `b.eq 0x100CA6354` is PC-relative inside the first 16 bytes, so the trampoline must relocate it. It ends in `br x2`.

## 5. IPlacedComponent_SetTransform → 0x100C417F0 (1:0xC417F0), size 0xCC

**DB value refuted.** DB 1:0x1443324 is libpng code (png_image_read strings).

**Body:**
- compare the WorldTransform (x1: fixed-point position ints @0..0xB, quaternion @0x10) with localTransform (comp+0xC0 position, +0xD0 orientation);
- if they differ, copy it in;
- if `[comp+0x50]` (owner) is set, call 0x100C91DD0(owner) to schedule a transform update.

**Evidence**
- The game's own `Entity::SetWorldTransform(const WorldTransform&)` 0x100C97A90 is `ldr x0,[x0,#0xb0]; cbz; b 0x100C417F0`, with 25 callers. That is exactly Codeware's EntityEx::SetWorldTransform.
- Siblings:
  - 0x100C41574: float `Transform` version (wrapper 0x100C97A80);
  - 0x100C413E8: position only;
  - 0x100C414C8: orientation only;
  - 0x100C418BC: WorldTransform copy *without* the owner notify (1 caller, not suitable).

**Signature:** `void (IPlacedComponent* x0, const WorldTransform* x1)`, MATCH.

**Hook:** clean; Codeware only calls it.

## 6. MorphTargetManager_ApplyMorphTarget → 0x100C6C0C4 (1:0xC6C0C4), size 0x3CC

**DB value refuted.** DB 1:0x1C5226C is render-target binding validation ("Too many render targets" …).

**No script native exists.** There is no "ApplyMorphTarget" string in the binary. entMorphTargetManagerComponent (class 0x10758F940, C++ vtable 0x106ECED50) registers only "MorphTarget_DEBUG". The script method is Codeware's own EntityEx RTTI method.

**Body**
- weights `DynArray<entMorphTargetWeightEntry{targetName, regionName, weight@0x10}>` at mgr+0x98 (size +0xA4, stride 0x18);
- find the (x1, x2) entry or append one with weight 1.0;
- skip if |Δ| ≤ 0.01, unless w3 is set;
- renormalize the region;
- for each mesh pointer in mgr+0xD8 (count +0xE4), call `0x100C72648(mesh, x1, x2, s0)`;
- store the weight and call 0x100C6BD38.

**Callers:** 9, all in CharacterCustomizationSystem (0x102466968 holds "CharacterCustomizationSystem/PostponedMorphs"). Example call site 0x102452D34–0x102452D48: x1 = target, x2 = region, s0 = weight, w3 = 0.

**Signature:** `void (IComponent* x0, CName x1, CName x2, float s0, bool w3)`, MATCH. clang lowers Codeware's prototype to the same registers.

**Hook:** clean.

## 7. PlayerSystem_OnPlayerSpawned (cp::PlayerSystem::OnPlayerMainObjectSpawned) → 0x103FD4D70 (1:0x3FD4D70), size 0x4E0

**cpPlayerSystem**
- registration 0x103FD6A2C ("cpPlayerSystem", size 0x60500);
- ctor 0x103FCDB84, vtable 0x10729B260;
- +0x250 is the "PlayerSystem - Stubs" spawner, +0x288 the "PlayerSystem - Entities" spawner.

**Call chain**
- 0x103FD4574 (reached from callback thunk 0x103FD740C) builds the player entity spawn from the stub.
- It wraps a libc++ `std::function` closure (vtable 0x10729B718, created at 0x103FD4944).
- The closure's operator() 0x103FD788C (slot +0x30) does `b 0x103FD4D70(this=[c+0x10], entry=[c+0x18], &req=c+0x20, c+0xB0)`.

**What 0x103FD4D70 does**
- takes the system's recursive lock (+0x240..0x247);
- tests the spawn token (`[entry+0x20]` vt+0x20);
- on success:
  - pulls the spawned entity handle (0x101FD1408) into entry+0x10;
  - stores it as the local main object (this+0x604C0) when the player index matches (req+0x52);
  - calls vt+0x210 0x1039A6F60;
  - invokes the request callback (req+0x60/+0x80) with the entity;
- on failure, deletes the stub: `0x10140C6FC(this+0x250, [entry+0x38], "player")`.

**Signature:** `void (PlayerSystem* x0, Entry* x1, Request* x2, void* x3)` against Windows `void*(void*, void*, void*, void*)`. MATCH for HookAfter: all four registers are forwarded and the return value is unused.

**Hook:** clean prologue; the first adrp is at +0x2C. The only entry is a tail call from 0x103FD788C, which is fine for a HookAfter.

**Confidence: medium.** No name string exists. The identity comes from the stub→entity→completion flow and the 4-argument arity.

## 8. EntitySpawner (ent::EntitySpawner)

### 8a. SpawnEntity → 0x100CDD130 (1:0xCDD130), size 0x1DC

**Evidence**
- The verified game wrapper `EntitySpawner_SpawnFromTemplate` 0x1014084B0 builds an ent request at sp+0x40 and calls this function at 0x101408700 with `x0=[this+0x50]`, `x1=&req`, `x8=&SharedPtr`. There are 9 callers in total.
- Body:
  - lock spawner+0x7B;
  - if `!req->templatePath@0`, or the shutdown flag +0x7C is set: write an empty SharedPtr to [x8];
  - otherwise `spawner->factory@0x68->vt[+0x10](x8, req)` creates the token (0x100CDC618);
  - then it queues {token, the callback copied from req+0x48, id = ++spawner+0x70, priority req+0xE1}.

**Signature MISMATCH.**
- mac: `SharedPtr<EntitySpawnerToken> (sret x8) (EntitySpawner* x0, const EntitySpawnerRequest& x1)`.
- Windows `void*(spawner, SharedPtr& out, const Request&)` passes MSVC's hidden return as arg 2.
- With the Windows typedef, x1 = &token (empty) is read as the request. templatePath is then 0, so the early-out writes `stp xzr,xzr,[x8]` through a **stale x8**.
- Fix: `using Fn = Red::SharedPtr<Red::EntitySpawnerToken> (*)(Red::EntitySpawner*, const Red::EntitySpawnerRequest&);`

**Hook:** clean.

### 8b. EntitySpawnerToken_dtor → body 0x100CE0D28 (1:0xCE0D28), alias 0x100CE0DD0 (1:0xCE0DD0)

- The alias is a 4-byte thunk `b 0x100CE0D28`, used by all 78 call sites. It is tiny and PC-relative; call it, do not hook it.
- Body:
  1. If entity @+0x10 has status (+0x156) in 2..5, call Entity::Dispose.
  2. Release the entity Handle (+0x10/+0x18).
  3. Release the SharedPtr task (+0/+8; task dtor 0x100CE1588).
  4. Return this.
- Used by the SharedPtr release followed by `Free<PoolEngine>` (0x100CDD2D8–0x100CDD2E8).
- Signature `Token* (Token* x0)` against Windows `void(Token*)`: MATCH.

**Token layout (0x28): MATCH**
- allocated at 0x100CDC784 (PoolEngine);
- ctor 0x100CE0CE4: task SharedPtr @0/8, entity Handle @0x10, detached @0x20 (= req+0x78), clears +0x21/+0x22;
- IsDone 0x100CE0DE8 reads @0x10, @0x20, @0x21 and entity status;
- IsFailed 0x100CE0E38 reads +0x21;
- IsAborted 0x100CE0E30 reads +0x22;
- Abort 0x100CE0F50 sets +0x22.

### 8c. EntitySpawnerToken_GetWaitHandle → 0x100CE0EB0 (1:0xCE0EB0), size 0xA0

- Returns `const JobHandle*` in x0:
  - `&task->+0x78` (via 0x100CE184C), or
  - a lazily built static empty JobHandle 0x1075A0FB0 (Counter ctor 0x1009D4824).
- Its 4 callers pass the result to JobHandle_Join 0x1009D4A3C (0x102539F64→0x102539F70, 0x1015C8E68→0x1015C8E74).
- mac signature: `const JobHandle& (Token* x0)`. Codeware has no declaration.
- Hook: `adrp` is the 4th instruction.

### 8d. Request and system layouts

**EntitySpawnerRequest (0xF0): MATCH**

The ent request is filled in 0x1014084B0 (req = sp+0x40):

| field | offset | instruction |
|---|---|---|
| templatePath | +0 | 0x1014086A8 |
| transform: position / quaternion | +0x10 / +0x20 | 0x10140856C–0x101408574 |
| entityID | +0x30 | 0x101408578 |
| appearanceName + unk40 | +0x38 | 0x101408580 |
| callback FlexCallback | +0x48 (ops +0x60, flags +0x70) | 0x1014086E8–0x1014086F0 |
| detached | +0x78 | 0x101408594 |
| dissolve | +0x79 | 0x101408520 |
| priority | +0xE1 | 0x101408588 |

The token factory 0x100CDC618 also reads +0x7B, +0x80, +0x90, a second FlexCallback at +0xA0, and +0xE0/+0xE2/+0xE4.

**The pass2 request is a different type.** pass2's request (appearance +0xA0, recordID +0xC0 on mac) is the outer **game::EntitySpawner** request consumed by 0x1014084B0. That function converts it into this ent request:

| game request | ent request |
|---|---|
| +0xA0 | +0x38 |
| +0xC9 | +0xE1 |
| !+0xCA | +0x78 |
| +0xB8 / +0xC0 | +0x80 / +0x88 |
| +0x90 | +0x90 |
| +0x50 | +0xA0 |

There is no conflict between the two.

**RuntimeSystemEntity::Spawner @+0x58: MATCH**
- worldRuntimeSystemEntity vtable 0x106EDEEE8.
- Its vt+0x178 0x100D25388 creates the 0xA0-byte spawner via 0x100CDF3B8 (ctor 0x100CDCE08; init 0x100CDF190 puts the token factory at spawner+0x68).
- It stores the spawner with `str x8,[x20,#0x58]` at 0x100D2540C. The old one is freed via 0x100CDD12C.
- Accessor 0x100D25B34 `ldr x0,[x0,#0x58]` has 15 callers.

## 9. RuntimeEntityRegistry

The registry object: ctor 0x100D14DD4, vtable 0x106EDE178, size 0xB0 (RTTI 176). Lock +0x43, entity map +0x48, listener map +0x80.

### 9a. RegisterEntity → 0x100D13C5C (1:0xD13C5C), size 0x128

- Body:
  1. Lock +0x43.
  2. key = entity->entityID (+0x48), value = WeakHandle (0x1021864F8).
  3. Insert into map +0x48 (0x100D152C0).
  4. Notify via `0x100D13D84(this, ent, 1)`.
  5. Return bool.
- Callers: gameObject::OnAttach 0x101F87D34 (BL at 0x101F87D78; registry = `[[[ent+0xB8]+8]+0x68]`), 0x1020A6C48, 0x1036A9A1C.

### 9b. UnregisterEntity → 0x100D14084 (1:0xD14084), size 0xF8

- Body: take the same lock, find key entity+0x48 in map +0x48, remove the node (0x1009184B8), then notify via `0x100D13D84(this, ent, 0)` and return bool.
- Callers: gameObject::OnDetach 0x101F87E68 (BL at 0x101F87F80), 0x1036A77D8.

**Signature:** `bool (Registry* x0, Entity* x1)` against `void(...)`: MATCH. Both hooks clean.

## 10. RuntimeScene / RuntimeSystemEntity attach and detach

### 10a. RuntimeSystemEntity_AttachEntity → 0x100D25E50 (1:0xD25E50), 8 bytes

- Code: `ldr x0,[x0,#0x48]; b 0x100CF6EBC`. The entity scene lives at rse+0x48 (0x229F50 bytes, created in 0x100D25388).
- 0x100CF6EBC calls `0x100CF6ED8(scene, req, op 1)`, which queues the op or runs 0x100CF5FCC immediately.
- For op 1, 0x100CF5FCC calls `Entity::Attach(entity=[req], {sceneCtx, 0, req+0x10, (u16)req+0x30})` at 0x100CF608C.

**Request layout check**
- The proxy entity's OnAttach 0x100CABFF8 builds the 0x40-byte request at 0x100CAC028–0x100CAC03C:
  - Handle<Entity> @0;
  - zero handles @0x10 and @0x20;
  - u16 @0x30.
- It calls 0x100D25E50 at 0x100CAC0B0.
- The request dtor 0x100CAC378 releases +0x20, +0x10 and +0x00. The op copy 0x100CF74B0 copies +0/+0x10/+0x20/+0x30.

**Signature:** `void (RuntimeSystemEntity* x0, const RuntimeSystemEntityAttachRequest& x1)`, MATCH.

### 10b. RuntimeSystemEntity_DetachEntity → 0x100D25E58 (1:0xD25E58), 8 bytes

- Code: `ldr x0,[x0,#0x48]; b 0x100CF6FD0`.
- 0x100CF6FD0 builds `{handle, 0, 0, 0}` and calls `0x100CF6ED8(scene, &req, op 2)`. Op 2 runs Entity::Detach (0x100CF61C0). Op 3 is detach plus Uninitialize.
- Callers (9) include proxy OnDetach 0x100CAC0D4, which passes &child Handle +0x160 at 0x100CAC118.

**Signature:** `void (RuntimeSystemEntity* x0, const Handle<Entity>& x1)`, MATCH.

Both functions are **tiny** (8 bytes, PC-relative `b` second). Call them; do not hook them.

### 10c. RuntimeScene_AttachEntity / RuntimeScene_DetachEntity: not found

Codeware has no declaration and no caller for these two hashes, so there is no Windows signature to check. For reference only (unverified, do not enable): the scene-level targets 0x100CF6EBC `(scene, const AttachRequest&)` and 0x100CF6FD0 `(scene, const Handle<Entity>&)`. They are reached only through the thunks above.

## 11. TagSystem

### 11a. AssignTag → 0x1020CDE7C (1:0x20CDE7C), size 0x2F4

- Body:
  1. Return if tag == 0.
  2. Lock +0x60.
  3. Find or insert the tag entry (0x1020D0680; tag map at +0x00: size +0x8, nodes +0x10, stride +0x1C).
  4. Notify listeners (map +0x30, lock +0x61, vt+0x10).
  5. Append the EntityID if absent, then notify vt+0x20.
- Its only caller is AssignTags(TagList) 0x1020CE300. That is called from gameObject::OnAttach at 0x101F87DDC with `x0 = GameTagSystem+0x48`, `x1 = ent+0x48`, `x2 = &tags@0x230`.

### 11b. UnassignTag → 0x1020CE358 (1:0x20CE358), size 0x27C

- Same arguments. Takes lock +0x60, finds the tag, swap-removes the EntityID, then notifies listeners.
- Its only caller is UnassignTags 0x1020CE764, which gameObject::OnDetach calls at 0x101F87F64.

**Signature:** `void (redTagSystem* x0, EntityID x1, CName x2)`, MATCH for both. Hooks clean.

**GameTagSystem→redTagSystem +0x48: MATCH (embedded object)**
- GameTagSystem ctor 0x101FA3664: `str x8,[x0],#0x48; b 0x1020CDAEC` (redTagSystem ctor runs on this+0x48).
- Class size 0xB0 (0x101FA3624) = 0x48 + redTagSystem 0x68 (RTTI 104).
- Call sites use `add x0,x0,#0x48` at 0x101F87DD4 and 0x101F87F5C.

## Danger list (fix before enabling)

1. `Raw::Puppet::RecordID` must be **0x25C** on mac. 0x260 corrupts the record ID and breaks the record filters.
2. `Entity_ResolveRecordID` returns the TweakDBID in x0 and takes the param in x1 (`TweakDBID(*)(Entity*, Param*)`). The Windows out-param typedef dereferences garbage.
3. `EntitySpawner_SpawnEntity` returns its SharedPtr through **x8**. The Windows typedef writes 16 bytes through a stale x8.
4. IComponent_Toggle:
   - the trampoline must relocate the `b.eq` at +8;
   - script-initiated toggles are inlined and are not seen by the hook.
5. ExtractComponentsJob frees its params at exit. Keep it HookBefore only.
6. The RuntimeSystemEntity attach/detach entries and the token-dtor alias 0x100CE0DD0 are thunks of 4–8 bytes. Call them only.
7. Entity_Assemble is void on mac. The detour's bool is garbage but ignored by callers.
8. RuntimeScene_AttachEntity and RuntimeScene_DetachEntity are not located. Keep them disabled.
