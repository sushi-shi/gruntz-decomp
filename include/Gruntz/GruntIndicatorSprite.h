// GruntIndicatorSprite.h - shared layout for the in-game grunt-indicator sprite
// objects (CGruntSelectedSprite / CGruntPowerupSprite / CGruntToySprite).
//
// Each is a CUserLogic-derived game object (vftables 0x5e705c / 0x5e70b4, the
// CUserLogic / CUserBase pair - proven by each class's 0x44 leaf dtor stamping
// 0x5e705c then 0x5e70b4 and tearing down the +0x18 link via the embedded
// ~EngStr, the SAME shape every UserLogic leaf dtor matches, e.g. ~CTimeBomb
// @0x012a70 / ~CInGameIcon @0x011d00). So despite the trace's "CGruntSprite"
// label, the dtor RTTI says these are plain CUserLogic leaves; the 0x44 is a
// DESTRUCTOR (the CUserLogic-folded leaf-dtor archetype), NOT a ctor.
//
// The accessor/updater methods anchor at +0x54/+0x58 (a tile coord pair) and
// poke the bound renderable (CUserLogic::m_10) + the +0x38 game object. They
// resolve the current grunt for the (m_54,m_58) cell from the global game
// registry's grunt table and copy that grunt's screen position into the bound
// renderable so the indicator tracks the grunt.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTINDICATORSPRITE_H
#define GRUNTZ_GRUNTINDICATORSPRITE_H

#include <rva.h>

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

// ---------------------------------------------------------------------------
// The level layer-clamp holder reached as a grunt's m_10->m_194. The Toy
// updater clamps the grunt's layer index into [m_64, m_68] and maps it through
// the +0x14 layer table into m_10->m_198. Only the touched offsets modeled.
// ---------------------------------------------------------------------------
struct CGruntLayerHolder {
    char m_pad00[0x14];
    i32* m_14; // +0x14  the per-index layer table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  lo layer bound
    i32 m_68; // +0x68  hi layer bound
};

// ---------------------------------------------------------------------------
// CGruntRenderable - the visible sprite the indicator binds to (CUserLogic::m_10
// AND each grunt entry's +0x10). The updaters read/copy its screen position
// (+0x5c/+0x60) and, for the Toy sprite, its layer fields (+0x190/+0x194/+0x198).
// ---------------------------------------------------------------------------
struct CGruntRenderable {
    char m_pad00[0x4c];
    i32 m_4c; // +0x4c  bute-set record (powerup setter)
    i32 m_50; // +0x50  display state (== 7)
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  visibility flag (== 1)
    i32 m_5c; // +0x5c  screen x
    i32 m_60; // +0x60  screen y
    char m_pad64[0x190 - 0x64];
    i32 m_190;                // +0x190  resolved layer index
    CGruntLayerHolder* m_194; // +0x194  layer-clamp holder
    i32 m_198;                // +0x198  mapped layer value
};

// ---------------------------------------------------------------------------
// CGruntEntry - one grunt slot in the registry's grunt table. The table base is
// g_gameReg->m_68 + 0x1c, indexed by (m_54*15 + m_58); each slot is a dword
// pointer to a grunt. The updaters read the grunt's renderable (+0x10), a "drawn"
// gate (+0x1d8) and a layer index (+0x198).
// ---------------------------------------------------------------------------
struct CGruntEntry {
    char m_pad00[0x10];
    CGruntRenderable* m_10; // +0x10  the grunt's renderable
    char m_pad14[0x198 - 0x14];
    i32 m_198; // +0x198  the grunt's current layer index
    char m_pad19c[0x1d8 - 0x19c];
    i32 m_1d8; // +0x1d8  the grunt's "drawn/visible" gate
};

// ---------------------------------------------------------------------------
// CIndicatorReg - the minimal game-registry view the indicator updaters use.
// Same singleton as CGameReg (?g_gameReg@@3PA...@@A @ 0x64556c); modeled with
// the grunt table at +0x68 and the bute-set table at +0x78 the powerup setter
// reads. Declared with the registry's own type so the data ref reloc-masks.
//   +0x68  m_68 : grunt table base (entries at +0x1c, dword stride)
//   +0x78  m_78 : the bute lookup table the powerup setter indexes
// ---------------------------------------------------------------------------
struct CIndicatorReg {
    char m_pad00[0x68];
    char* m_68; // +0x68  grunt table base (CGruntEntry* at +0x1c[idx])
    char m_pad6c[0x78 - 0x6c];
    char* m_78; // +0x78  bute-set table base
};

DATA(0x0024556c)
extern CIndicatorReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c

// ---------------------------------------------------------------------------
// A bound-object sub-object on the +0x38 game object: at +0x1a0 sits a helper
// whose Sync(arg) (0x15c360, __thiscall ret 4) flushes/advances the indicator's
// draw state. g_indicatorSync (0x6bf3bc, BSS) is the global arg it is handed.
// Both are external/no-body so the call + the load reloc-mask.
// ---------------------------------------------------------------------------
struct CIndicatorSyncHelper {
    void Sync(void* ctx); // 0x15c360 (__thiscall ret 4)
};
DATA(0x002bf3bc)
extern void* g_indicatorSync; // DAT_006bf3bc

// The bute store the powerup setter seeds the "A" node from (g_buteTree.Find).
// Owned by another TU; declared extern so `ecx=&g_buteTree; call Find` reloc-masks.
#include <Bute/ButeMgr.h>
DATA(0x002bf620)
extern CButeTree g_buteTree;

#endif // GRUNTZ_GRUNTINDICATORSPRITE_H
