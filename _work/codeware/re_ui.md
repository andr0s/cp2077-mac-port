# Codeware UI / Ink: macOS 2.3.1 arm64 address and layout findings

Static analysis only, of the installed `Cyberpunk2077` binary (UUID A6656ADC-…), image base 0x100000000. Nothing was run.
Every address below is the start of a function in LC_FUNCTION_STARTS (checked with `re.sh func`).
None of these functions starts with a PC-relative instruction in its first 4 instructions. Every one is a normal `sub sp`/`stp` prologue, and the smallest is 0x70 bytes.
Scratch dumps and the name→handler mapper `regmap.py` are in `_work/codeware/re_ui_tmp/`. The large `.txt` dumps there can be regenerated and may be deleted.

## Summary table

| name (hash) | mac address | DB form | arm64 signature vs Codeware declaration | conf. |
|---|---|---|---|---|
| InkLayer_RegisterListener (4792499) | 0x1048DC344 | 1:0x48DC344 | MATCH `void(inkLayer* x0, const Handle<IScriptable>& x1)` | high |
| InkLayer_AttachWidgetTree (2050958960) | 0x1048DB648 | 1:0x48DB648 | Args MATCH (x0 layer, x1 `const Handle<inkWidget>&`, x2 `const Handle<inkLayerProxy>&`, strong copy). Returns **void**; the declared uint64 is garbage and is ignored. | high |
| InkLogicController_OnInitialize (2671382832) | 0x1047CF584 | 1:0x47CF584 | MATCH `void(inkWidgetLogicController*)` | high |
| InkLogicController_OnUninitialize (1159862457) | 0x1047CF720 | 1:0x47CF720 | MATCH | high |
| InkLogicController_OnArrangeChildrenComplete (3589084925) | 0x1047CF848 | 1:0x47CF848 | MATCH | high |
| InkWidget_GetLayer (1667176558) | 0x1049ADE28 | 1:0x49ADE28 | **MISMATCH (sret)**: `Handle<inkLayerProxy> (inkWidget* x0)`, with the result in **x8**. x1 is ignored. | high |
| InkWidget_SetLayer (2516523266) | 0x1049B18F0 | 1:0x49B18F0 | MATCH `void(inkWidget* x0, const Handle<inkLayerProxy>& x1)` | high |
| InkWidget_ScriptReparent (492509908) | 0x1049B888C | 1:0x49B888C | MATCH, native handler `(ctx x0, CStackFrame* x1, ret x2, type x3)` | high |
| InkWidget_ScriptAddChild (3558023194) | 0x1049A3084 | 1:0x49A3084 | MATCH, native handler (`inkCompoundWidget.AddChild`) | high |
| InkWidget_TriggerEvent (3357288308) | **0x10492B550** (recommended; generic `<IScriptable>` instantiation) | 1:0x492B550 | MATCH `bool(void* mgr x0, CName x1, Handle<Event> by value → pointer to caller-owned temp in x2)` | high (ABI/semantics), medium (which instantiation Windows used) |
| InkSystem_ProcessInputEvents (673454961) | 0x1048842E0 | 1:0x48842E0 | MATCH `void(InkSystem* x0, uint64 a2 x1, RawInputBuffer* x2)` | high |
| InkSystem_ProcessCharacterEvent (4284882427) | 0x104887524 | 1:0x4887524 | MATCH `bool(InkSystem* x0, EInputKey w1, EInputAction w2)` | high |
| InkSystem_Instance (2635337807) | 0x108D5D830 (data) | 3:0x19C9830 | `InkSystem*` global | high |

Layout results:
- These Codeware offsets are correct on mac:
  - InkSystem: `inputWidget` @0x2E8, `keyboardState` (uint16) @0x2F8, `layerManagers` @0x380.
  - EventManager @widget+0x50.
  - RawInputBuffer: `inputs` @+8, element size 0x40.
  - InkSpawningContext: `request` @0x18, size 0x28.
  - InkSpawningRequest: 0x48 / 0x50 / 0x60 / 0x70 / 0x80 / 0x90 / 0xA0.
