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
class CHelpState {
public:
    void LoadAssets(int, int, int);
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
    void LoadActionTileSprites(int);
    void LoadLevelSounds(int);
    void LoadLevelImages(int);
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
    void LoadGruntzPalette(int, int);
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
RVA(0x00a3b0, 0x6e2)
void EngineLabelBacklog::CreateGameObjectByName() {}

// @confidence: low
// @source: string-xref
// @stub
RVA(0x018830, 0x380)
void __stdcall EngineLabelBacklog::LoadBootyCheatState(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x018d30, 0xcd)
void BootyState::vfunc_9(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x018f00, 0x4fb)
void EngineLabelBacklog::ShowSecretBonusMessage() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x019540, 0x12a)
void EngineThisStub::BuildWarpStoneGlitterAnimation() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x019920, 0x1c2)
void EngineLabelBacklog::BuildGruntSprintAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x01a040, 0x55e)
void EngineThisStub::LoadGruntEffectSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x01b450, 0x1ac)
void EngineThisStub::BuildBootyWalkingGruntz() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x01b690, 0x7bf)
void EngineLabelBacklog::UpdateBootyWalkingGruntz() {}

// BuildBootyPerfectAnimation @0x01c070 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: string-xref
// @stub
RVA(0x01c210, 0x4b5)
void EngineThisStub::CheckWarpLetterBonus() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x01c8a0, 0xec)
void EngineThisStub::BootyState_OnActivate2_vfunc8() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x01c9d0, 0x351)
void EngineLabelBacklog::ShowLevelCompleteMessage() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x01ce60, 0x450)
void EngineLabelBacklog::BuildBootyGruntIdleAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x01e720, 0x2fe)
void __stdcall EngineLabelBacklog::BuildPowerupIconKeys(int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x01ed30, 0x549)
void EngineLabelBacklog::DrawBattleStats() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x01f6f0, 0x10b)
void BootyState::OnActivate_vfunc8() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x01f9b0, 0x2d2)
void EngineLabelBacklog::StartUpPrompt() {}

// @confidence: low
// @source: import:OpenFile
// @stub
RVA(0x01fd70, 0x45)
void EngineLabelBacklog::Stub_01fd70() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x020f40, 0x188)
void EngineThisStub::LoadChatBoxSprite(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x022e60, 0x1be)
void EngineLabelBacklog::LoadCheatConfig() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x038d20, 0x176)
void EngineThisStub::LoadCreditzStateAssets(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x039570, 0x122)
void EngineThisStub::InitAttractTitle() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x039dc0, 0x10b)
void EngineThisStub::LoadCreditzAssets2() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x03b7c0, 0x12c)
void EngineLabelBacklog::LoadCustomWorldInfo() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x03c0e0, 0xfb)
void EngineThisStub::BuildWorldLevelKey(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x03f5f0, 0x526)
void EngineLabelBacklog::HandleFortConquered() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x044f80, 0x127)
void EngineThisStub::BuildFortSplashParticles() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x045270, 0x2a8)
void EngineThisStub::NotifyFortUnderAttack() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x04dd50, 0x22c0)
void EngineThisStub::LoadGruntTypeTable(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x050ce0, 0x399)
void __stdcall EngineLabelBacklog::LoadVehicleGruntSprites(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x057100, 0x577)
void EngineThisStub::LoadGruntAbilityTuning(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x057890, 0x19c)
void EngineThisStub::BuildGruntLoseItemAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0597a0, 0x1345)
void EngineThisStub::LoadGruntCombatAnimations(int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x05d210, 0x1443)
void EngineThisStub::LoadGruntTuningConstants(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x060150, 0xd90)
void EngineThisStub::LoadGruntDeathAnimations(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0612a0, 0x23c)
void EngineThisStub::LoadGruntDecayConfig() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x061570, 0x11d)
void EngineThisStub::LoadGruntDecayConfig2() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x061cb0, 0x34a)
void Projectile::vfunc_9() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x063db0, 0x32f)
void EngineThisStub::LoadVehicleGruntAnimations() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0641b0, 0x2c1)
void EngineThisStub::BuildGruntExitAnimation() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x065630, 0x34b)
void EngineThisStub::LoadBombGruntRunConfig(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x065a60, 0x159)
void EngineThisStub::LoadWandGruntItemConfig() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x065e80, 0x12b8)
void EngineThisStub::LoadPickupSprites(int, int, int, int, int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x068520, 0x2a2)
void EngineThisStub::LoadBombGruntRunConfig2() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x068880, 0x67c)
void EngineThisStub::LoadWingzGruntSprites(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x069d60, 0x1e1)
void EngineThisStub::LoadFreezeSpellAssets() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x06a060, 0x43d)
void EngineThisStub::LoadGruntMovingDeathConfig() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x06c130, 0xd62)
void __stdcall EngineLabelBacklog::WireTileSwitchLogic(int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x06eb80, 0x5ef)
void EngineThisStub::LoadTeleporterGooConfig(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x075e90, 0x1329)
void __stdcall EngineLabelBacklog::LoadTerrainTileSprites(int, int, int, int, int, int) {}

