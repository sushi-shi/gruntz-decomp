#include <Win32.h> // ShowCursor (matching-neutral; ApiCallers.cpp in this aggregate already pulls it)
#include <stdio.h> // engine sprintf (reloc-masked) - LoadGruntzPalette

#include <rva.h>
// Backlog.cpp - engine-label stubs without a class attribution.

// Folded engine-label stubs with a known owning class.
class BootyState {
public:
    void vfunc_9(int);
    void OnActivate_vfunc8();
};
class ButeMgr {
public:
    void ParseAttributeFile();
};
// The help-screen game state. LoadAssets() first chains the base-class asset
// loader (LoadGameAssetNamespaces, reloc-masked external call) which populates
// m_4/m_8, forces the cursor hidden, registers the "STATEZ_HELP" namespace
// through m_8 (cached at m_2c), then pumps a fixed message burst through
// m_4->m_4. Only offsets / code bytes are load-bearing.
struct CHelpAssetSet {          // m_8 points here
    void* Register(char* name); // FUN_0053c030, __thiscall, returns the registered object
};
struct CHelpMsgPump {          // m_4->m_4 points here
    void Pump(int msg, int n); // FUN_0053d4e0, __thiscall message burst
};
struct CHelpAssetRoot { // m_4 / arg1 points here
    char m_pad00[0x4];
    CHelpMsgPump* m_4; // +0x4
};
class CHelpState {
public:
    int LoadAssets(int, int, int);
    int LoadGameAssetNamespaces(int, int, int); // base loader; reloc-masked external call

    char m_pad00[0x4];
    CHelpAssetRoot* m_4; // +0x4
    CHelpAssetSet* m_8;  // +0x8
    char m_pad0c[0x2c - 0xc];
    void* m_2c; // +0x2c  the registered "STATEZ_HELP" object (0 on failure)
};
class CloudHazard {
public:
    void vfunc_20(int, int);
};
class GameLevelState {
public:
    void OnActivate_vfunc8();
};
class Projectile {
public:
    void vfunc_9();
};
class SFManager {
public:
    void SelectBestDevice();
};
class StatusBarItem {
public:
    void vfunc_12(int, int);
    void vfunc_16(int, int, int);
};

// Fake placeholder struct for engine-label stubs the disasm shows are __thiscall
// (they take `this` in ecx) but whose real owning class is not yet recovered.
// Making them members surfaces the implicit `this` and the __thiscall ABI;
// the explicit args are the N/4 from the callee's `ret N`.
struct EngineThisStub {
    void BuildWarpStoneGlitterAnimation();
    void LoadGruntEffectSprites();
    void BuildBootyWalkingGruntz();
    void CheckWarpLetterBonus();
    void BootyState_OnActivate2_vfunc8();
    void LoadChatBoxSprite(int);
    void LoadCreditzStateAssets(int, int, int);
    void InitAttractTitle();
    void LoadCreditzAssets2();
    void BuildWorldLevelKey(int);
    void BuildFortSplashParticles();
    void NotifyFortUnderAttack();
    void LoadGruntTypeTable(int, int, int, int);
    void LoadGruntAbilityTuning(int);
    void BuildGruntLoseItemAnimation();
    void LoadGruntCombatAnimations(int, int, int, int, int, int, int, int);
    void LoadGruntTuningConstants(int);
    void LoadGruntDeathAnimations(int, int);
    void LoadGruntDecayConfig();
    void LoadGruntDecayConfig2();
    void LoadVehicleGruntAnimations();
    void BuildGruntExitAnimation();
    void LoadBombGruntRunConfig(int, int);
    void LoadWandGruntItemConfig();
    void LoadPickupSprites(int, int, int, int, int);
    void LoadBombGruntRunConfig2();
    void LoadWingzGruntSprites(int);
    void LoadFreezeSpellAssets();
    void LoadGruntMovingDeathConfig();
    void LoadTeleporterGooConfig(int);
    void LoadGruntCombatTuning(int, int, int, int, int);
    void LoadFinishLevelSprite(int);
    void LoadMonologoSprite();
    void LoadSaveMessageSprite();
    void LoadStateImages_vfunc8();
    void BuildVersionString(int, int, int, int);
    void LoadMenuSelectSprite(int);
    void LoadDroppedObjectEffects();
    void LoadImageBanks_vfunc29();
    void LoadCursorSprites(int, int);
    void LoadScrollSpeedOptions();
    void LoadSBITextEdges(int);
    void LoadWarlordSprites(int, int);
    int LoadActionTileSprites(int);
    int LoadLevelSounds(int);
    int LoadLevelImages(int);
    void BuildMusicCategoryTable(int);
    void BuildWorldLevelPath(int);
    void LoadLevelEffectSprites();
    void BuildGruntTypeNameTable(int, int, int, int);
    void BuildGruntNamespaceList(int);
    void BuildWarlordNameTable(int);
    void BuildSpriteImageKeyTable(int);
    void BuildAnizKeyTable(int);
    void LoadLevelPreviewScreen();
    void LoadProjectileEffects();
    void BuildToolToyColorKey(int);
    void LookupToolToyColorKey(int);
    int LoadGruntzPalette(int, int);
    void SaveGameFile(int);
    void BuildResourceTabStatusBar(int, int, int, int, int, int, int, int, int, int, int);
    void BuildStatzTabStatusBar(int, int, int, int, int, int, int, int, int, int, int, int, int);
    void BuildStatzTabSmallSprite();
    void BuildMultiplayerTabStatusBar(int, int, int, int, int, int, int, int, int, int, int, int);
    void LoadGameAssetNamespaces(int, int, int);
    void LoadBattlezItemConfig(int);
    void LoadMainStatusBarSprite();
    void UpdateStatusBarTabHighlight(int, int, int);
    void LoadDestructButtonSprite(int);
    void BuildGameTabResumeButton(int);
    void BuildGameTabPauseButton();
    void LoadGooCookingSprite(int);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    void UpdateFallingItemStatusBar(int, int, int);
    void UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(int);
    void UpdateDestructButtonStatusBar2(int);
    void BuildRockBreakInGameText();
    void LoadGruntSpawnConfig(int, int, int, int, int);
    void DebugPrintf();
    void Stub_1c152f(int);
    void Stub_1ccae7(int, int, int);
    void Stub_1ccbfc(int, int, int, int);
};