- These are **wrong**:
  - `inkWorldLayer.components` is at **0x168** (SDK says 0x170).
  - `IWidgetComponentWrapper::GetGameController` is vtable slot **+0x90** (SDK says +0x98).
  - `InkSpawningInfo.context` is at **0x18** (Codeware says 0x38; Codeware does not read it).
  - The InkSpawningRequest status byte is at 0x1AC (the comment says 0x184; unused).

---

## 1. InkLayer_RegisterListener → 0x1048DC344 (1:0x48DC344)

Evidence:
- **The fields belong to inkLayer.** The inkLayer ctor `0x1048D8900` (vtable 0x107371740) initializes a listener container at **+0xD0** (`0x102100C1C` at 0x1048D8954) and the SharedSpinLock byte at **+0x130** (`strb wzr` at 0x1048D8960).
- **Body** (size 0xDC):
  1. Takes the exclusive lock: `add x19,x0,#0x130; bl 0x1000020C0` at 0x1048DC360/68.
  2. Promotes `[x1]` (weak→strong lock on `{instance @0, refCnt @8}`) at 0x1048DC370–39C.
  3. Calls AddListener `0x102101128(layer+0xD0, &weakCopy)` at 0x1048DC3C8.
  4. Unlocks with `ldclralb` at 0x1048DC408.
- **Only caller: inkLayerProxy::RegisterListener `0x1048E5104`** (BL 0x1048E51B0). That function:
  - Locks proxy+0x38 (`WeakHandle<inkLayer> layer`) and calls this function.
  - Otherwise falls back to the non-layer owner at proxy+0x48 (`0x1048D50EC`, which uses its own container at +0xA8 and lock at +0x108).
- **Proxy+0x38 is an inkLayer.** The proxy ctor `0x1048E3FD4` stores the owner at +0x38 and sets +0x30 = IsA(owner, `inkFullScreenLayer`). The class getter is 0x1048C34D8; global 0x10910DEC0 is registered with that name at 0x1048C38FC.
- **Counterpart UnregisterListener `0x1048DC420`**:
  - Same lock; calls RemoveListener `0x102101D08(layer+0xD0)`.
  - Reached from proxy `0x1048E5264`, from the widget wrapper `0x1049B1804`, and from base `inkIWidgetLogicController::OnUninitialize 0x1047CD05C`.
- **Widget-level wrapper `0x1049B1794(widget, listener)`** does `GetLayer(widget)->RegisterListener(listener)`. It is used by AttachWidgetTree (0x1048DB91C).

Signature: x0 = `inkLayer*`; x1 = reference to a 16-byte handle (it reads `[x1]` and `[x1+8]`; a Handle or WeakHandle both work). Returns nothing. **MATCH.**

Hook: Codeware only calls it.

## 2. InkLayer_AttachWidgetTree → 0x1048DB648 (1:0x48DB648)

Evidence:
- **Single caller is an inkLayer method.** The only caller is `0x1048DAF04`. It creates a new 0x68-byte inkLayerProxy with ctor `0x1048E3FD4` (thunk 0x1048E40DC), passing `weak(this)` → proxy+0x38. That makes `this` an inkLayer, by the IsA(inkFullScreenLayer) logic in §1.
- **Call arguments** from 0x1048DAF04:
  - x1 = `&instance->rootWidget` (`0x1049239D4` = instance+0x38)
  - x2 = `&Handle<proxy>`
- **Body** (size 0x6B4) is an iterative DFS over the tree:
  - Tree access: children via vt+0x118, count via vt+0xE0, child via vt+0xE8.
  - For each widget:
    - **SetLayer** `0x1049B18F0(w, &proxyCopy)` (0x1048DB890).
    - Gathers logic controllers (`0x10486F248`).
    - Sets `ctrl->widget` = w (`0x1047CD280`, ctrl+0x40).
    - Unless InkSystem flag +0x209 is set (`0x1048860D4`), registers the controller through `0x1049B1794` (0x1048DB91C).
  - Then, for each controller: `0x1047CD3F0(ctrl, layer+0x90)` (layer+0x90 at 0x1048DBC14, call at 0x1048DBC24) sets ctrl+0x50 = the layer's own `layerProxy` (SDK inkLayer.layerProxy @0x90). It then calls **ctrl->vt+0x110 (OnInitialize)** (`ldr x8,[x8,#0x110]; blr` at 0x1048DBC30/34).
