// RollingBall.h - the rolling-ball hazard game object (CRollingBall : CUserLogic,
// vftable 0x5e86fc, sizeof 0xa0). A CUserLogic leaf: its own state begins after
// the shared CUserLogic base region (ends +0x40) and runs to +0xa0.
//
// Modeled polymorphically (like every UserLogic leaf) so the empty dtor folds to
// the bare CUserLogic teardown (store the CUserLogic vptr 0x5e705c, ~EngStr on the
// +0x18 link, store the CUserBase vptr 0x5e70b4) under a /GX frame, the shape every
// UserLogic leaf dtor matches. Serialize is the slot-1 override (modeled as a plain
// method so its ?Serialize@...@@QAE.. name + RVA pin; the vtable slot is reloc-
// masked, the same way CSecretTeleporterTrigger::Serialize is wired).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing (campaign doctrine). Layout recovered from Update (0xb0140) and
// Serialize (0xb0fe0): +0x58 double (per-tile time), +0x60/+0x68 double (sub-tile
// pos), +0x70/+0x74 int (step dir), +0x78/+0x7c int (target tile coords), +0x80/
// +0x84 latches, +0x88..+0x94 (explosion timing), +0x98/+0x9c (move delta).
#ifndef GRUNTZ_GRUNTZ_ROLLINGBALL_H
#define GRUNTZ_GRUNTZ_ROLLINGBALL_H

#include <rva.h>

#include <Mfc.h> // CObject base + the two CString diagnostic temps in Update (/GX)

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

// ---------------------------------------------------------------------------
// The +0x34 serializable sub-object's chain (0x8c00, __thiscall ret 0x10). Run
// on `&this->m_34` (reached via `lea ecx,[edi+0x34]`). External/no-body so the
// call reloc-masks; the body is pinned in src/Stub/Discovered.cpp. Mirror of
// CSecretTeleporterTrigger's CSerialSub34 (UserLogic.cpp).
// ---------------------------------------------------------------------------
struct CRbSerialSub34 {
    i32 Chain(i32 a, i32 b, i32 c, i32 d); // 0x8c00
};

// ---------------------------------------------------------------------------
// The CArchive-like serializer the record is streamed through (Serialize's arg1).
// Modeled polymorphic (slot decls only, never defined -> no ??_7 emitted) so
// `ar->Read(buf,n)`/`ar->Write(buf,n)` lower to `mov eax,[ar]; push n; push buf;
// mov ecx,ar; call [eax+0x2c|0x30]`. Mirror of TileActionEvent.h's
// CTileActionArchive: +0x2c = Read (mode 7 = load), +0x30 = Write (mode 4 = store).
// ---------------------------------------------------------------------------
struct CRbArchive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual i32 Read(void* buf, i32 n);  // +0x2c
    virtual i32 Write(void* buf, i32 n); // +0x30
};

// ---------------------------------------------------------------------------
// CRollingBall : CUserLogic (vftable 0x5e86fc). Own state from +0x40 onward.
// The dtor (0x12f80) adds no destructible members, so it folds the bare
// CUserLogic teardown.
// ---------------------------------------------------------------------------
class CRollingBall : public CUserLogic {
public:
    CRollingBall(CGameObject* obj);   // 0x0af820 (folds CUserLogic(obj) + the ball setup)
    virtual ~CRollingBall() OVERRIDE; // 0x012f80 (folds the bare CUserLogic teardown)

    // Construct the class's activation-coordinate registry (g_rollingBallActReg
    // @0x6461b0) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x0afd60
    // Bind the per-frame handler (Update) to the activation key "A" via the shared
    // name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs(); // 0x0aff40

    i32 Serialize(CRbArchive* ar, i32 tag, i32 c, i32 d); // 0x0b0fe0 (vtable slot 1)
    i32 Update();                                         // 0x0b0140

    // --- CRollingBall own fields (placeholders; offsets load-bearing) ---
    i32 m_40;                  // +0x40  geometry id (m_38->m_1b4 snapshot)
    char m_pad44[0x58 - 0x44]; // CUserLogic ends +0x40
    double m_58;               // +0x58  per-tile move time (1000/RollingBallTimePerTile)
    double m_60;               // +0x60  sub-tile X position
    double m_68;               // +0x68  sub-tile Y position
    i32 m_70;                  // +0x70  X step direction (-1/0/1)
    i32 m_74;                  // +0x74  Y step direction (-1/0/1)
    i32 m_78;                  // +0x78  target tile X (<<5)
    i32 m_7c;                  // +0x7c  target tile Y (<<5)
    i32 m_80;                  // +0x80  explosion latch
    i32 m_84;                  // +0x84  fall latch
    i32 m_88;                  // +0x88  explosion start ms (lo)
    i32 m_8c;                  // +0x8c  explosion start ms (hi)
    i32 m_90;                  // +0x90  explosion window lo
    i32 m_94;                  // +0x94  explosion window hi
    i32 m_98;                  // +0x98  move delta lo
    i32 m_9c;                  // +0x9c  move delta hi
};

SIZE(CRollingBall, 0xa0);

#endif // GRUNTZ_GRUNTZ_ROLLINGBALL_H