namespace EngineLabelBacklog {

    void CreateGameObjectByName();
    void __stdcall LoadBootyCheatState(int, int, int);
    void ShowSecretBonusMessage();
    void BuildGruntSprintAnimation();
    void UpdateBootyWalkingGruntz();
    void BuildBootyPerfectAnimation();
    void ShowLevelCompleteMessage();
    void BuildBootyGruntIdleAnimation();
    void __stdcall BuildPowerupIconKeys(int, int);
    void DrawBattleStats();
    void StartUpPrompt();
    void Stub_01fd70();
    void LoadCheatConfig();
    void LoadCustomWorldInfo();
    void HandleFortConquered();
    void __stdcall LoadVehicleGruntSprites(int);
    void __stdcall WireTileSwitchLogic(int, int, int);
    void __stdcall LoadTerrainTileSprites(int, int, int, int, int, int);
    void LoadCameraSprite();
    void __stdcall LoadToyBoxIcon(int, int, int, int, int);
    void LoadExplosionSprites();
    void __stdcall BuildRockBreakParticles(int, int, int, int);
    void __stdcall LoadGruntResurrectTuning(int, int, int);
    void LoadPowerupIconSprites();
    void __stdcall LaunchPortalExe(int);
    void __stdcall BuildLevelRezPath(int, int, int, int, int);
    void LoadHelpBookSprite();
    void __stdcall LoadObjectImageResources(int, int);
    void __stdcall LoadObjectSoundResources(int, int);
    void __stdcall LoadObjectAnimResources(int, int);
    void __stdcall LoadMenuStateAssets(int, int, int);
    void LoadAreaLevelTable();
    void LoadRollingBallHazardSprites();
    void BuildGruntzCrcInfo();
    void BuildNamedGruntTable();
    void __stdcall LoadLevelByMode(int, int);
    void DrawDebugStats();
    void ValidateLevelTiles();
    void __stdcall BuildAssetNamespacePrefixes(int, int, int, int);
    void DrawSaveGameMenu();
    void BuildLevelTitleString();
    void BuildSoundFontPath();
    void Stub_0f90f0();
    void LoadStatzTabToggleSprite();
    void UpdateGruntOvenStatusBar();
    void UpdateChipGrinderStatusBar();
    void UpdateWarpStoneStatusBar();
    void UpdateDestructButtonStatusBar();
    void LoadSwitchDownSprite();
    void LoadSwitchUpSprite();
    void __stdcall LoadBridgeMoveSprites(int);
    void LoadPyramidBridgeSprites();
    void __stdcall BuildStatzTabSmall_vfunc1(int, int, int, int, int, int, int, int, int);
    void SaveScreenshot();
    void FreeAllFonts();
    void FormatGameInfoString();
    void __stdcall BuildVoiceSoundList(int);
    void Stub_11f900();
    void Stub_12abf0();
    void Stub_12d230();
    void Stub_12d460();
    void Stub_12d880();
    void Stub_1315d0();
    void __stdcall Stub_148940(int, int, int, int);
    void _tr_init();
    void _ct_init();
    void Stub_18c780();
    void __stdcall Stub_1bf702(int, int);
    void __stdcall Stub_1bf8f8(int, int);
    void __stdcall Stub_1c1609(int, int);
    void __stdcall Stub_1c176a(int, int);
    void Stub_1c7cb3();
    void __stdcall Stub_1ccb5c(int, int, int);
    void Stub_1d4ee3();
    void __stdcall Stub_1d4f77(int);
    void __stdcall Stub_1d4fbd(int, int, int);
    void __stdcall Stub_1d5029(int, int, int, int);
    void __stdcall Stub_1d513b(int, int, int, int);
} // namespace EngineLabelBacklog

