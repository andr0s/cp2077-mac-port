# Codeware macOS port: working RE notes (scratch, mine)

Binary UUID A6656ADC..., base 0x100000000. __DATA_CONST vmaddr 0x106DE0000, __DATA vmaddr 0x107394000.

## ScriptingService
- ScriptValidator_Validate 898639042: 0x1021FBF90 (DB had 1:0x3D96BF4 unverified, 0 callers -> wrong).
  Evidence: only caller is script loader 0x10223A748 (vt slot +0x08 of script runtime 0x1070012C0; "Failed to load scripts"),
  call (validator=&stack byte, bundle=&stack bundle, report=x2) after validator ctor 0x1021FBF8C and bundle load 0x1021F0204.
  Body: collect defs from bundle (0x1021EEF74), per def vt+0x38 IsNative, vt+0x18 GetType; dispatch Type(0)->0x1021FC1A4,
  Enum(3)->0x1021FC290, Bitfield(4)->0x1021FC47C, Class(1)->0x1021FC61C(validator,def,report), Function(5)->0x1021FCEE0;
  "Validation failed for %u type(s)" -> report vt+0x10. Returns w0 bool. Sig (void*, ScriptBundle*, ScriptReport*) -> bool. MATCH.
  NOTE: same hash is used by RED4ext loader's own ValidateScripts hook (currently disabled because unverified).
- ScriptValidator_ValidateProperty 560604283: 0x1021FCFB8. Only caller class validator 0x1021FCCDC: x1=scriptProp,
  x2=nativeProp (GetProperty 0x102198B40 result), x0 unused; false -> "Imported property '%hs.%hs' type '%hs' does not match".
  Body: scriptProp->type @+0x90, metaType @+0x2C switch(0..5), nativeProp->type @+0, nativeType vt+0x28 GetType,
  vt+0x10 GetName, vt+0xC8 inner type, rtti vt+0x120 ConvertScriptToNativeName; arrays tail-call CompareType.
- ScriptValidator_CompareType 4224523662: 0x1021FD144 (only reached by tail call from ValidateProperty).
  bool(IType* native x0, ScriptType* script x1): loop on native GetType()-3 switch: Array<->DynArray(4), StaticArray<->5,
  Handle(9)<->2, WeakHandle(10)<->3, ScriptRef(15?)<->6, else name compare (+ConvertScriptToNativeName).
- ScriptValidator_CompareTypeName 1156189659: NO function on macOS - inlined in ValidateProperty (prim/class/handle cases)
  and CompareType default branch. -> re-implement Codeware's name fallback in the macOS ValidateProperty/CompareType wrappers.
- ScriptBundle layout mac: ctor 0x1021ECDAC (thunk 0x1021ECFD4), maps 0x00/0x30/0x60, arrays 0x90..0xF0, map 0x100,
  spinlock byte 0x130, pthread_mutex 0x138 (init recursive), size 0x178.
- ScriptBundle_dtor 3993832650: body 0x1021ECFD8 (only reached via thunk 0x1021ED64C `b`; thunk callers: script loader
  0x10223A748 (+0x08 slot) and 0x10223B05C (+0x10 slot)). Destroys +0x138 mutex, map +0x100, strings +0xF0 (CString 0x20)...
- GetScriptGameInstance 1559305924: 0x101F42944 = handler of global "GetGameInstance": registration 0x101F43A20
  (CName "GetGameInstance", CGlobalFunction ctor thunk 0x102173AD4(obj, name, name, handler=0x101F42944), register 0x1021884A4).
  Body: ++frame->code; builds ScriptGameInstance (0x101F44968) into ret x2 (0x18 bytes). Sig matches native handler.
- ScriptOpCodes_Register 2070546748: 0x102252EF4: calls 0x1021FD4B8(op, handler) {table[op]=handler} for op 0..; table =
  0x10908B798 (verified OpcodeHandlers). Only caller: script runtime ctor 0x10223A3B8 (lazy singleton 0x10223A2AC,
  first used from engine init 0x103D99E44 / LoadScripts). New(48)=0x102251FF0 (reads CClass* from code, CreateInstance
  0x102197AAC, handle), Cast(90)=0x102251B48. Native funcs registered from index 255 on (0x1021FD4F4: counter 0x10900B790 +0xFF).