- **Argument 3 is copied strongly.** x2 is copied with a strong ref++ (`ldp x9,x8,[x21]` … `ldaddal` at 0x1048DB6E0–6F0). Codeware passes `widget->layerProxy` (a Handle), so this is fine.
- **Twin `0x1048D6834` is NOT the inkLayer version.** It is byte-identical except `add x19,x19,#0x70` instead of `#0x90`. It belongs to the non-layer proxy owner (proxy+0x48) and is reached from `0x1048D6168`.

Signature: `void(inkLayer*, const Handle<inkWidget>&, const Handle<inkLayerProxy>&)`. The arguments MATCH. The Windows declaration returns `uint64`, but the mac function returns void, so x0 is garbage. Codeware ignores the result.

## 3. InkLogicController OnInitialize / OnUninitialize / OnArrangeChildrenComplete

**Class and vtables.** The class is registered at 0x1047D3B6C: `CClass(0x108D38160, "inkWidgetLogicController", 0x78, 2)`, with TTypedClass vtable 0x107359358. Its slot +0xE0 (ConstructCls `0x1047D3BA8`) stores the **C++ vtable address point 0x107359118**:

| slot | address | role |
|---|---|---|
| +0x110 | **0x1047CF584** | OnInitialize |
| +0x118 | **0x1047CF720** | OnUninitialize |
| +0x120 | **0x1047CF848** | OnArrangeChildrenComplete |

These slots are the same as the SDK's mac layout (Windows 0x108/0x110/0x118 + 8), so Codeware's `inkComponent` overrides land correctly.

The base `inkIWidgetLogicController` vtable is 0x107358FE0:
- +0x110: `ret` stub 0x1047CD058
- +0x118: `0x1047CD05C`, which unregisters the listener via the layerProxy at +0x50 (or the owner at +0x60)
- +0x120: `__cxa_pure_virtual`

**How the leaf functions are used.** They appear in 82 / 181 / 197 vtables and have 46 / 13 / 0 direct BL callers (derived overrides calling the base). The game calls slot +0x110 from AttachWidgetTree.

**0x1047CF584 OnInitialize** (0x10C bytes):
1. If `audioMetadataName` (+0x70) is set, it locks the widget (+0x40) and calls `0x1048331A0`.
2. It then calls script event `"OnInitialize"` (hash 0x21EB649E5A76AF36) via `0x100A8D654(this, &{CName,1}, &bool)`. That helper is a call-by-short-name returning bool, with 102 callers.

**0x1047CF720 OnUninitialize** (0x98 bytes): fires `"OnUninitialize"`, then calls base `0x1047CD05C(this)`.

**0x1047CF848 OnArrangeChildrenComplete** (0xB0 bytes):
1. `InkSystem::Get()`.
2. If `[ink+0x209]` is set, it returns.
3. Otherwise it fires `"OnArrangeChildrenComplete"`.

**Unreferenced duplicates:** `0x1047CF690` (OnInitialize without the audio step) and `0x1047CF7B8` (OnUninitialize without the base call). They have no callers and no vtable or data references. Do not use them.

Signature: `void(this x0)` for all three. **MATCH.** Hook: Codeware fully hooks OnInitialize. Its prologue is `sub/stp/stp/stp` with no PC-relative instructions. One inline hook covers both the vtable and direct callers.

## 4. InkWidget_GetLayer → 0x1049ADE28 / InkWidget_SetLayer → 0x1049B18F0

The inkWidget class is registered at 0x1049B9008: `CClass(0x108DA0280,"inkWidget",0x200,2)`, global 0x10910E4D8. Its C++ vtable is 0x107386870 (set by the dtor `0x1049AAA04`). `layerProxy` @+0x148 is confirmed: the dtor releases it at 0x1049AAA80.

**GetLayer 0x1049ADE28** (0x2D4 bytes):
- `mov x19, x8` (sret) at 0x1049ADE48.
- Fast path: if `widget+0x148` is set, return a copy.
- Otherwise it walks ancestors through `parentWidget` (weak @+0x168/+0x170) under `parentLock` +0x1F4 (0x1049ADF28–F64).
- **Side effect:** it caches the found proxy into **widget+0x148** (`str q0,[x22]` at 0x1049AE010/0x1049AE02C, with x22 = widget+0x148).
- It returns a copy through x8 (`stp x8,x9,[x19]` at 0x1049AE0D0, ref++ at 0x1049AE0DC).
- It has 36 callers, including the listener wrappers 0x1049B1794 and 0x1049B1804.
- 0x1049B1874 returns `proxy+0x58` (the resource), not the layer.