// @confidence: high
// @source: string-xref
// @stub
RVA(0x0000a3b0, 0x6e2)
void EngineLabelBacklog::CreateGameObjectByName() {}

// @confidence: low
// @source: string-xref
// @stub
RVA(0x00018830, 0x380)
void __stdcall EngineLabelBacklog::LoadBootyCheatState(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00018d30, 0xcd)
void BootyState::vfunc_9(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00018f00, 0x4fb)
void EngineLabelBacklog::ShowSecretBonusMessage() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00019540, 0x12a)
void EngineThisStub::BuildWarpStoneGlitterAnimation() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00019920, 0x1c2)
void EngineLabelBacklog::BuildGruntSprintAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001a040, 0x55e)
void EngineThisStub::LoadGruntEffectSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001b450, 0x1ac)
void EngineThisStub::BuildBootyWalkingGruntz() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001b690, 0x7bf)
void EngineLabelBacklog::UpdateBootyWalkingGruntz() {}

// BuildBootyPerfectAnimation @0x01c070 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001c210, 0x4b5)
void EngineThisStub::CheckWarpLetterBonus() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001c8a0, 0xec)
void EngineThisStub::BootyState_OnActivate2_vfunc8() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001c9d0, 0x351)
void EngineLabelBacklog::ShowLevelCompleteMessage() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001ce60, 0x450)
void EngineLabelBacklog::BuildBootyGruntIdleAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001e720, 0x2fe)
void __stdcall EngineLabelBacklog::BuildPowerupIconKeys(int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001ed30, 0x549)
void EngineLabelBacklog::DrawBattleStats() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001f6f0, 0x10b)
void BootyState::OnActivate_vfunc8() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x0001f9b0, 0x2d2)
void EngineLabelBacklog::StartUpPrompt() {}

// @confidence: low
// @source: import:OpenFile
// @stub
RVA(0x0001fd70, 0x45)
void EngineLabelBacklog::Stub_01fd70() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00020f40, 0x188)
void EngineThisStub::LoadChatBoxSprite(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00022e60, 0x1be)
void EngineLabelBacklog::LoadCheatConfig() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00038d20, 0x176)
void EngineThisStub::LoadCreditzStateAssets(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00039570, 0x122)
void EngineThisStub::InitAttractTitle() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x00039dc0, 0x10b)
void EngineThisStub::LoadCreditzAssets2() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0003b7c0, 0x12c)
void EngineLabelBacklog::LoadCustomWorldInfo() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0003c0e0, 0xfb)
void EngineThisStub::BuildWorldLevelKey(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0003f5f0, 0x526)
void EngineLabelBacklog::HandleFortConquered() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00044f80, 0x127)
void EngineThisStub::BuildFortSplashParticles() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00045270, 0x2a8)
void EngineThisStub::NotifyFortUnderAttack() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0004dd50, 0x22c0)
void EngineThisStub::LoadGruntTypeTable(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00050ce0, 0x399)
void __stdcall EngineLabelBacklog::LoadVehicleGruntSprites(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00057100, 0x577)
void EngineThisStub::LoadGruntAbilityTuning(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00057890, 0x19c)
void EngineThisStub::BuildGruntLoseItemAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000597a0, 0x1345)
void EngineThisStub::LoadGruntCombatAnimations(int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0005d210, 0x1443)
void EngineThisStub::LoadGruntTuningConstants(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00060150, 0xd90)
void EngineThisStub::LoadGruntDeathAnimations(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000612a0, 0x23c)
void EngineThisStub::LoadGruntDecayConfig() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x00061570, 0x11d)
void EngineThisStub::LoadGruntDecayConfig2() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00061cb0, 0x34a)
void Projectile::vfunc_9() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00063db0, 0x32f)
void EngineThisStub::LoadVehicleGruntAnimations() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000641b0, 0x2c1)
void EngineThisStub::BuildGruntExitAnimation() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00065630, 0x34b)
void EngineThisStub::LoadBombGruntRunConfig(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00065a60, 0x159)
void EngineThisStub::LoadWandGruntItemConfig() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00065e80, 0x12b8)
void EngineThisStub::LoadPickupSprites(int, int, int, int, int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x00068520, 0x2a2)
void EngineThisStub::LoadBombGruntRunConfig2() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00068880, 0x67c)
void EngineThisStub::LoadWingzGruntSprites(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00069d60, 0x1e1)
void EngineThisStub::LoadFreezeSpellAssets() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0006a060, 0x43d)
void EngineThisStub::LoadGruntMovingDeathConfig() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0006c130, 0xd62)
void __stdcall EngineLabelBacklog::WireTileSwitchLogic(int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0006eb80, 0x5ef)
void EngineThisStub::LoadTeleporterGooConfig(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00075e90, 0x1329)
void __stdcall EngineLabelBacklog::LoadTerrainTileSprites(int, int, int, int, int, int) {}

