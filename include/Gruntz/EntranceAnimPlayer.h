// EntranceAnimPlayer.h - CEntranceAnimPlayer, the per-grunt animation-player /
// draw-state object at CGrunt+0x154 (== CGruntBehaviorLeaf::m_drawState - the ex
// "CDecayMgr" view was THIS class: same +0x08 flags, +0x40 visible bit, +0x194
// CImageSet*, +0x1a0 embedded CAniAdvanceCursor). Extracted from <Gruntz/Grunt.h>
// (which re-includes it) so the leaf header shares the ONE shape.
//
// RECONCILIATION EVIDENCE (the pending player == CGameObject unification, the
// wide 0x1dc game object of <Gruntz/UserLogic.h>): the player's method RVAs are
// CGameObject's own bodies (CacheFirstFrame/CacheFrameIndexed == ApplyLookupSprite
// 0x1504d0, CacheFrame == ApplyName 0x150540, ApplyLookupGeometry 0x1505b0,
// ApplyGeometryDirect 0x58b60); its ctor-seeded m_e8/m_f4 are m_collCategory/
// m_collMask; its +0x1a0 cursor tail ends at +0x1dc == SIZE(CGameObject); and
// AniAdvanceCursor.h's vtable proof (~CWwdGameObjectA/B stamp ??_7CAniAdvanceCursor
// at +0x1a0) is about this same family. CGruntAnimState (CGrunt::m_38, Grunt.h) is
// a THIRD view of the shape. Folding all of them onto CGameObject also requires
// re-modeling CGameObject's +0x1a0..+0x1dc tail (m_geoId/m_1c0/m_1c8 are cursor
// interior fields) tree-wide - deferred; see the stage-C handback note.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing.
#ifndef GRUNTZ_GRUNTZ_ENTRANCEANIMPLAYER_H
#define GRUNTZ_GRUNTZ_ENTRANCEANIMPLAYER_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                     // CMapStringToPtr (the sprite-set lookup map)
#include <Gruntz/AniAdvanceCursor.h> // the REAL +0x1a0 cursor (embedded value member)

struct CSprite; // opaque looked-up sprite

// (The ex-`CMapStringToOb` view is DISSOLVED - the member is the real map. It is a
// CMapStringToPtr: every retail Lookup on it calls 0x1b8438, the CMapStringToPtr band
// (mfc_class arbitration; verified in 0x49c60 / 0x65e80 / 0x68880 disasm + Warlord's
// FID note). The old CMapStringToOb typing bound the WRONG library body, 0x1b8008.)

SIZE_UNKNOWN(CEntranceSpriteMgr);
struct CEntranceSpriteMgr {
    // 1-arg lookup returning the resolved sprite directly (the EXIT/RUN loaders
    // use this form; the 2-arg m_10map.Lookup writes through an out-param instead).
    // (Formerly reached by a per-TU CDDrawSubMgrLeaf facet cast; folded here.)
    void* LookupValue_06b2a0(const char* key); // 0x6b2a0 (external/reloc-masked)

    char m_pad0[0x10];
    CMapStringToPtr m_10map; // +0x10 (Ptr band 0x1b8438 - see note above)
};

// The player's resource object (name->sprite-set lookup holder) at m_c.
// BuildGruntExitAnimation drives the +0x2c manager's Apply (0x6b2e0, 2-arg
// __thiscall) with the resolved sprite. External/no-body (reloc-masked).
SIZE_UNKNOWN(CEntranceResMgr);
struct CEntranceResMgr {
    char m_pad0[0x2c];
    CEntranceSpriteMgr* m_2c; // +0x2c
};

// (The former `CEntranceAnimSub` data-less "geometry sub-player" view is
// DISSOLVED (2026-07-16): the +0x1a0 sub-object IS the real 0x3c-byte
// CAniAdvanceCursor, embedded as a value member below. Its "SetGeometry"
// (FUN_0055c2d0) is CAniAdvanceCursor::Setup_15c2d0 and its probe (0x15c360)
// CAniAdvanceCursor::Advance - both real, defined methods now bound at every
// ex-cast site. The old "cannot retype - 0x3c would overrun m_1a4/m_1b4"
// objection had it backwards: m_1a4/m_1b4/m_1c0/m_1c8 WERE the cursor's own
// +0x04/+0x14/+0x20/+0x28 interior fields under duplicate names.)