**MISMATCH:** the Windows form `void(inkWidget*, Handle&)` is an MSVC member-function sret. On mac, declare `Red::Handle<Red::inkLayerProxy> (*)(Red::inkWidget*)`. Calling the Windows form makes the callee write through a garbage x8.

After the call, `widget->layerProxy` is already set. `layerProxy = GetLayer(widget)` leaves the refcount correct.

**SetLayer 0x1049B18F0** (0xF0 bytes) is a non-recursive per-widget setter (the recursion lives in AttachWidgetTree):
1. Address of the field: `add x8,x0,#0x148` at 0x1049B1908.
2. Strong copy of `[x1]` (0x1049B190C–91C), store at 0x1049B192C/0x1049B1948, then release the old value.
3. Exclusive lock on widget+0x1F5 (0x1049B1984). If `propertyManager` (+0x40) exists, calls `0x104981E6C(pm, &weakSelf)`. Unlocks at 0x1049B19C8.

It has 19 callers, including AttachWidgetTree 0x1048DB890. No other function writes +0x148 except GetLayer's cache and the dtor. **MATCH.**

## 5. InkWidget_ScriptReparent → 0x1049B888C, InkWidget_ScriptAddChild → 0x1049A3084

The mapping comes from `re_ui_tmp/regmap.py`, which pairs each CName with its guarded CClassFunction ctor (thunk 0x102173860, x4 = handler, x5 = 0, w6 = 0).

**Reparent:**
- Registration: in the inkWidget register function `0x1049A414C` (called at 0x1049B9064 with x0 = `[0x10910E4D8]`), CName `"Reparent"` (0x106DB12AF) is built at 0x1049A5700–08. The guard is 0x108DA07D8; its slow path at 0x1049A789C calls the ctor at 0x1049A78D8 with **x4 = 0x1049B888C** (function object 0x108DA4060). AddFunction is at 0x1049A5728.
- Body (0x150 bytes):
  1. Reads `wref<inkCompoundWidget>` into sp+0x20 and `Int32` (default −1).
  2. `++frame->code`.
  3. Calls native `inkWidget::Reparent 0x1049AD7C0(widget, &Handle parent, w2 index)` at 0x1049B8988. That native removes the child from the old parent (`0x1049A1444`) and adds it with `0x1049A1300`. It never touches layers.
  4. Does not use ret/type.
- It is also called directly by the `inkWidgetRef.Reparent` forwarder `0x1049FE2A0` (0x1049FE434/4CC) with ctx = the referenced widget, so a hook covers both paths.

**AddChild:**
- Registration: in the inkCompoundWidget register function `0x1049A02EC` (called at 0x1049A3AE8; class at 0x1049A3A24 is `CClass(0x108D9EDB0,"inkCompoundWidget",0x230,2)`), CName `"AddChild"` (0x106DB958B) is built at 0x1049A04E4–EC. The guard is 0x108D9F098; its slow path at 0x1049A08C0 calls the ctor at 0x1049A08F4 with **x4 = 0x1049A3084** (function object 0x108D9F2C0).
- Body (0x150 bytes):
  1. Reads a CName and runs `++code`.
  2. `CRTTISystem::Get()->vt+0x108` (GetClass).
  3. CreateInstance `0x102197AAC`, then an inkWidget cast.
  4. Native `AddChild 0x1049A10A4(this, &Handle)`: `children(+0x200)->vt+0x100`, then `child->SetParent 0x1049AD700`. No layer handling.
  5. Writes a wref result into x2.
- Sibling `AddChildWidget` handler: **0x1049A31D4** (1:0x49A31D4). It also appends through 0x1049A10A4. Use it only if the Windows hash meant AddChildWidget.
- `inkCompoundWidget.children` is @+0x200. inkMultiChildren (ConstructCls 0x104A1AA54, vtable 0x10738E4C0) has lock byte +0x30, `DynArray children` +0x38 and size 0x48, all matching the SDK. That is what `AfterAddChildFromScript` reads.