// LoadCameraSprite @0x078960 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x07a3f0, 0xd7)
void __stdcall EngineLabelBacklog::LoadToyBoxIcon(int, int, int, int, int) {}

// LoadExplosionSprites @0x07b330 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x07b440, 0x3f0)
void __stdcall EngineLabelBacklog::BuildRockBreakParticles(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x07b930, 0x3b7)
void EngineThisStub::LoadGruntCombatTuning(int, int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x07be60, 0x21e)
void __stdcall EngineLabelBacklog::LoadGruntResurrectTuning(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x07c3d0, 0x1ae)
void EngineThisStub::LoadFinishLevelSprite(int) {}

// LoadPowerupIconSprites @0x07c620 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x090550, 0x1e6)
void __stdcall EngineLabelBacklog::LaunchPortalExe(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x090d10, 0x18e)
void EngineThisStub::LoadMonologoSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x092420, 0xa4)
void EngineThisStub::LoadSaveMessageSprite() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x093d40, 0x473)
void __stdcall EngineLabelBacklog::BuildLevelRezPath(int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x095090, 0x6e)
void CHelpState::LoadAssets(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0997c0, 0x1e7)
void EngineLabelBacklog::LoadHelpBookSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x09a510, 0x275)
void __stdcall EngineLabelBacklog::LoadObjectImageResources(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x09a910, 0x261)
void __stdcall EngineLabelBacklog::LoadObjectSoundResources(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x09ac20, 0x261)
void __stdcall EngineLabelBacklog::LoadObjectAnimResources(int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x09fe50, 0x343)
void __stdcall EngineLabelBacklog::LoadMenuStateAssets(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0a09a0, 0x6a)
void EngineThisStub::LoadStateImages_vfunc8() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0a0d80, 0xd7)
void EngineThisStub::BuildVersionString(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0a11d0, 0x180d)
void EngineLabelBacklog::LoadAreaLevelTable() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0b0140, 0xa7a)
void EngineLabelBacklog::LoadRollingBallHazardSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0b4640, 0x104)
void CloudHazard::vfunc_20(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0ba620, 0x14a)
void EngineThisStub::LoadMenuSelectSprite(int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x0bf1d0, 0x249)
void EngineLabelBacklog::BuildGruntzCrcInfo() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0c16b0, 0x3d)
void EngineLabelBacklog::BuildNamedGruntTable() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0c7090, 0x21b)
void EngineThisStub::LoadDroppedObjectEffects() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0ca200, 0xe34)
void __stdcall EngineLabelBacklog::LoadLevelByMode(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0cb800, 0x191)
void GameLevelState::OnActivate_vfunc8() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0cbcc0, 0x16da)
void StatusBarItem::vfunc_12(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0ce660, 0x362)
void StatusBarItem::vfunc_16(int, int, int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x0cf770, 0x35e)
void EngineLabelBacklog::DrawDebugStats() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0cffe0, 0x3c)
void EngineThisStub::LoadImageBanks_vfunc29() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0d0120, 0x5d8)
void EngineThisStub::LoadCursorSprites(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0d12b0, 0x2d5)
void EngineThisStub::LoadScrollSpeedOptions() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0d1710, 0x122)
void EngineThisStub::LoadSBITextEdges(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0d2dd0, 0x1de4)
void EngineLabelBacklog::ValidateLevelTiles() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0d65d0, 0x7a4)
void EngineThisStub::LoadWarlordSprites(int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0db600, 0x8f)
void EngineThisStub::LoadActionTileSprites(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0db6c0, 0x70)
void EngineThisStub::LoadLevelSounds(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0db7e0, 0x84)
void EngineThisStub::LoadLevelImages(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dba30, 0x1ca)
void EngineThisStub::BuildMusicCategoryTable(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dbc80, 0x309)
void EngineThisStub::BuildWorldLevelPath(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dc060, 0x51b)
void EngineThisStub::LoadLevelEffectSprites() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dc6d0, 0x215)
void EngineThisStub::BuildGruntTypeNameTable(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dca70, 0x4a4)
void __stdcall EngineLabelBacklog::BuildAssetNamespacePrefixes(int, int, int, int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x0dd050, 0x24b)
void EngineThisStub::BuildGruntNamespaceList(int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x0dd340, 0x189)
void EngineThisStub::BuildWarlordNameTable(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dd540, 0x241)
void EngineThisStub::BuildSpriteImageKeyTable(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0ddaa0, 0x228)
void EngineThisStub::BuildAnizKeyTable(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0de420, 0x115)
void EngineThisStub::LoadLevelPreviewScreen() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0dfd00, 0x6f5)
void EngineThisStub::LoadProjectileEffects() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0e2400, 0x39e)
void EngineThisStub::BuildToolToyColorKey(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0e2980, 0x2cd)
void EngineThisStub::LookupToolToyColorKey(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0e2d10, 0xa1)
void EngineThisStub::LoadGruntzPalette(int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0e3f40, 0x3d6)
void EngineLabelBacklog::DrawSaveGameMenu() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0e44e0, 0x2b2)
void EngineLabelBacklog::BuildLevelTitleString() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0e4b60, 0x158)
void EngineThisStub::SaveGameFile(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0e8a70, 0x18c)
void EngineThisStub::
    BuildResourceTabStatusBar(int, int, int, int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0e9600, 0x18c)