SIZE_UNKNOWN(CEntranceAnimPlayer);
class CEntranceAnimPlayer {
public:
    // Geometry setter that forwards to the +0x1a0 cursor then, if flag!=0,
    // a 2nd setter (FUN_00458b60, ret 8). PlaySound's IDLE arm drives it directly.
    // A 1-arg setter the WALK/E arms call on the player itself (FUN_00550540,
    // FUN_005504d0 is the 2-arg form). Takes the resolved cell name.
    // The death/freeze finalize 2-arg geometry setter (FUN_005505b0); takes the
    // resolved key string + a flag. External/reloc-masked.
    // The combat-reaction dispatch (@0x646b0) drives the player through its
    // CGameObject base name/sprite setters (0x150540 / 0x1504d0, not the 0x55xxxx
    // entrance forms). External/no-body so the call rel32 reloc-masks.
    // The CGameObject-base lookup-geometry setter (same 0x1505b0 slot CHudSprite
    // uses) the death/freeze finalize drives with the DEATHZ_SPARKLE/UNFREEZE keys.
    //
    // The CGameObject-base name/sprite/geometry setters the asset loaders drive on the
    // player directly (external/reloc-masked so the call rel32 masks; the former per-TU
    // CGruntSprite/CGruntAnimPlayer facet views are folded here). The player IS the
    // created game object (see the reconciliation block in the file header).
    void CacheFirstFrame(const char* name); // 0x1504d0
    // The 2-arg frame-cache form at the same 0x1504d0 slot (the CGameObject-base
    // ApplyLookupSprite(key, flag); the entrance re-stamp steps drive it with the
    // built cell-name buffer + the active descriptor's first-element frame index).
    void CacheFrameIndexed(const char* key, i32 frame);   // 0x1504d0 (2-arg)
    void CacheFrame(const char* key, i32 frame);          // 0x150540
    void ApplyLookupGeometry(const char* key, i32 frame); // 0x1505b0
    void ApplyGeometryDirect(CAniElement* src, i32 flag); // 0x58b60

    // The +0x1a0 anim-advance cursor accessor, kept for the ~30 existing
    // `->Cursor()->` call sites; now a plain address-of (the cast is gone -
    // m_1a0 IS the real cursor).
    CAniAdvanceCursor* Cursor() {
        return &m_1a0;
    }

    char m_pad0[0x8];
    i32 m_8;              // +0x08  state-flag word (death loader |= 1 / |= 0x10000;
                          //        == the ex CDecayMgr's "dirty flags" - one field)
    CEntranceResMgr* m_c; // +0x0c  resource object (lookup table holder)
    char m_pad10[0x40 - 0x10];
    i32 m_40; // +0x40  flags (bit0 = visible/active; the decay loader sets it -
              //        ex CDecayMgr::m_40 == CGameObject::m_stateFlags)
    char m_pad44[0xe8 - 0x44];
    i32 m_e8; // +0xe8  (ctor = 0x100000; == CGameObject::m_collCategory)
    i32 m_ec; // +0xec  (ctor = 0x3d1)
    i32 m_f0; // +0xf0  (ctor = 1)
    i32 m_f4; // +0xf4  (ctor |= 0x103f; == CGameObject::m_collMask)
    char m_padf8[0x148 - 0xf8];
    i32 m_148;   // +0x148
    i32 m_14c;   // +0x14c
    void* m_150; // +0x150
    char m_pad154[0x18c - 0x154];
    i32 m_18c; // +0x18c
    char m_pad190[0x194 - 0x190];
    class CImageSet* m_194; // +0x194  cached image set (SetAllTypes; ex CDecayMgr::m_194)
    i32 m_198;              // +0x198
    i32 m_19c;              // +0x19c
    // +0x1a0..+0x1db: the embedded 0x3c CAniAdvanceCursor (vptr @+0x1a0; ends at
    // +0x1dc == the 0x1dc CGameObject size). The former m_1a4/m_1b4/m_1c0/m_1c8
    // duplicate names were its interior fields: m_1a4 == cursor +0x04 (CLoadable),
    // m_1b4 == m_1a0.m_14 (the active CAniElement* descriptor), m_1c0 == m_1a0.m_20
    // (per-frame timer; "entrance-done flag B"), m_1c8 == m_1a0.m_28 (paused/done;
    // "entrance-done flag A"). Same offsets, same bytes - one shape.
    CAniAdvanceCursor m_1a0; // +0x1a0 the anim-advance / geometry cursor
};

#endif // GRUNTZ_GRUNTZ_ENTRANCEANIMPLAYER_H