Both handlers: `(IScriptable* x0, CStackFrame* x1, void* ret x2, IType* x3)`. **MATCH.** Both are suitable for HookAfter (clean prologue).

## 6. InkWidget_TriggerEvent → 0x10492B550 (recommended); EventManager @widget+0x50 confirmed

**The +0x50 offset:**
- inkWidget::OnPress (vtable slot +0x198 → 0x1049B1D68) does `add x19,x0,#0x50` at 0x1049B1D8C.
- It then calls `TriggerEvent<PointerEvent> 0x1049BE828(x19, CName"OnPress", x2=&tmpHandle)` at 0x1049B1DF0. The *caller* releases the temporary afterwards (0x1049B1DF4–E24).
- The inkWidget dtor destroys the object at +0x50 with `0x104866B8C` (0x1049AAD20).
- The CallCustomCallback native `0x1049B4C04` also uses widget+0x50.

**What every instantiation does (each is 0xEC bytes):**
1. `slot = 0x104866EC8(mgr, name)`.
2. Copies `[x2]` and dispatches.
3. Returns `w0 = (slot != 0)`.
4. Never destroys the by-value argument (the Itanium ABI makes the caller destroy it).

**Instantiations found:**
- **0x10492B550 `<IScriptable>`** (generic):
  - Chain: dispatch 0x10492C064 → invoker 0x10492C2EC → type `THandle<IScriptable>` (0x101FC91EC).
  - Caller chain: `0x10492B4B8(widget, CName, Handle&)` = `if (name) TriggerEvent(widget+0x50, …)`. That is called by `0x1049338B0`, which is called by the script native **`inkMenusState.DispatchEvent`** handler 0x1049340BC (registered in 0x104930B10, `"DispatchEvent"`, ctor at 0x104931170).
- **0x1049BE828 `<ink::PointerEvent>`**: 13 callers (inkWidget slots +0x198…+0x200: OnPress, OnRelease, OnHoverOver, …).
- **0x1049BDED0 `<ink::CharacterEvent>`**: slot +0x190, `"OnCharacterKey"`.
- Also present: FocusEvent 0x1049BD578, Widget 0x1049BCBC0, and a CName-argument variant 0x1049BEEB4.

**Why any of them works for Codeware.** The IScriptable and PointerEvent dispatch helpers and script invokers are instruction-identical apart from call targets. Script listeners are first looked up by **short function name** (flag 1 → `0x102174088`). The event type only feeds the fallback signature-hash lookup (`0x10488D3AC`). So any instantiation delivers Codeware's `inkKeyInputEvent` to `OnInputKey` listeners. The generic `<IScriptable>` one best matches `Handle<Event>`.

**Which instantiation Windows used** cannot be decided from the mac binary (that is why it is medium). The ABI of all of them is identical.

**ABI.** On arm64, a by-value `Handle<T>` (non-trivially copyable) is passed as a pointer to a temp in x2, and the caller destroys it. Clang produces exactly this from Codeware's `bool(*)(void*, CName, Handle<Event>)`. **MATCH.** Codeware only calls it.

## 7. InkSystem_ProcessInputEvents → 0x1048842E0, InkSystem_ProcessCharacterEvent → 0x104887524

**ProcessInputEvents** (0xC94 bytes):
1. Takes `x0 = this, x1 = a2, x2 = buffer` (0x104884308–310).
2. Forwards a2 to `0x1048B72EC([this+0x308], a2)`.
3. **`ldr w8,[x20,#0x38C]; cbz` → returns if `layerManagers` is empty** (0x10488432C). Codeware's OnRawInput repeats this same precondition.
4. Calls `buf->vt+0x10` (GetInputs) at 0x104884478–484.
5. Loops with stride 0x40: key +0, action +4 (3 = axis), value +8, mouse +0xC/+0x10, +0x20, +0x28 (0x1048844B8–528).
6. Per input, calls `ProcessCharacterEvent(this, key, action)` (`ldp w1,w2,[x24]` 0x104884BD4, BL 0x104884BDC).