void EngineThisStub::
    BuildStatzTabStatusBar(int, int, int, int, int, int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0e9850, 0x111)
void EngineThisStub::BuildStatzTabSmallSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0ea1f0, 0x1fa)
void EngineThisStub::
    BuildMultiplayerTabStatusBar(int, int, int, int, int, int, int, int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0f8970, 0x3b4)
void SFManager::SelectBestDevice() {}

// @confidence: high
// @source: string-xref
// @stub
RVA(0x0f8f30, 0x160)
void EngineLabelBacklog::BuildSoundFontPath() {}

// @confidence: low
// @source: import:OpenFile
// @stub
RVA(0x0f90f0, 0x45)
void EngineLabelBacklog::Stub_0f90f0() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0f9ea0, 0x21d)
void EngineThisStub::LoadGameAssetNamespaces(int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0fdc00, 0x5c2)
void EngineThisStub::LoadBattlezItemConfig(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0fe6b0, 0x145)
void EngineThisStub::LoadMainStatusBarSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0fe910, 0xb8e)
void EngineThisStub::UpdateStatusBarTabHighlight(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0ffb20, 0x13a)
void EngineThisStub::LoadDestructButtonSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x102180, 0x5f)
void EngineThisStub::BuildGameTabResumeButton(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x102200, 0x37)
void EngineThisStub::BuildGameTabPauseButton() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x1055b0, 0x109)
void EngineThisStub::LoadGooCookingSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x105990, 0x398)
void EngineThisStub::UpdateRezConveyorStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x105e40, 0x62c)
void EngineThisStub::LoadRezMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x106660, 0x68)
void EngineThisStub::UpdateRezMachineSnoozeStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x106bb0, 0x7bc)
void EngineThisStub::LoadChipMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x107590, 0xc4)
void EngineThisStub::UpdateFallingItemStatusBar(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x107a10, 0x62)
void EngineThisStub::UpdateRezMachineWakeStatusBar() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x107ae0, 0x1aa)
void EngineThisStub::LoadMultiplayerBattlezConfig(int) {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x10bc30, 0x78)
void EngineThisStub::UpdateDestructButtonStatusBar2(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x110860, 0x25f)
void __stdcall EngineLabelBacklog::LoadBridgeMoveSprites(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x110c10, 0xe3f)
void EngineLabelBacklog::LoadPyramidBridgeSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x1122a0, 0x241)
void EngineThisStub::BuildRockBreakInGameText() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x112a50, 0xdd)
void __stdcall EngineLabelBacklog::
    BuildStatzTabSmall_vfunc1(int, int, int, int, int, int, int, int, int) {}

// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x114ff0, 0x1b3)
void EngineLabelBacklog::SaveScreenshot() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x1183b0, 0x211)
void EngineLabelBacklog::FormatGameInfoString() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x11afb0, 0x321)
void EngineThisStub::LoadGruntSpawnConfig(int, int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x11c210, 0x29d)
void __stdcall EngineLabelBacklog::BuildVoiceSoundList(int) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x11f900, 0x101)
void EngineLabelBacklog::Stub_11f900() {}

// @confidence: low
// @source: import:WriteFile
// @stub
RVA(0x12abf0, 0x1d6)
void EngineLabelBacklog::Stub_12abf0() {}

// @confidence: low
// @source: import:WriteFile
// @stub
RVA(0x12d230, 0x209)
void EngineLabelBacklog::Stub_12d230() {}

// @confidence: low
// @source: import:CreateFileA
// @stub
RVA(0x12d460, 0x351)
void EngineLabelBacklog::Stub_12d460() {}

// @confidence: low
// @source: import:SetFilePointer
// @stub
RVA(0x12d880, 0x80)
void EngineLabelBacklog::Stub_12d880() {}

// @confidence: low
// @source: import:ReadFile
// @stub
RVA(0x1315d0, 0x225)
void EngineLabelBacklog::Stub_1315d0() {}

// @confidence: low
// @source: second '.PID' xref; image-format dispatch consumer (entry not pinned)
// @stub
RVA(0x148940, 0x102)
void __stdcall EngineLabelBacklog::Stub_148940(int, int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x170750, 0x9d8)
void ButeMgr::ParseAttributeFile() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x185000, 0x1a6)
void EngineThisStub::DebugPrintf() {}

// @confidence: high
// @source: call-xref
// @stub
RVA(0x188440, 0x74)
void EngineLabelBacklog::_tr_init() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1884c0, 0x1ee)
void EngineLabelBacklog::_ct_init() {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x18c780, 0x33f)
void EngineLabelBacklog::Stub_18c780() {}

// @confidence: med
// @source: import:RegQueryValueExA
// @stub
RVA(0x1bf702, 0xac)
void __stdcall EngineLabelBacklog::Stub_1bf702(int, int) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x1bf8f8, 0xd9)
void __stdcall EngineLabelBacklog::Stub_1bf8f8(int, int) {}

// @confidence: low
// @source: import:GetFileSize
// @stub
RVA(0x1c152f, 0xda)
void EngineThisStub::Stub_1c152f(int) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x1c1609, 0xb2)
void __stdcall EngineLabelBacklog::Stub_1c1609(int, int) {}

// @confidence: low
// @source: import:CreateFileA
// @stub
RVA(0x1c176a, 0x12d)
void __stdcall EngineLabelBacklog::Stub_1c176a(int, int) {}

// @confidence: med
// @source: import:RegCloseKey,RegOpenKeyExA,RegQueryValueExA
// @stub
RVA(0x1c7cb3, 0x177)
void EngineLabelBacklog::Stub_1c7cb3() {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
RVA(0x1ccae7, 0x75)
void EngineThisStub::Stub_1ccae7(int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
RVA(0x1ccb5c, 0xa0)
void __stdcall EngineLabelBacklog::Stub_1ccb5c(int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
RVA(0x1ccbfc, 0xa1)
void EngineThisStub::Stub_1ccbfc(int, int, int, int) {}

// @confidence: med
// @source: import:RegOpenKeyExA
// @stub
RVA(0x1d4ee3, 0x94)
void EngineLabelBacklog::Stub_1d4ee3() {}

// @confidence: med
// @source: import:RegCloseKey,RegCreateKeyExA
// @stub
RVA(0x1d4f77, 0x46)
void __stdcall EngineLabelBacklog::Stub_1d4f77(int) {}

// @confidence: med
// @source: import:RegCloseKey,RegQueryValueExA
// @stub
RVA(0x1d4fbd, 0x6c)
void __stdcall EngineLabelBacklog::Stub_1d4fbd(int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey
// @stub
RVA(0x1d5029, 0x112)
void __stdcall EngineLabelBacklog::Stub_1d5029(int, int, int, int) {}

// @confidence: med
// @source: import:RegCloseKey
// @stub
RVA(0x1d513b, 0x11b)
void __stdcall EngineLabelBacklog::Stub_1d513b(int, int, int, int) {}