// LoadCameraSprite @0x078960 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0007a3f0, 0xd7)
void __stdcall EngineLabelBacklog::LoadToyBoxIcon(int, int, int, int, int) {}

// LoadExplosionSprites @0x07b330 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0007b440, 0x3f0)
void __stdcall EngineLabelBacklog::BuildRockBreakParticles(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0007b930, 0x3b7)
void EngineThisStub::LoadGruntCombatTuning(int, int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0007be60, 0x21e)
void __stdcall EngineLabelBacklog::LoadGruntResurrectTuning(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0007c3d0, 0x1ae)
void EngineThisStub::LoadFinishLevelSprite(int) {}

// LoadPowerupIconSprites @0x07c620 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x00090550, 0x1e6)
void __stdcall EngineLabelBacklog::LaunchPortalExe(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00090d10, 0x18e)
void EngineThisStub::LoadMonologoSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00092420, 0xa4)
void EngineThisStub::LoadSaveMessageSprite() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00093d40, 0x473)
void __stdcall EngineLabelBacklog::BuildLevelRezPath(int, int, int, int, int) {}

// @source: decomp-xref
RVA(0x00095090, 0x6e)
int CHelpState::LoadAssets(int a1, int a2, int a3) {
    if (!LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;
    m_2c = m_8->Register("STATEZ_HELP");
    if (!m_2c) {
        return 0;
    }
    m_4->m_4->Pump(0x100, 0x40);
    return 1;
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000997c0, 0x1e7)
void EngineLabelBacklog::LoadHelpBookSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0009a510, 0x275)
void __stdcall EngineLabelBacklog::LoadObjectImageResources(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0009a910, 0x261)
void __stdcall EngineLabelBacklog::LoadObjectSoundResources(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0009ac20, 0x261)
void __stdcall EngineLabelBacklog::LoadObjectAnimResources(int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0009fe50, 0x343)
void __stdcall EngineLabelBacklog::LoadMenuStateAssets(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000a09a0, 0x6a)
void EngineThisStub::LoadStateImages_vfunc8() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000a0d80, 0xd7)
void EngineThisStub::BuildVersionString(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000a11d0, 0x180d)
void EngineLabelBacklog::LoadAreaLevelTable() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000b0140, 0xa7a)
void EngineLabelBacklog::LoadRollingBallHazardSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000b4640, 0x104)
void CloudHazard::vfunc_20(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ba620, 0x14a)
void EngineThisStub::LoadMenuSelectSprite(int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x000bf1d0, 0x249)
void EngineLabelBacklog::BuildGruntzCrcInfo() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000c16b0, 0x3d)
void EngineLabelBacklog::BuildNamedGruntTable() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000c7090, 0x21b)
void EngineThisStub::LoadDroppedObjectEffects() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000ca200, 0xe34)
void __stdcall EngineLabelBacklog::LoadLevelByMode(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000cb800, 0x191)
void GameLevelState::OnActivate_vfunc8() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000cbcc0, 0x16da)
void StatusBarItem::vfunc_12(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ce660, 0x362)
void StatusBarItem::vfunc_16(int, int, int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x000cf770, 0x35e)
void EngineLabelBacklog::DrawDebugStats() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000cffe0, 0x3c)
void EngineThisStub::LoadImageBanks_vfunc29() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000d0120, 0x5d8)
void EngineThisStub::LoadCursorSprites(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000d12b0, 0x2d5)
void EngineThisStub::LoadScrollSpeedOptions() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000d1710, 0x122)
void EngineThisStub::LoadSBITextEdges(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000d2dd0, 0x1de4)
void EngineLabelBacklog::ValidateLevelTiles() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000d65d0, 0x7a4)
void EngineThisStub::LoadWarlordSprites(int, int) {}

