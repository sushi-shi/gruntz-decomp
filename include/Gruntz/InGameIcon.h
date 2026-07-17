// InGameIcon.h - the in-game HUD/cursor icon, a CUserLogic-derived game object
// (vftables 0x5e705c / 0x5e70b4, the CUserLogic / CUserBase pair). The class
// owns the inherited CUserLogic layout (the CGameObject* at +0x10, the +0x18
// EngStr link, etc.; see <Gruntz/UserLogic.h>) plus its own icon-state fields
// from +0x44 onward. Modeled as a CUserLogic leaf so the empty dtor folds to the
// bare CUserLogic teardown (store 0x5e705c, ~EngStr on +0x18, store 0x5e70b4)
// under a /GX frame - the same shape every UserLogic leaf dtor matches.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes
// are load-bearing (campaign doctrine). Layout recovered from the method field
// stores (Setup986b0 / Serialize98c90 write +0x30..+0x64, +0x78; Set99b10 -> +0x54).
#ifndef GRUNTZ_GRUNTZ_CINGAMEICON_H
#define GRUNTZ_GRUNTZ_CINGAMEICON_H

#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h> // CObject/CArchive base + <windows.h>

#include <Gruntz/UserLogic.h>        // CUserLogic : CUserBase, EngStr, CGameObject
extern "C" CGameRegistry* g_gameReg; // *0x24556c canonical singleton

// ---------------------------------------------------------------------------
// CGameRegistry - the global game-manager singleton (the object at *0x64556c). Only
// the offsets the icon paths touch are modeled. Named CGameRegistry (not the richer
// CGameRegistry) so the ?g_gameReg@@3PAUCGameReg@@A data ref matches retail.
//
//   +0x30  m_30  : a CMap-chain holder (Set99b10 walks m_30->m_28 to a Lookup)
//   +0x68  m_68  : the icon/sprite factory (GetByIndex on the per-player block)
//   +0x70  m_70  : the tile occupancy grid (Setup986b0 clears a cell flag)
//   +0x74  m_74  : the per-player icon-table base for the input-driven handler
//   +0x120 .. +0x148, +0x158 : the input/view-bounds + per-player icon table.
// ---------------------------------------------------------------------------
// The CMap value lookup helper (a CMapPtrToPtr-style table). Lookup(key, &out)
// hashes the key and writes the found value through out; the call (0x1b8438,
// __thiscall) reloc-masks. The table is embedded at +0x10 of its holder, so
// `add ecx,0x10; call Lookup` falls out.
// The +0x10 table is an MFC CMapStringToOb; Lookup @0x1b8438 is CMapStringToOb::Lookup
// (reached via a CMapStringToOb cast at the call in InGameIcon.cpp).
// (ex-`CMapStringToOb`: empty phantom; the +0x10 table IS the MFC CMapStringToOb - verified,
//  CInGameIcon::SetField54 @0x99b10 calls 0x1b8438. The +0x48 map on the sprite factory
//  is a DIFFERENT class, CMapPtrToPtr @0x1b8760 - do not conflate them.)
struct CIconMapHolder {
    char m_pad00[0x10];
    CMapStringToPtr m_10map; // +0x10  the lookup table (Lookup 0x1b8438 = CMapStringToPtr)
};
struct CGameRegMapHolder {
    char m_pad00[0x28];
    CIconMapHolder* m_28; // +0x28  the map holder (Lookup table at +0x10)
};

// The tile occupancy grid reached as g_gameReg->m_tileGrid is CTileGrid
// (<Gruntz/TileGrid.h>, via CGameRegistry.h): m_8 row table, m_c/m_10 bounds;
// each cell is 0x1c bytes (7 dwords), indexed by tile (x,y).

// The icon/sprite factory (g_gameReg->m_spriteFactory, +0x74) is CSpriteRefTable
// (<Gruntz/SpriteRefTable.h>): the former CIconFactory::GetByIndex(idx, z) is
// CSpriteRefTable::GetSel(i, bAlt) - the icon paths call GetSel directly, cast-free
// (thunk 0x4165 -> 0xe23c0 == CSpriteRefTable::GetSel).

#include <Gruntz/CurPlayer.h> // g_curPlayer (the current local player index)

#include <Gruntz/SoundState.h> // g_sndCueTag (the cue-item id) + g_sndEnabled


extern "C" u32 g_frameTime; // DAT_00645588  (the running game clock stamped into +0x58)