- CGameFramework_InitializeGameInstance 4007601841: candidate 0x103F0D900 = job lambda "CGameFramework/GameInstance"
  (ctor 0x103F0D2AC dispatches it): (capture x0={framework,u8,x}, RunContext x1): gameInstance=[[x0]+0x10];
  gameInstance->vt[+0x28](gi, &byte, runCtx, x3) (= GameInstance::Initialize 0x101F2D664 -> 0x103314FA0 sret JobHandle).
  Sig (void*, void*) matches a job lambda. confidence medium (Windows identity is a name only).
- CBaseEngine::scriptsLoaded @0x54 confirmed: LoadScripts 0x103D9A028 does strb w0,[engine,#0x54] after load 0x103D9622C;
  engine+0x56 silentValidation, +0x90 validation errors, +0x38 timestamp.
- CRTTISystem mac: types map +0x10, maps +0x40,+0x70,+0xA0(funcs),+0xD0,+0x100; typesLock +0x220 (SDK 0x208), mutex +0x228.

## Entity (entEntity C++ vtable 0x106ED2D20; TTypedClass vtable 0x106ED3140, ctor via slot +0xE0 0x100C9C120)
- Entity_Initialize 3490519617: 0x100C985EC (DB 0x3641A50 wrong). status=1; entityID@0x48<-req+0x20; isDynamic req+0x38;
  req+0x40/0x48 -> ent+0xB8/0xC0; req+0x70 handle -> ent+0xC8; req+0xB0 -> +0x155; req+0xCC -> +0x158; labels
  "Entity/InitializeComponents", "Entity/OnInitialize", "Entity/GatherEventListeners",
  "Entity/ResolveRequestedComponentsAndFinish". (entity x0, JobQueue& x1, request x2). callers 0x100CABD18 (BL+B), 0x100CE283C.
- Entity_Attach 4248638169: 0x100C99368 (DB 0x180C9D8 wrong). status 3 -> 4; [x1]->ent+0xB8; vt+0x160,+0x178.
  callers 0x100CF475C, 0x100CF5FCC, 0x100CFABD8. (entity, params*)
- Entity_Detach 2263681375: 0x100C996C0 (DB 0x15EDE88 wrong). status 5 -> 2; reverse component detach; vt+0x168.
- Entity_Uninitialize 3596356635: 0x100C992C4 (DB 0xCC45EC wrong). status 6 -> 7; vt+0x158; clears +0xB0.
- Entity_Dispose 2515274237: 0x100C999E0 (DB 0xCF5E98 wrong). if status in 2..5: scene? 0x100CFB73C(scene,ent) :
  inline uninit (6->7); then +0x15C |= 8. 34 callers.
- Entity_RequestComponents 2296260874: INLINED on macOS (in Initialize after vt+0x190 OnRequestComponents: per component
  vt+0x210(comp, ctx); same inline loop in Reassemble over the new list). Out-of-line entity-list copy 0x100C97D3C has
  0 callers. -> mac: use upstream's earlier hooks: HookBefore Initialize (Entity/Initialize) and HookBefore Reassemble
  (Entity/Reassemble).
- Entity_Assemble 2182550867: DB 0xCCEDEC = ent::Assembler (entityAssembler.cpp) fn, x0 = assembler, not entity -> wrong.
  not found.
- virtual stubs (ret) precede the methods: +0x110 (Initialize), +0x150 (Uninitialize), +0x158, +0x160 ...

## SDK hashes Codeware pulls in
- UpdateRegistrar_RegisterGroupUpdate (SDK 4254156293, DB unverified 1:0x1FFFF8C = mid-function in an RTTI registration,
  wrong). Real: 0x103D95800: (registrar x0, group w1, CClass* x2, name x3, callback&& x4 (moved, ops at +0x20), u32 w5
  stored in the entry; game callers pass 10, SDK passes 0). Callers: 128 OnRegisterUpdates, e.g. VisionModeSystem
  0x10159F7B4 ("VisionModeSystem/Tick", group 3), WeatherSystem 0x100D5E9E8. RegisterBucketUpdate = 0x103D95924
  (WorkspotSystem 0x1015F7A70; 7 args). StaticEntitySystem's call compiled out on mac until verified.
- CEnum_ToString/FromString/Unserialize + CBaseRTTIType_sub_80..A0 (unverified): avoided by building plugin enums on the
  game's CEnum vtable (0x106FFC2B8; ctor 0x1021A33E0 stores it; read at runtime from native enums in the type map).
- InkSystem: singleton ptr global 0x108D5D830 (set in InkSystem::Initialize 0x104882F7C at 0x104883064; getter 0x10488B8DC)
  -> InkSystem_Instance = 3:0x19C9830. requestsHandler @0x370 (GetSystemRequestsHandler 0x10488A470, native 0x1047CAB74),
  layerManagers @0x380 (size @0x38C; 0x10488556C, 0x10488A07C), InkLayerManager.layers @0x38 (0x104940208).

## Entity game-system interfaces (DynamicEntitySystem/StaticEntitySystem/OpenWorldSystem)
- First virtual of IGameSystem-derived interfaces is mac +0x1B8 (IGameSystem OnInitialize +0x1A8 / OnUninitialize +0x1B0).
- gameEntityStubSystem vtable 0x106FBFDB8: +0x1B8 CreateStub 0x101FD71BC = `SharedPtr<EntityStubToken>(this, Request& x1,
  Callback&& x2 (ops @+0x80))` via x8 -> SDK MISMATCH (out-param). +0x1C0 RestoreStub 0x101FD74B0, +0x1C8 CancelStub 0x101FD7620,
  +0x1D0 DeleteStub 0x101FD7734 (stub+0x40 entityID) -> SDK MATCH, +0x1D8 FindStub 0x101FD8D80.
- gameDynamicEntityIDSystem vtable 0x1071DD9A0: +0x1B8 GetNextTransientID 0x1035EDD9C (atomic +0xE0, `add w0,...` -> id in
  w0, zero-extended), +0x1C0 GetNextPersistableID 0x1035EDDD8 (atomic +0xE4) -> SDK MISMATCH (out-param).
- gamePersistencySystem vtable 0x10729DE70: +0x1B8 GetPersistentState 0x103FED0B4 = Handle via x8, (PersistentID& x1, CClass* x2,
  bool w3) -> SDK MISMATCH; +0x1F0 RemoveDynamicEntityState 0x103FF1E3C (this, u32) -> MATCH.
- gameEntitySpawnerEventsBroadcaster vtable 0x106F308B0: +0x1B8 RegisterListener 0x1014C6A80 (returns u32), +0x1C0
  UnregisterListener 0x1014C6C40 -> MATCH.
- gamePopulationSystem vtable 0x106F36420 (ctor 0x101575314):
  +0x1B8 FindEntity 0x10155C9D4: x8 = out Handle (stp [x10+0x30] -> [x20], refcount ++), w1 = id (hash bucket udiv) -> MISMATCH.
  +0x1D8 IsSpawning 0x10155CE0C: `mov x2,#0; b 0x101577000` (search spawn queue this+0x348/+0x354 for [e+0x10]==w1) -> MATCH.
  +0x1F8 RegisterEntity: `b 0x10156BBA4`; reads request [x1]: +0 id, +8 appearance, +0x10 priority, +0x11, +0x12 alwaysSpawned,
  +0x13/0x14 (ldurh), +0x15, +0x18/+0x20 -> exactly PopulationEntityRegisterRequest; returns w0 = bool -> MATCH.
  +0x200 RemoveEntity: `b 0x10156C220`: (this, w1 id stored, x2 a2 stored) -> MATCH.
