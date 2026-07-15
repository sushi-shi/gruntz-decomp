// GruntBehaviorLeaf.h - the per-frame grunt "decay/wand" AI leaf (C:\Proj\Gruntz).
//
// @identity-TODO: CGruntBehaviorLeaf is a PLACEHOLDER name for the grunt-behavior
// logic leaf that extends CUserLogic (base at +0) with a decay-timer + anim state.
// Its real RTTI identity is not yet recovered (no caller/vtable/RTTI pin found for
// the 0x870-byte leaf); only the OFFSETS + code bytes are load-bearing. Extracted
// out of the UserLogic god-TU so the three decay/wand bodies home onto one class.
#ifndef GRUNTZ_CGRUNTBEHAVIORLEAF_H
#define GRUNTZ_CGRUNTBEHAVIORLEAF_H

#include <rva.h>

#include <Gruntz/UserLogic.h>        // CUserLogic base + CGameObject
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor (the +0x1a0 arrival probe, value member)

class CImageSet; // <Image/ImageSet.h> (m_194->SetAllTypes; full def only in the .cpp)

SIZE_UNKNOWN(CDecayMgr);
struct CDecayMgr { // m_154 - the bound draw-state manager
    char m_pad00[0x8];
    i32 m_8; // +0x08 dirty flags
    char m_pad0c[0x40 - 0xc];
    i32 m_40; // +0x40 flags
    char m_pad44[0x194 - 0x44];
    CImageSet* m_194; // +0x194
    char m_pad198[0x1a0 - 0x198];
    CAniAdvanceCursor m_1a0; // +0x1a0
};

// (the ex-`CDecayAnim` view is GONE. It was not an anim controller at all - it was five
// fabricated method names standing in for functions on THREE DIFFERENT REAL CLASSES, all
// already reconstructed in the tree. Every one of its "RVAs" was an ILT jmp-thunk; chased
// through to the real bodies they are:
//   Anim2a72      thunk 0x2a72 -> 0x79fb0  CTriggerMgr::NotifyCell(row,col,z)      [void]
//   SetAnim       thunk 0x2e96 -> 0x6bcb0  CTriggerMgr::CellDispatch(row,col,k,arg)
//   PlayAnimEx    thunk 0x3003 -> 0x7a180  CTriggerMgr::SpawnPuddle(x,y,...)
//   DrawAnimAt    thunk 0x1073 -> 0x7b440  CTriggerMgr::BuildRockBreakParticles(...)
//   PlayStateAnim thunk 0x3945 -> 0x75e90  CTriggerMgr::LoadTileArrivalFx(...)
// Four of the five really return i32; the view declared them all `void`, throwing away the
// eax retail's callers consume. The old "PROVEN-heterogeneous slot" reading of +0x260 is
// OVERTURNED (2026-07-15): all five targets are CTriggerMgr methods - the "CRockBreakMgr"
// and "CTileWireLogic" receivers were themselves placeholder views of CTriggerMgr (both
// dissolved), so the slot is HOMOGENEOUS and typed CTriggerMgr* below (== CGrunt::m_tileMgr,
// the same +0x260 board).)

SIZE_UNKNOWN(CGruntBehaviorLeaf);
class CGruntBehaviorLeaf : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 LoadGruntDecayConfig();    // 0x612a0
    i32 LoadGruntDecayConfig2();   // 0x61570
    i32 LoadWandGruntItemConfig(); // 0x65a60
    // Leaf's own reloc-masked __thiscall helpers.
    void RefreshDecay();                     // 0x22de
    void SetDecayTarget(i32 a);              // 0x3c29
    void InitAnimState(i32 a, i32 b, i32 c); // 0x136b

    // Members beyond CUserLogic's 0x40 base.
    char m_pad40[0x154 - 0x40];
    CDecayMgr* m_drawState; // +0x154 bound draw-state manager
    char m_pad158[0x170 - 0x158];
    i32 m_gruntSubState; // +0x170 grunt sub-state
    char m_pad174[0x1c0 - 0x174];
    char* m_gruntTypeTag; // +0x1c0 grunt-type bute tag
    i32 m_1c4;            // +0x1c4
    char m_pad1c8[0x1e4 - 0x1c8];
    i32 m_downtimeLatch; // +0x1e4 latch flag
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_animArg0; // +0x1ec anim arg
    i32 m_animArg1; // +0x1f0 anim arg
    i32 m_animArg2; // +0x1f4 anim arg
    char m_pad1f8[0x258 - 0x1f8];
    i32 m_typeDisc; // +0x258 type discriminator (0x3b = no-downtime)
    char m_pad25c[0x260 - 0x25c];
    class CTriggerMgr* m_260; // +0x260 the tile/trigger board (== CGrunt::m_tileMgr;
                              //        was void* under the overturned "heterogeneous" note)
    char m_pad264[0x360 - 0x264];
    i32 m_gruntMode; // +0x360 grunt mode
    char m_pad364[0x36c - 0x364];
    i32 m_animSuppress; // +0x36c anim-suppress gate
    char m_pad370[0x380 - 0x370];
    i32 m_380; // +0x380
    char m_pad384[0x3e4 - 0x384];
    i32 m_3e4;    // +0x3e4
    i32 m_3e8;    // +0x3e8
    i32 m_health; // +0x3ec health
    i32 m_3f0;    // +0x3f0
    char m_pad3f4[0x460 - 0x3f4];
    i32 m_460; // +0x460
    char m_pad464[0x830 - 0x464];
    i32 m_decayTimerLo;    // +0x830 timer start (lo)
    i32 m_decayTimerHi;    // +0x834 (hi, always 0)
    i32 m_decayDurationLo; // +0x838 timer duration (lo)
    i32 m_decayDurationHi; // +0x83c (hi, always 0)
    char m_pad840[0x860 - 0x840];
    i32 m_wandTimerLo;    // +0x860 wand timer start (lo)
    i32 m_wandTimerHi;    // +0x864 (hi)
    i32 m_wandDowntimeLo; // +0x868 wand downtime (lo)
    i32 m_wandDowntimeHi; // +0x86c (hi)
};

#endif // GRUNTZ_CGRUNTBEHAVIORLEAF_H