#include <Gruntz/SerialCounter.h> // g_serialCounter (the serialize sequence counter)

// ---------------------------------------------------------------------------
// CButeTree::Find on the global g_buteTree - the bute store the Setup path
// queries (mov ecx,&g_buteTree; call Find). Declared in <Bute/ButeMgr.h>,
// pulled via UserLogic.h indirectly; g_buteTree owned by another TU.
// ---------------------------------------------------------------------------

// The input/cmd sink the place handler flushes (thunk 0x25fe -> FUN_0041f940,
// __stdcall posts a cmd packet). External/no-body (reloc-masked).
void __stdcall Eng_PostCmd(i32 ctx, i32 a, i32 b, i32 c); // 0x41f940

// The icon-table per-player record reached as g_gameReg->m_68[(idx)*4 + 0x1c].
// __thiscall LoadPickupSprites / LoadGruntTypeTable bind the icon's sprite set;
// +0x1fc is a "configured" gate, +0x38c a posted-id slot the place handler stamps.
// Both engine methods are external (reloc-masked, no body).
// CIconRecord's LoadPickupSprites/LoadGruntTypeTable ARE CGrunt's; cast at the calls.
struct CIconRecord {
    char m_pad00[0x1fc];
    i32 m_1fc; // +0x1fc  configured flag
    char m_pad200[0x38c - 0x200];
    i32 m_38c; // +0x38c  posted command id
};

// ---------------------------------------------------------------------------
// CInGameIcon : CUserLogic. Its own state begins at +0x44 (CUserLogic ends at
// +0x40). The dtor (0x11d00) adds no destructible members, so it folds the bare
// CUserLogic teardown.
// ---------------------------------------------------------------------------
class CInGameIcon : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011cb0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMEICON;
    } // slot 2
public:
    CInGameIcon(CGameObject* obj);   // 0x095b10  (the HUD-icon builder ctor)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    // Configure the icon's sprite/animation for a category ("GAME_TREASURE" /
    // "GAME_POWERUP" / "GAME_CURSE"; 0 for the initial reset). Reached through an
    // ILT thunk (0x3440), reloc-masked.
    void SetupSprite(const char* cat); // -> 0x3440
    // Post-build validity probe (ILT 0x1fb4); returns 0 to hide the icon.
    i32 Check(); // -> 0x1fb4

    i32 HandleInput(); // 0x097680
    virtual void FireActivation(i32 id)
        OVERRIDE; // 0x097880 (dispatch g_iconActionTable[id] on this)
    // (RunState @0x097de0 is GONE from here: the ILT bytes prove it is CToyPeek's
    // vtable slot 4 - see <Gruntz/ToyPeek.h>. It never was a CInGameIcon method.)
    i32 RefreshCell();                  // 0x098340
    i32 PeekCycle();                    // 0x0984b0 (peek-timer random-sprite refresh)
    i32 PlaceAt(i32 idx, i32 gridBase); // 0x0986b0
    i32 Reposition();                   // 0x098a90 (drift re-place refresh)
    void SetField54(i32 v);             // 0x099b10

    // --- CInGameIcon own fields (+0x44/+0x68..+0x74 roles still unproven) ---
    i32 m_cmapId;              // +0x54  registry-CMap lookup result (SetField54; place gate)
    i32 m_driftPos;            // +0x58  drift-tracked position lo (i64 {m_driftPos:m_driftPosHi})
    i32 m_driftPosHi;          // +0x5c  drift-tracked position hi
    i32 m_driftThresh;         // +0x60  drift threshold lo (i64 {m_driftThresh:m_driftThreshHi})
    i32 m_driftThreshHi;       // +0x64  drift threshold hi
    i32 m_68;                  // +0x68  (serialized state; role unproven)
    i32 m_6c;                  // +0x6c  (role unproven)
    i32 m_70;                  // +0x70  (role unproven)
    i32 m_74;                  // +0x74  (role unproven)
    CGameObject* m_glitterSprite; // +0x78  glitter overlay FX sprite (powerup/curse)
};
VTBL(CInGameIcon, 0x1e7d04);

// The handler PMF stored in each g_icon*Table slot (a 4-byte code pointer on this
// complete single-inheritance class; RunAction/RunState dispatch it on `this`).
typedef i32 (CUserLogic::*IconActHandler)();

#endif // GRUNTZ_GRUNTZ_CINGAMEICON_H
