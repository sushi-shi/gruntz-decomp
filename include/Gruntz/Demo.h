#ifndef GRUNTZ_GRUNTZ_CDEMO_H
#define GRUNTZ_GRUNTZ_CDEMO_H

#include <rva.h>         // OVERRIDE
#include <Gruntz/Play.h> // canonical CPlay base (typed MFC members -> the /GX fold)

class CDemo : public CPlay {
public:
    // Empty ctor: cl runs the CPlay/CState base chain + auto-stamps ??_7CDemo
    // (masks retail 0x5e9f0c). The factory's `new CDemo` (GruntzMgrTransition.cpp)
    // drives the out-of-line COMDAT vtable emission.
    CDemo() {}
    virtual ~CDemo() OVERRIDE; // slot 0  0x8d0d0 (/GX dtor, GruntzMgr.cpp)
    // slot 1  0x3bfa0 (Demo.cpp; retail ??_7CDemo slot 1 = ILT 0x34f9 -> 0x3bfa0)
    virtual i32 LoadGameAssetNamespaces(i32, i32, i32) OVERRIDE;
    // slot 2: retail ??_7CDemo slot 2 holds 0x3c010 - itself a 5-byte jmp to ILT
    // 0x1dc5 -> 0xc8700 == CPlay::ReleaseResources (byte-verified). CDemo has NO
    // distinct slot-2 body of its own (the ex "DerivedCleanup" decl aliased that
    // thunk); declared-only here so the emitted slot stays an own-override.
    virtual void ReleaseResources() OVERRIDE;      // slot 2
    virtual GameStateId Update() OVERRIDE;         // slot 4
    virtual i32 Render() OVERRIDE;                 // slot 5  0x3c220 (Demo.cpp)
    virtual i32 Vslot15() OVERRIDE;                // slot 21 0x3c030 (GruntzMgrTransition.cpp)
    virtual i32 BuildWorldLevelPath(i32) OVERRIDE; // slot 42 (+0xa8) i32(i32)

    // CPlay ends at +0x51c (canonical); CDemo's own words begin at +0x520 (the
    // slot-1 loader stores [this+0x520]), so a 4-byte gap sits between the base
    // end and m_520.
    char m_pad51c[0x520 - 0x51c];
    i32 m_520; // +0x520  the slot-1 loader stores 0x124f80 on success
    i32 m_524; // +0x524
};
SIZE_UNKNOWN();

extern "C" const i32 g_rotTableA_60d008[27]; // 0x60d008  CW transitions
extern "C" const i32 g_rotTableB_60d078[27]; // 0x60d078  CCW transitions

#endif // GRUNTZ_GRUNTZ_CDEMO_H