Its sole caller is the InkSystem tick `0x1048841DC` (BL 0x104884228). That tick has the same `(this, ctx, buffer)` arguments and is called from the engine at 0x103D99B6C.

**RawInputBuffer** is created at 0x103D99AC4:
- Size 0x20, vtable 0x10726FEB0 (D1, D0, GetInputs = `0x100643F9C: add x0,x0,#8`).
- `inputs` @+8; size read at 0x103D99B78 (`[x22,#0x14]`).

This matches Codeware's struct. Signature **MATCH**; HookBefore-safe.

**ProcessCharacterEvent** (0x2CC bytes):
1. Locks `inputWidget` (weak @0x2E8/0x2F0, 0x104887540–580).
2. Keyboard state: `add x21,x0,#0x2F8` (0x104887594).
3. Shift test `0x1048B4EC4`.
4. Creates an `inkCharacterEvent` (0x98 bytes, vtable 0x10736BAE0, ctor 0x1048A8244 at 0x104887618 or 0x1048A82BC at 0x1048877E0).
5. Dispatches through `widget->vt+0x190` (0x10488762C–638), i.e. `OnCharacterKey` → `TriggerEvent<CharacterEvent>`.
6. Returns `and w0,w23,w24` (0x1048876AC).

`bool(InkSystem*, EInputKey w1, EInputAction w2)` — **MATCH**; HookBefore-safe.

**InkSystem_Instance 0x108D5D830** (3:0x19C9830):
- Set in Initialize 0x104882F7C at 0x104883064.
- Cleared in the dtor `0x1048829CC` at 0x1048829FC.
- Getter 0x10488B8DC.
- InkSystem has two vptrs: primary 0x107369570, secondary 0x1073695A8 at +8.

## 8. Layout checks

**InkSystem**
- `inputWidget` WeakHandle @**0x2E8**: 0x104887540 `ldr x8,[x0,#0x2F0]`, 0x104887550 `add x8,x0,#0x2E8`, 0x104887574 `ldr q0,[x8]` (weak lock). ✓
- `keyboardState` uint16 @**0x2F8**:
  - Proving instructions: 0x104887594; 0x104887820 (`add x0,x0,#0x2F8; mov w2,#1; bl 0x1048B4F80`); getter 0x104888080; `IsShift` 0x1048B4EC4 (`ldrh; tst #3`).
  - Bits set by `0x1048B4F80` (jump table 0x106C14506):
    - IK_LShift → bit0
    - IK_RShift → bit1
    - IK_LControl → bit2
    - IK_RControl → bit3
    - IK_Alt (0x12) → bit5
    - IK_CapsLock (0x14) → bit8
    - bit4 is never set by the game.
  - Codeware's bitfield lines up with this (`altRight` is the game's generic Alt bit). ✓
- `layerManagers` @0x380, size @0x38C: 0x10488432C, 0x1048865B8–5C0. ✓
- `inkInputEvent+0x68` = `KeyboardState*` (ctor 0x1048AB198 `str x19,[x0,#0x68]` at 0x1048AB1C0). This is Codeware's `unk68`. ✓

**inkWorldLayer** (class size 0x1B0 at 0x10491B88C; ctor 0x104917F54; vtable 0x1073799C8)
- **`components` is at 0x168, not 0x170. MISMATCH with the SDK.**
- **Cause:** inkLayer's data ends at +0x149 (inkLayer ctor `strb wzr,[x19,#0x148]` at 0x1048D8988). Itanium tail-padding reuse places the derived members:

  | offset | field (mac) |
  |---|---|
  | +0x149 | byte |
  | +0x150 | DynArray |
  | +0x160/+0x161 | 2 bytes; +0x160 is the components lock |
  | **+0x168** | `DynArray<SharedPtr<IWidgetComponentWrapper>>` |
  | +0x178 | DynArray |
  | +0x188 | byte |
  | +0x190 | DynArray |
  | +0x1A0 | qword |
  | +0x1A8 | byte |