// ---------------------------------------------------------------------------
// EngineThisStub::LoadActionTileSprites - per-level ACTION/TILEZ asset loader.
// Reaches the game resource registry through this->m_c (->+0x10): if not forced
// (arg1==0) and the "ACTION" sprite set is already present, bail; otherwise
// register the "ACTION" and "BACK" namespaces (empty-string key), reset the
// severus tile counter, look up the level's "TILEZ" sprite set off this->m_28,
// and wire it into the registry through a virtual slot. Only offsets / code
// bytes are load-bearing; helpers are reloc-masked externals.
//
// The registry object embedded at this->m_c->m_10: Has() probes a named set
// (FUN_00555550, returns found-flag), Register() adds a namespace
// (FUN_00555360), and a virtual slot (index 18) installs the loaded TILEZ set.
struct CActionResRegistry {
    int Has(char* szName);                    // FUN_00555550 __thiscall, ret found
    void Register(char* szName, char* szKey); // FUN_00555360 __thiscall
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void InstallTileSet(void* set, char* empty, char* key); // slot 18 (+0x48)
};
struct CActionResMgr { // this->m_c points here; +0x10 is the registry
    char m_pad00[0x10];
    CActionResRegistry* m_10; // +0x10
};
struct CTileSetSource {                // this->m_28 points here
    void* LookupTileSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};
DATA(0x002bf37c)
extern int g_severusCounterA;
extern "C" char g_emptyString[]; // 0x6293f4

// Typed view of `this` for this loader: m_c is the resource manager, m_28 the
// level's tile-set source. (EngineThisStub is the shared placeholder owner.)
struct CActionTileOwner {
    char m_pad00[0xc];
    CActionResMgr* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    CTileSetSource* m_28; // +0x28
};

// @confidence: high
// @source: decomp-xref
// Byte-exact (100%). The real return type is int (BOOL): each guard is `return 0`
// reusing the just-zeroed eax (no normalizing mov) and the success path is
// `mov eax,1` - declaring the function `void` tail-merges the identical bare
// epilogues and never emits `mov eax,1`, capping ~85%; the int return reproduces
// retail's inline per-site epilogues + the eax=1 tail. The typed-view-of-`this`
// (CActionTileOwner) steers cl to retail's edx (not esi) for the final m_c load.
// (Ghidra demangled the symbol as `...QAEXH@Z` = void from the relocs; the
// RVA-keyed delinker still pairs the int-return body at this address.)
RVA(0x000db600, 0x8f)
int EngineThisStub::LoadActionTileSprites(int force) {
    CActionTileOwner* self = (CActionTileOwner*)this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && self->m_c->m_10->Has("ACTION")) {
        return 1;
    }

    self->m_c->m_10->Register("ACTION", g_emptyString);
    self->m_c->m_10->Register("BACK", g_emptyString);
    g_severusCounterA = 0;

    void* tiles = self->m_28->LookupTileSet("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_c->m_10->InstallTileSet(tiles, g_emptyString, "_");
    return 1;
}

// ---------------------------------------------------------------------------
// EngineThisStub::LoadLevelSounds - per-level SOUNDZ asset loader. Sibling of the
// two tile/image loaders above; same Lookup/Register registry idiom but reaches
// a DIFFERENT registry object embedded at this->m_c->+0x28 (not ->m_10), whose
// Has/Register/Install are plain non-virtual __thiscall helpers (not the +0x48
// vtable slot). If not forced (arg1==0) and the "LEVEL" set is already present,
// bail; otherwise register the "LEVEL" namespace (key "_"), look up the level's
// "SOUNDZ" set off this->m_28, and install it (key "_"). No severus reset here.
// Only offsets / code bytes are load-bearing; helpers are reloc-masked externals.
struct CSoundResRegistry {           // this->m_c->+0x28 points here
    int Has(char* szName);                                 // FUN_004583c0 __thiscall, ret found
    void Register(char* szName, char* szKey);              // FUN_00557c70 __thiscall
    void Install(void* set, char* szName, char* szKey);    // FUN_00557ee0 __thiscall
};
struct CSoundResMgr { // this->m_c points here; +0x28 is the sound registry
    char m_pad00[0x28];
    CSoundResRegistry* m_28; // +0x28
};
struct CSoundSetSource {               // this->m_28 points here
    void* LookupSoundSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};

// Typed view of `this` for this loader: m_c is the sound resource manager, m_28
// the level's sound-set source. (EngineThisStub is the shared placeholder owner.)
struct CSoundOwner {
    char m_pad00[0xc];
    CSoundResMgr* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    CSoundSetSource* m_28; // +0x28
};

// @confidence: high
// @source: decomp-xref
// Byte-exact (100%). int (BOOL) return like its tile/image siblings: each guard
// is `return 0` reusing the just-zeroed eax and the success path is `mov eax,1`,
// reproducing retail's per-site `pop esi; ret 4` epilogues + the eax=1 tail (a
// void return tail-merges them and never emits eax=1). Registry reached through
// m_c->+0x28 (CSoundResRegistry) - distinct object/class from the tile loader's
// m_c->m_10, with direct (non-virtual) Has/Register/Install helpers.
RVA(0x000db6c0, 0x70)
int EngineThisStub::LoadLevelSounds(int force) {
    CSoundOwner* self = (CSoundOwner*)this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && self->m_c->m_28->Has("LEVEL")) {
        return 1;
    }

    self->m_c->m_28->Register("LEVEL", "_");

    void* sounds = self->m_28->LookupSoundSet("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    self->m_c->m_28->Install(sounds, "LEVEL", "_");
    return 1;
}

