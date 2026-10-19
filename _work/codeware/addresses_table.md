| name | hash | status | macOS offset | confidence | evidence / reason |
|---|---|---|---|---|---|
| Main | 240386859 | DB verified | 1:0x344CBFC | - | - |
| GetScriptGameInstance | 1559305924 | **proposed** (new) | 1:0x1F42944 | high | notes.md |
| CBaseEngine_InitEngine | 3273923080 | DB verified | 1:0x3D9E494 | - | - |
| CBaseEngine_LoadGatheredResources | 3729789488 | DB verified | 1:0x3D9EFC8 | - | - |
| CBaseFunction_InternalExecute | 2744524784 | disabled on macOS | - | - | Not used (its declaration is commented out upstream). |
| CBaseStack_ResolveContext | 1745688837 | disabled on macOS | - | - | Not used (its declaration is commented out upstream). |
| CGameFramework_InitializeGameInstance | 4007601841 | **proposed** (new) | 1:0x3F0D900 | medium | notes.md |
| CMesh_GetAppearance | 773462733 | DB verified | 1:0xE199D8 | - | - |
| CMesh_FindAppearance | 3007126651 | DB verified | 1:0xE189F4 | - | - |
| CommunitySystem_Update | 1559565515 | **proposed** (new) | 1:0x14AD374 | high | re_res.md#10, re_quest.md#6 |
| EngineTime_Frequency | 3151104823 | **proposed** (new) | 3:0x1C77408 | high | re_res.md#8 |
| Entity_Attach | 4248638169 | **proposed** (correction) | 1:0xC99368 | high | notes.md |
| Entity_Detach | 2263681375 | **proposed** (correction) | 1:0xC996C0 | high | notes.md |
| Entity_Dispose | 2515274237 | **proposed** (correction) | 1:0xC999E0 | high | notes.md |
| Entity_Initialize | 3490519617 | **proposed** (correction) | 1:0xC985EC | high | notes.md |
| Entity_Assemble | 2182550867 | **proposed** (correction) | 1:0xC97E28 | high | re_entity.md#1 |
| Entity_Reassemble | 1560690857 | DB verified | 1:0xC95744 | - | - |
| Entity_Uninitialize | 3596356635 | **proposed** (correction) | 1:0xC992C4 | high | notes.md |
| Entity_RequestComponents | 2296260874 | disabled on macOS | - | - | Inlined into Entity::Initialize and Entity::Reassemble on macOS (out-of-line copy 0x100C97D3C has no callers). Entity/Initialize and Entity/Reassemble come from hooks on those two functions instead. |
| Entity_ResolveRecordID | 525805301 | **proposed** (new) | 1:0x378A7E8 | high | re_entity.md#2 |
| EntityBuilder_ExtractComponentsJob | 489494088 | **proposed** (correction) | 1:0xCA41E0 | high | re_entity.md#3 |
| EntityBuilder_ScheduleExtractComponentsJob | 437791594 | DB verified | 1:0xCA08A0 | - | - |
| EntitySpawner_SpawnEntity | 3068279668 | **proposed** (new) | 1:0xCDD130 | high | re_entity.md#8a |
| EntitySpawnerToken_dtor | 552341017 | **proposed** (new) | 1:0xCE0D28 | high | re_entity.md#8b |
| EntitySpawnerToken_GetWaitHandle | 2091718968 | disabled on macOS | - | - | Found (1:0xCE0EB0, re_entity.md#8c) but Codeware has no declaration or use. |
| InkLayer_RegisterListener | 4792499 | **proposed** (new) | 1:0x48DC344 | high | re_ui.md#1 |
| InkLayer_AttachWidgetTree | 2050958960 | **proposed** (new) | 1:0x48DB648 | high | re_ui.md#2 |
| InkLogicController_OnInitialize | 2671382832 | **proposed** (new) | 1:0x47CF584 | high | re_ui.md#3 |
| InkLogicController_OnUninitialize | 1159862457 | **proposed** (new) | 1:0x47CF720 | high | re_ui.md#3 |
| InkLogicController_OnArrangeChildrenComplete | 3589084925 | **proposed** (new) | 1:0x47CF848 | high | re_ui.md#3 |
| InkSpawner_FinishAsyncSpawn | 2698985195 | DB verified | 1:0x49799B8 | - | - |
| InkSystem_Instance | 2635337807 | **proposed** (new) | 3:0x19C9830 | high | notes.md, re_ui.md |
| InkSystem_ProcessCharacterEvent | 4284882427 | **proposed** (new) | 1:0x4887524 | high | re_ui.md#7 |
| InkSystem_ProcessInputEvents | 673454961 | **proposed** (new) | 1:0x48842E0 | high | re_ui.md#7 |
| InkWidget_GetLayer | 1667176558 | **proposed** (new) | 1:0x49ADE28 | high | re_ui.md#4 |
| InkWidget_SetLayer | 2516523266 | **proposed** (new) | 1:0x49B18F0 | high | re_ui.md#4 |
| InkWidget_ScriptReparent | 492509908 | **proposed** (new) | 1:0x49B888C | high | re_ui.md#5 |
| InkWidget_ScriptAddChild | 3558023194 | **proposed** (new) | 1:0x49A3084 | high | re_ui.md#5 |
| InkWidget_TriggerEvent | 3357288308 | **proposed** (new) | 1:0x492B550 | medium | re_ui.md#6 |
| InkWidgetLibrary_AsyncSpawnFromExternal | 1396063719 | DB verified | 1:0x4965B38 | - | - |
| InkWidgetLibrary_AsyncSpawnFromLocal | 118698863 | DB verified | 1:0x4965980 | - | - |
| InkWidgetLibrary_SpawnFromExternal | 506278179 | DB verified | 1:0x4965EC0 | - | - |
| InkWidgetLibrary_SpawnFromLocal | 1158555307 | DB verified | 1:0x4965DE0 | - | - |
| IComponent_Toggle | 3370191745 | **proposed** (new) | 1:0xCA633C | high | re_entity.md#4 |
| IPlacedComponent_SetTransform | 1828854026 | **proposed** (correction) | 1:0xC417F0 | high | re_entity.md#5 |
| ISerializable_Clone | 2601784524 | disabled on macOS | - | - | Not found (no generic out-of-line deep clone, re_res.md#2): IScriptable/ISerializable Clone return null on macOS. |
| IScriptable_CheckClass | 2487884020 | disabled on macOS | - | - | Not used (its declaration is commented out upstream). |
| IScriptable_CheckExactClass | 1797004238 | disabled on macOS | - | - | Not used (its declaration is commented out upstream). |
| JobHandle_Wait | 1576079097 | DB verified | 1:0x9D479C | - | - |
| Localization_LoadOnScreens | 3550098299 | DB verified | 1:0x2F67DB4 | - | - |
| MeshAppearance_LoadMaterialSetupAsync | 1419388740 | DB verified | 1:0xE1F7B4 | - | - |
| MeshAppearance_ReleaseReference | 1679234269 | disabled on macOS | - | - | Found (1:0xE1EE08, re_res.md#9) but not used by Codeware. |
| MorphTargetManager_ApplyMorphTarget | 633871992 | **proposed** (correction) | 1:0xC6C0C4 | high | re_entity.md#6 |
| NodeRef_Create | 3491172781 | **proposed** (new) | 1:0x33409EC | high | re_res.md#7 |
| ObjectSerializer_Prepare | 1944983789 | **proposed** (new) | 1:0x2161438 | high | re_res.md#1 |
| ObjectSerializer_WriteToBuffer | 2646156567 | **proposed** (new) | 1:0x11D6FA4 | high | re_res.md#1 |
| ObjectSerializer_ReadFromStrean | 125247621 | **proposed** (new) | 1:0x11D6E58 | high | re_res.md#1 |
| PersistentObject_InitializeState | 3442350580 | **proposed** (new) | 1:0x1F8DFFC | high | re_res.md#6 |
| PhysicsTraceResult_GetHitObject | 2394822845 | **proposed** (new) | 1:0x4092830 | high | re_misc.md#8 |
| PlayerSystem_OnPlayerSpawned | 2050111212 | **proposed** (new) | 1:0x3FD4D70 | medium | re_entity.md#7 |
| PointOfInterestMappin_SetPhase | 803085170 | **proposed** (new) | 1:0x42EAF14 | high | re_quest.md#4 |
| QuestLoader_PhasePreloadCheck | 1567104722 | disabled on macOS | - | - | Inlined on macOS (copy 0x102ED49E0 has no callers); its hook is commented out upstream anyway. |
| QuestPhaseInstance_Initialize | 3423879019 | disabled on macOS | - | - | Found (1:0x2CFBA64, re_quest.md#2) but its hook is commented out upstream. |
| QuestPhaseInstance_ExecuteGraph | 779107884 | disabled on macOS | - | - | Found (1:0x2CFB178, re_quest.md#2) but only used by commented-out code. |
| QuestPhaseInstance_ExecuteNode | 3227858325 | **proposed** (new) | 1:0x2CFD644 | high | re_quest.md#2 |
| QuestsSystem_CreateContext | 3144298192 | **proposed** (new) | 1:0x2EFA150 | medium | re_quest.md#3 |
| RenderData_Release | 2996308699 | **proposed** (new) | 1:0x16A38D4 | high | re_res.md#9 |
| RenderProxy_SetMaterialOverride | 429336746 | disabled on macOS | - | - | Found (1:0x16A85A8, re_misc.md#4) but not used by Codeware. |
| RenderProxy_SetHighlightParams | 1093803822 | disabled on macOS | - | - | Found (1:0x16A83B8) but not used by Codeware. |
| RenderProxy_SetScanningState | 2838044016 | disabled on macOS | - | - | Found (1:0x16A8484) but not used by Codeware. |
| RenderProxy_SetVisibility | 1790971865 | disabled on macOS | - | - | Found (1:0x16A78E4) but not used by Codeware. |
| RenderProxy_UpdateVisibility | 2124419179 | disabled on macOS | - | - | Ambiguous on macOS (two candidates) and not used by Codeware. |
| ResourceBank_ForgetResource | 2288918343 | **proposed** (new) | 1:0x21B4AB8 | high | re_res.md#5 |
| ResourcePath_Create | 3998356057 | DB verified | 1:0x21C90A4 | - | - |
| ResourceSerializer_ExecutePostLoad | 1350901460 | disabled on macOS | - | - | Found (1:0x2259888, re_res.md#3; DB value 1:0x21AC670 refuted) but not used by Codeware. |
| ResourceSerializer_SchedulePostLoadJobs | 3599579551 | disabled on macOS | - | - | Inlined on macOS (dispatch in 0x10225646C): the Resource/Load event is not available. |
| ResourceSerializer_ScheduleFinalizeJobs | 949631709 | disabled on macOS | - | - | Inlined on macOS (dispatch in OnDependenciesReady 0x1022575C0): the Resource/PostLoad event is not available. |
| ResourceSerializer_OnDependenciesReady | 1185093671 | DB verified | 1:0x22575C0 | - | - |
| ResourceSerializer_OnResourceReady | 1147149338 | disabled on macOS | - | - | Inlined on macOS (5 copies); DB value 1:0x20AB510 refuted. Not used. |
| RuntimeEntityRegistry_RegisterEntity | 2840271332 | **proposed** (new) | 1:0xD13C5C | high | re_entity.md#9a |
| RuntimeEntityRegistry_UnregisterEntity | 3878623943 | **proposed** (new) | 1:0xD14084 | high | re_entity.md#9b |
| RuntimeScene_AttachEntity | 3800374702 | disabled on macOS | - | - | Not found; no declaration or use in Codeware. |
| RuntimeScene_DetachEntity | 1732384813 | disabled on macOS | - | - | Not found; no declaration or use in Codeware. |
| RuntimeSystemEntity_AttachEntity | 1576214475 | **proposed** (new) | 1:0xD25E50 | high | re_entity.md#10a |
| RuntimeSystemEntity_DetachEntity | 1737563910 | **proposed** (new) | 1:0xD25E58 | high | re_entity.md#10b |
| RuntimeSystemWeather_GetEnvironmentDefinition | 1471948702 | **proposed** (new) | 1:0xD5AEE0 | high | re_misc.md#6c |
| RuntimeSystemWeather_GetWeatherState | 2679119876 | **proposed** (new) | 1:0xD5AA60 | high | re_misc.md#6d |
| RuntimeSystemWeather_SetWeatherByName | 2334794340 | **proposed** (new) | 1:0xD5C384 | high | re_misc.md#6a |
| RuntimeSystemWeather_SetWeatherByIndex | 1821516328 | **proposed** (new) | 1:0xD597A8 | high | re_misc.md#6b |
| ScriptBundle_dtor | 3993832650 | **proposed** (new) | 1:0x21ECFD8 | high | notes.md |
| ScriptOpCodes_Handlers | 961751128 | DB verified | 3:0x1CF7798 | - | - |
| ScriptOpCodes_Handlers_New | 2669615409 | disabled on macOS | - | - | Declared but not used (Codeware installs its own New/Cast handlers). |
| ScriptOpCodes_Handlers_Cast | 2439126103 | disabled on macOS | - | - | Declared but not used (Codeware installs its own New/Cast handlers). |
| ScriptOpCodes_Register | 2070546748 | **proposed** (new) | 1:0x2252EF4 | high | notes.md |
| ScriptValidator_CompareType | 4224523662 | **proposed** (new) | 1:0x21FD144 | high | notes.md |
| ScriptValidator_CompareTypeName | 1156189659 | disabled on macOS | - | - | Inlined on macOS; Codeware's type-name rules run as a fallback after ValidateProperty/CompareType instead. |
| ScriptValidator_Validate | 898639042 | **proposed** (correction) | 1:0x21FBF90 | high | notes.md |
| ScriptValidator_ValidateProperty | 560604283 | **proposed** (new) | 1:0x21FCFB8 | high | notes.md |
| SoundBankManager_ReadSoundBanksJson | 3961524856 | disabled on macOS | - | - | Found (1:0xDC8D0C, re_res.md#4) but only hooked together with SchedulePostLoadJobs by the Resource/Load controller, which cannot activate on macOS. |
| TagSystem_AssignTag | 1239092911 | **proposed** (new) | 1:0x20CDE7C | high | re_entity.md#11a |
| TagSystem_UnassignTag | 1875775378 | **proposed** (new) | 1:0x20CE358 | high | re_entity.md#11b |
| VehicleGarageComponent_LoadSummonSetup | 1159928678 | **proposed** (new) | 1:0x124FE1C | high | re_misc.md#2 |
| VehicleController_ToggleAuxLights | 1883248814 | **proposed** (new) | 1:0x12E6F44 | medium | re_misc.md#1 |
| VehicleSystem_ToggleGarageVehicle | 3027836941 | **proposed** (new) | 1:0x1369A5C | high | re_misc.md#3 |
| VisionModeSystem_SetBraindanceMode | 1070077985 | disabled on macOS | - | - | Found (1:0x15A0544, 12-byte call-only, re_misc.md#5) but not used by Codeware. |
| VisualController_LoadDependencies | 2912168594 | **proposed** (new) | 1:0xC8B630 | high | re_misc.md#9 |
| WorkspotSystem_GetWorkspotInstance | 2234523611 | disabled on macOS | - | - | Inlined on macOS; the instance map differs (lock +0x4C, map +0x68, keyed by the low 32 bits), so WorkspotGameSystem.GetWorkspotInstance returns null on macOS. |