- **Ctor proof:** it initializes the arrays at +0x150/+0x168/+0x178/+0x190 (0x104917F7C/FA4/FC8/FF0) and zeroes the bytes at 0x104917F78, 0x104917FA0 (`strh` +0x160) and 0x104917FEC (+0x188).
- **Dtor `0x104918034` proof:** it releases each +0x168 element as a SharedPtr whose object is destroyed via vt+0x10 (GetAllocator) and vt+0x18 (D1) (0x10491815C–1E4).
- **AddComponent `0x104919988` proof:**
  - It locks +0x160 (0x104919A00–A08) and appends to +0x168 (data 0x104919A0C, capacity +0x170 at 0x104919A20, size +0x174 at 0x104919A14).
  - Its only caller, 0x100A43764, builds `worlduiWorldLayerWidget` wrappers (ctor 0x1009F9F10, vtable 0x106E97BF0). So every component is that wrapper type.
- **Fix:** Codeware's GetWorldWidgets needs offset 0x168.

**IWidgetComponentWrapper virtuals on mac**
The base vtable 0x107379330 has 41 slots (0x00–0x140; the SDK declares 39).

| slot | function | finding |
|---|---|---|
| +0x38 | **GetWindow** 0x1009FA0E8 | sret x8 (0x1009FA0F8). Returns a Handle to `component+0x140` (the window widget, set by slot +0x48 0x1009FA2A4 via 0x1009F4B44; accessor 0x1009F48CC). The game calls it the same way (inkWorldLayer 0x10491A1E4–1F4, x8 = sp+0x20). **MATCH** with the SDK. |
| +0x90 | **GetGameController** 0x1009FA654 | sret x8. Copies the Handle at `component+0x150` (0x1009FA6A4 → 0x1009F48D4 `add x0,x0,#0x150`). That field is set only by slot +0x50 0x1009FA38C after IsA `worlduiIWidgetGameController` (getter 0x100A015E8, global 0x108DC8140, name at 0x100A022EC). |
| +0x98 | 0x1009FAC9C | Returns a raw u64 `[component+0x2D0]` (`widgetResource` path) in x0 and never writes x8. inkWorldLayer::AddComponent uses it as a non-zero check (0x1049199B4). |
| +0xE8 | 0x1009FB1C4 | Returns the raw window pointer, not an entity. Codeware does not use it. |

- **The SDK's GetGameController slot +0x98 is a MISMATCH.** Calling it through the SDK would leave Codeware's returned Handle uninitialized. Use slot +0x90.

**InkSpawner** (job 0x104979398, FinishAsyncSpawn 0x1049799B8)

| struct.field | Codeware | mac | evidence | verdict |
|---|---|---|---|---|
| InkSpawningContext (size) | 0x28 | 0x28 | alloc 0x10497947C; vtable 0x107380E60 at 0x10497948C | ✓ |
| InkSpawningContext +0x08 | — | WeakHandle | 0x104979494 | — |
| InkSpawningContext.request (SharedPtr) | 0x18 | 0x18 / 0x20 | store `stp x22,x23,[x0,#0x18]` 0x10497949C; read 0x1049799D0 | ✓ |
| InkSpawningRequest.itemName | 0x48 | 0x48 | 0x104979554 / 0x1049795B0 | ✓ |
| InkSpawningRequest.parentWidget | 0x50 | 0x50 (weak) | 0x104979518–540 | ✓ |
| InkSpawningRequest.rootWidget | 0x60 | 0x60 | 0x104979A70 | ✓ |
| InkSpawningRequest.gameController | 0x70 | 0x70 | 0x104979AEC | ✓ |
| InkSpawningRequest.library | 0x80 | 0x80 | `add x2,x8,#0x80` 0x10497954C | ✓ |
| InkSpawningRequest.instance | 0x90 | 0x90 | 0x1049799F4 | ✓ |
| InkSpawningRequest.externalLibrary | 0xA0 | 0xA0 | 0x10497950C / 0x104979550 | ✓ |
| InkSpawningRequest status | 0x184 (comment) | **0x1AC** | 0x1049794FC, 0x104979B40 | unused |
| InkSpawningInfo.context | 0x38 | **0x18** | `stur x0,[x29,#-0x40]` with the info at x29−0x58, passed as x0 at 0x104979558 / 0x1049795B4 | MISMATCH, unused by Codeware's hooks |

- `library->path` is CResource.path @0x30. ✓
- `WidgetLibraryItemInstance.rootWidget` @0x38 (0x1049239D4) and `gameController` @0x48 (0x104923AF4) match the SDK. ✓