// ---------------------------------------------------------------------------
// EngineThisStub::LoadLevelImages - per-level IMAGEZ/LEVEL asset loader. Sibling
// of LoadActionTileSprites above; same registry idiom, different namespace keys
// ("LEVEL"/"IMAGEZ"/"_") and a SINGLE Register (no second "BACK" namespace).
// Reaches the registry through this->m_c (->m_10): if not forced (arg1==0) and
// "LEVEL" is already present, bail; otherwise register the "LEVEL" namespace
// (key "_"), reset the severus counter, look up the level's "IMAGEZ" set off
// this->m_28, and wire it in through the +0x48 virtual slot. Reuses the typed
// view-of-`this` (CActionTileOwner) from the sibling. Only offsets / code bytes
// are load-bearing; helpers are reloc-masked externals.
//
// @confidence: high
// @source: decomp-xref
// Byte-exact (100%). The real return type is int (BOOL): each guard is `return 0`
// reusing the just-zeroed eax (no normalizing mov) and the success path is
// `mov eax,1` - declaring the function `void` (as the sibling does) tail-merges
// the three identical bare epilogues and never emits `mov eax,1`, capping ~82%;
// the int return reproduces retail's inline per-site epilogues + the eax=1 tail.
// (Ghidra demangled the symbol as `...QAEXH@Z` = void from the relocs alone; the
// RVA-keyed delinker still pairs the int-return body at this address.)
RVA(0x000db7e0, 0x84)
int EngineThisStub::LoadLevelImages(int force) {
    CActionTileOwner* self = (CActionTileOwner*)this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && self->m_c->m_10->Has("LEVEL")) {
        return 1;
    }

    self->m_c->m_10->Register("LEVEL", "_");
    g_severusCounterA = 0;

    void* images = self->m_28->LookupTileSet("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_10->InstallTileSet(images, "LEVEL", "_");
    g_severusCounterA = 0;
    return 1;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dba30, 0x1ca)
void EngineThisStub::BuildMusicCategoryTable(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dbc80, 0x309)
void EngineThisStub::BuildWorldLevelPath(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dc060, 0x51b)
void EngineThisStub::LoadLevelEffectSprites() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dc6d0, 0x215)
void EngineThisStub::BuildGruntTypeNameTable(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dca70, 0x4a4)
void __stdcall EngineLabelBacklog::BuildAssetNamespacePrefixes(int, int, int, int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x000dd050, 0x24b)
void EngineThisStub::BuildGruntNamespaceList(int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x000dd340, 0x189)
void EngineThisStub::BuildWarlordNameTable(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dd540, 0x241)
void EngineThisStub::BuildSpriteImageKeyTable(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000ddaa0, 0x228)
void EngineThisStub::BuildAnizKeyTable(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000de420, 0x115)
void EngineThisStub::LoadLevelPreviewScreen() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dfd00, 0x6f5)
void EngineThisStub::LoadProjectileEffects() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000e2400, 0x39e)
void EngineThisStub::BuildToolToyColorKey(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000e2980, 0x2cd)
void EngineThisStub::LookupToolToyColorKey(int) {}

// ---------------------------------------------------------------------------
// EngineThisStub::LoadGruntzPalette - registers a level's "GRUNTZ_PALETTEZ_<name>"
// palette into the game's image/sprite registry. arg1 is the source registry
// object (its FUN_0053bff0 resolves a "PAL" resource by namespaced name); arg2
// is the level/name string spliced into the format. Reaches the destination
// registry through this->m_4->m_18: Lookup() (FUN_005b8008, +0x10 hash sub-table)
// probes whether the palette is already present, and virtual slot 9 (+0x24)
// installs the resolved palette. Only offsets / code bytes are load-bearing;
// helpers are reloc-masked externals.
//
// The destination registry at this->m_4->m_18 is polymorphic: a hash sub-table
// embedded at +0x10 backs Lookup() (out-param set non-null => already present),
// and Install (vtable slot 9) takes the resolved palette + two null args.
struct CPaletteHashTable { // embedded at CPaletteDestRegistry+0x10
    // FUN_005b8008 __thiscall; writes found value to *out.
    void Lookup(char* szName, void** out);
};
struct CPaletteDestRegistry {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual int Install(void* res, int a, int b); // slot 9 (+0x24)
    char m_pad04[0x10 - 0x4];
    CPaletteHashTable m_10; // +0x10  hash sub-table Lookup runs on
};
struct CPaletteDestRoot { // this->m_4 points here; +0x18 is the dest registry
    char m_pad00[0x18];
    CPaletteDestRegistry* m_18; // +0x18
};
// arg1's source registry: FUN_0053bff0 __thiscall resolves a packed-tag resource
// ('PAL' = 0x50414c) by namespaced name, returning the resource (0 if absent).
struct CPaletteSource {
    void* Resolve(char* szName, int tag); // FUN_0053bff0
};

