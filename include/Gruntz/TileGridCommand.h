// TileGridCommand.h - Gruntz CTileGridCommand (C:\Proj\Gruntz).
//
// A tile-grid command object: a type tag at +0x04, grid coords at +0x08/+0x0c,
// a flag at +0x14, a back-pointer to its owning 3-list container at +0x20, and a
// captured game-clock snapshot at +0x24.  It serializes its fields through a
// CSerialStream and edits the game registry's tile grid (g_gameReg) on apply.
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (type tag @ +0x04, coords
// @ +0x08/+0x0c, the +0x20 container back-pointer - not the +0x04 child-list
// switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_TILEGRIDCOMMAND_H
#define SRC_GRUNTZ_TILEGRIDCOMMAND_H

#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Wwd/WwdFile.h> // CPlaneRender - the canonical plane (the active layer)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created InGameText sprite)
#include <rva.h>                  // SIZE_UNKNOWN class-metadata macros used below

#include <Gruntz/TileTriggerContainer.h>

// The running game clock (DAT_00645588); reloc-masked DIR32 datum.
extern "C" u32 g_645588;

// The active tile layer (flat cell array + per-row base-offset table, cell (x,y) =
// m_tileGrid[m_colOffsets[y] + x]) is the shared CPlaneRender.

// The tile map: m_5c is the active layer.
struct TgcMap {
    char _pad00[0x5c];
    CPlaneRender* m_5c; // +0x5c  active layer
};
SIZE_UNKNOWN(TgcMap);

// A report record posted by the in-game text manager; +0x124 latches a serial.
// The floating "InGameText" record is a fresh CSpriteFactory::CreateSprite result
// (the canonical 0x1597b0 factory entry; the former "Report" role-name was a
// mislabel - it CREATES the "InGameText" sprite, hint 95000=0x17318): the caller
// stamps the string id into the created CGameObject's m_124 selector key.
// The active game manager: m_08 is the sprite factory; m_24 the tile map.
struct TgcGameMgr {
    char _pad00[0x08];
    CSpriteFactory* m_08; // +0x08  the sprite factory (CreateSprite @0x1597b0)
    char _pad0c[0x24 - 0x0c];
    TgcMap* m_24; // +0x24  the tile map
};
SIZE_UNKNOWN(TgcGameMgr);

// A redraw-region helper (g_gameReg->m_tileGrid): MarkCell pushes a dirty cell so the
// renderer repaints it.  __thiscall engine callee, reloc-masked.
struct TgcRedraw {};
SIZE_UNKNOWN(TgcRedraw);

// A pixel-region dirty helper (g_gameReg->m_68): MarkRect flags a screen rect for
// repaint.  __thiscall engine callee, reloc-masked.
struct TgcRegion {
    // MarkRect @0x152d IS EngineLabelBacklog::LoadPowerupIconSprites; cast at the call.
};
SIZE_UNKNOWN(TgcRegion);

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c); +0x30 is the active game
// manager, +0x68 the rect-dirty helper, +0x70 the redraw helper.
SIZE_UNKNOWN(CGameRegistry);

// The serialization stream is the shared WAP32 CSerialArchive (Read @ vtable +0x2c /
// Write @ +0x30 - the store/transfer slot this cluster drives), now the one modeled
// class in <Gruntz/SerialArchive.h> - the former local `TgcStream` view is folded away.

// CTileGridCommand IS GONE (folded 2026-07-13). It was an INVENTED name: the string
// "CTileGridCommand" does not occur anywhere in GRUNTZ.EXE, while CTileTriggerLogic /
// CGiantRockLogic / CTileTriggerSwitchLogic all do (RTTI). It was a second, divergent
// reconstruction of CTileTriggerLogic - field-for-field identical (0x9c, type tag @ +0x04,
// coords @ +0x08/+0x0c, container back-pointer @ +0x20, 24-dword block @ +0x3c) - and the
// binary proves the two names denote ONE object at runtime, three ways:
//   1. CTileTriggerLogic::ValidateByType (called by CTileTriggerFactory::Build on a freshly
//      `new`ed 0x9c CTileTriggerLogic) hands its own `this` in ecx straight to what was
//      "CTileGridCommand::Serialize/Deserialize".
//   2. The container's list walkers `new` these elements through AddLogic, whose retail
//      signature RETURNS CTileTriggerLogic* - and then cast them to CTileGridCommand to call
//      ApplyMove/Classify. The cast was the symptom.
//   3. Decisive: the inlined `delete` in those same walkers zeroes +0x1c, which is
//      CTileTriggerLogic's dtor (`m_1c = 0`). The 0x8c CTileTriggerSwitchLogic's dtor zeroes
//      +0x20 instead - that is how RemoveByKeys' element type was told apart from this one.
// All five methods (Tick/RecordMove/Classify/BumpCell/ApplyMove) now live on
// CTileTriggerLogic in <Gruntz/TileTriggerLogic.h>, with the richer field names migrated.
// The remaining Tgc* structs here are the real engine helper shapes the bodies reach.
#include <Gruntz/TileTriggerLogic.h>

#endif // SRC_GRUNTZ_TILEGRIDCOMMAND_H