// Typed view of `this`: m_4 is the destination registry root. (EngineThisStub is
// the shared placeholder owner.)
struct CPaletteOwner {
    char m_pad00[0x4];
    CPaletteDestRoot* m_4; // +0x04
};

// @confidence: high
// @source: decomp-xref
// int (BOOL) return: the `!arg1` and already-present guards return literal 0/1
// (reusing the zeroed eax / `mov eax,1`), and the success path normalizes the
// Install() return through neg/sbb/neg (`!!x`). A `void` return would tail-merge
// the bare epilogues and drop the eax=1 tail. The "PAL" key is a packed-tag
// immediate (0x50414c, not relocated). Lookup() runs on the +0x10 hash sub-table
// of m_4->m_18; Install is that object's vtable slot 9.
//
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// the ONLY residual is retail SINKING `mov [&found],0` past the `lea &found` (lea
// then store) while cl HOISTS it (store then lea) - identical instruction multiset,
// one 2-instr permutation, source-invariant under /O2. Logic + all bytes otherwise
// exact (frame 0x40, epilogues, !!x normalize all match).
RVA(0x000e2d10, 0xa1)
int EngineThisStub::LoadGruntzPalette(int src, int name) {
    CPaletteOwner* self = (CPaletteOwner*)this;
    if (!src) {
        return 0;
    }

    void* found = 0;
    self->m_4->m_18->m_10.Lookup((char*)name, &found);
    if (found) {
        return 1;
    }

    char buf[0x40];
    sprintf(buf, "GRUNTZ_PALETTEZ_%s", (char*)name);
    void* pal = ((CPaletteSource*)src)->Resolve(buf, 0x50414c);
    if (!pal) {
        return 0;
    }
    return self->m_4->m_18->Install(pal, 0, 0) != 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000e3f40, 0x3d6)
void EngineLabelBacklog::DrawSaveGameMenu() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000e44e0, 0x2b2)
void EngineLabelBacklog::BuildLevelTitleString() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000e4b60, 0x158)
void EngineThisStub::SaveGameFile(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000e8a70, 0x18c)
void EngineThisStub::
    BuildResourceTabStatusBar(int, int, int, int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000e9600, 0x18c)
void EngineThisStub::
    BuildStatzTabStatusBar(int, int, int, int, int, int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000e9850, 0x111)
void EngineThisStub::BuildStatzTabSmallSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ea1f0, 0x1fa)
void EngineThisStub::
    BuildMultiplayerTabStatusBar(int, int, int, int, int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000f8970, 0x3b4)
void SFManager::SelectBestDevice() {}

// @confidence: high
// @source: string-xref
// @stub
RVA(0x000f8f30, 0x160)
void EngineLabelBacklog::BuildSoundFontPath() {}

// @confidence: low
// @source: import:OpenFile
// @stub
RVA(0x000f90f0, 0x45)
void EngineLabelBacklog::Stub_0f90f0() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000f9ea0, 0x21d)
void EngineThisStub::LoadGameAssetNamespaces(int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000fdc00, 0x5c2)
void EngineThisStub::LoadBattlezItemConfig(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe6b0, 0x145)
void EngineThisStub::LoadMainStatusBarSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe910, 0xb8e)
void EngineThisStub::UpdateStatusBarTabHighlight(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ffb20, 0x13a)
void EngineThisStub::LoadDestructButtonSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102180, 0x5f)
void EngineThisStub::BuildGameTabResumeButton(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102200, 0x37)
void EngineThisStub::BuildGameTabPauseButton() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001055b0, 0x109)
void EngineThisStub::LoadGooCookingSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00105990, 0x398)
void EngineThisStub::UpdateRezConveyorStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00105e40, 0x62c)
void EngineThisStub::LoadRezMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00106660, 0x68)
void EngineThisStub::UpdateRezMachineSnoozeStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00106bb0, 0x7bc)
void EngineThisStub::LoadChipMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107590, 0xc4)
void EngineThisStub::UpdateFallingItemStatusBar(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107a10, 0x62)
void EngineThisStub::UpdateRezMachineWakeStatusBar() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107ae0, 0x1aa)
void EngineThisStub::LoadMultiplayerBattlezConfig(int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x0010bc30, 0x78)
void EngineThisStub::UpdateDestructButtonStatusBar2(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00110860, 0x25f)
void __stdcall EngineLabelBacklog::LoadBridgeMoveSprites(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00110c10, 0xe3f)
void EngineLabelBacklog::LoadPyramidBridgeSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001122a0, 0x241)
void EngineThisStub::BuildRockBreakInGameText() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00112a50, 0xdd)
void __stdcall EngineLabelBacklog::
    BuildStatzTabSmall_vfunc1(int, int, int, int, int, int, int, int, int) {}

// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x00114ff0, 0x1b3)
void EngineLabelBacklog::SaveScreenshot() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x001183b0, 0x211)
void EngineLabelBacklog::FormatGameInfoString() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0011afb0, 0x321)
void EngineThisStub::LoadGruntSpawnConfig(int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0011c210, 0x29d)
void __stdcall EngineLabelBacklog::BuildVoiceSoundList(int) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x0011f900, 0x101)
void EngineLabelBacklog::Stub_11f900() {}

// @confidence: low
// @source: import:WriteFile
// @stub
RVA(0x0012abf0, 0x1d6)
void EngineLabelBacklog::Stub_12abf0() {}

// @confidence: low
// @source: import:WriteFile
// @stub
RVA(0x0012d230, 0x209)
void EngineLabelBacklog::Stub_12d230() {}

// @confidence: low
// @source: import:CreateFileA
// @stub
RVA(0x0012d460, 0x351)
void EngineLabelBacklog::Stub_12d460() {}

// @confidence: low
// @source: import:SetFilePointer
// @stub
RVA(0x0012d880, 0x80)
void EngineLabelBacklog::Stub_12d880() {}

// @confidence: low
// @source: import:ReadFile
// @stub
RVA(0x001315d0, 0x225)
void EngineLabelBacklog::Stub_1315d0() {}

// @confidence: low
// @source: second '.PID' xref; image-format dispatch consumer (entry not pinned)
// @stub
RVA(0x00148940, 0x102)
void __stdcall EngineLabelBacklog::Stub_148940(int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00170750, 0x9d8)
void ButeMgr::ParseAttributeFile() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x00185000, 0x1a6)
void EngineThisStub::DebugPrintf() {}

// @confidence: high
// @source: call-xref
// @stub
RVA(0x00188440, 0x74)
void EngineLabelBacklog::_tr_init() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x001884c0, 0x1ee)
void EngineLabelBacklog::_ct_init() {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x0018c780, 0x33f)
void EngineLabelBacklog::Stub_18c780() {}

// @confidence: med
// @source: import:RegQueryValueExA
// @stub
RVA(0x001bf702, 0xac)
void __stdcall EngineLabelBacklog::Stub_1bf702(int, int) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x001bf8f8, 0xd9)
void __stdcall EngineLabelBacklog::Stub_1bf8f8(int, int) {}

// @confidence: low
// @source: import:GetFileSize
// @stub
RVA(0x001c152f, 0xda)
void EngineThisStub::Stub_1c152f(int) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x001c1609, 0xb2)
void __stdcall EngineLabelBacklog::Stub_1c1609(int, int) {}

// @confidence: low
// @source: import:CreateFileA
// @stub
RVA(0x001c176a, 0x12d)
void __stdcall EngineLabelBacklog::Stub_1c176a(int, int) {}

// @confidence: med
// @source: import:RegCloseKey,RegOpenKeyExA,RegQueryValueExA
// @stub
RVA(0x001c7cb3, 0x177)
void EngineLabelBacklog::Stub_1c7cb3() {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
RVA(0x001ccae7, 0x75)
void EngineThisStub::Stub_1ccae7(int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
RVA(0x001ccb5c, 0xa0)
void __stdcall EngineLabelBacklog::Stub_1ccb5c(int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
RVA(0x001ccbfc, 0xa1)
void EngineThisStub::Stub_1ccbfc(int, int, int, int) {}

// @confidence: med
// @source: import:RegOpenKeyExA
// @stub
RVA(0x001d4ee3, 0x94)
void EngineLabelBacklog::Stub_1d4ee3() {}

// @confidence: med
// @source: import:RegCloseKey,RegCreateKeyExA
// @stub
RVA(0x001d4f77, 0x46)
void __stdcall EngineLabelBacklog::Stub_1d4f77(int) {}

// @confidence: med
// @source: import:RegCloseKey,RegQueryValueExA
// @stub
RVA(0x001d4fbd, 0x6c)
void __stdcall EngineLabelBacklog::Stub_1d4fbd(int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey
// @stub
RVA(0x001d5029, 0x112)
void __stdcall EngineLabelBacklog::Stub_1d5029(int, int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey
// @stub
RVA(0x001d513b, 0x11b)
void __stdcall EngineLabelBacklog::Stub_1d513b(int, int, int, int) {}
