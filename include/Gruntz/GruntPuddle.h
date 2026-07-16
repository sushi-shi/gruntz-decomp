// GruntPuddle.h - the grunt-puddle game object, a CUserLogic-derived leaf
// (vftables 0x5e705c / 0x5e70b4, the CUserLogic / CUserBase pair). Confirmed
// CGruntPuddle by its own string constants: "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2"
// (the lookup-geometry sprite key @ 0x60c1c0) and "B" (the one-char bute key @
// 0x60d1bc) both referenced from the place handler.
//
// Modeled as a CUserLogic leaf so the empty dtor (0x10d10) folds to the bare
// CUserLogic teardown (store 0x5e705c, ~EngStr on +0x18, store 0x5e70b4) under a
// /GX frame - the same shape every UserLogic leaf dtor matches.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes
// are load-bearing (campaign doctrine). Layout recovered from the method field
// stores: Place040c30 writes +0x54/+0x58/+0x5c/+0x60/+0x64/+0x68/+0x6c, +0x40,
// and CUserLogic::m_30; Remove040d20 reads +0x54/+0x58/+0x60; SetBute07d810
// writes CUserLogic::m_30.
#ifndef GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
#define GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H

#include <rva.h>

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/ActReg.h>     // CLogicActTable (the slot-4 activation-dispatch table)
#include <Gruntz/UserLogic.h>  // CUserLogic : CUserBase, EngStr, CGameObject
#include <Gruntz/InGameIcon.h> // CGameReg / g_gameReg / CIconFactory / CIconTileGrid

// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

// The bute store the place/set paths query (mov ecx,&g_buteTree; call Find).
// g_buteTree + CButeTree are declared canonically in <Bute/ButeTree.h>, reached here
// via UserLogic.h -> <Bute/ButeMgr.h> -> <Bute/ButeTree.h>.

// The intrusive object list reached as g_gameReg->m_68 (cast from the void* the
// shared CGameReg models). Remove040d20 walks it for the node whose +0x8 data
// pointer is this, then unlinks it via RemoveAt. Node = {next@0, prev@4, data@8}.
SIZE_UNKNOWN(CObjListNode);
struct CObjListNode {
    CObjListNode* m_next; // +0x00
    CObjListNode* m_prev; // +0x04
    void* m_data;         // +0x08
};
SIZE_UNKNOWN(CObjList);
struct CObjList {
    char m_pad00[0x4];
    CObjListNode* m_head;             // +0x04  list head
    void RemoveAt(CObjListNode* pos); // 0x1b4ac7 (__thiscall, unlink + free node)
};

// The sub-object embedded at CGameObject+0x1a0 the remove path notifies. Its one
// method (0x15c360, __thiscall, 1 arg) is external/no-body so the call reloc-masks.
SIZE_UNKNOWN(CGruntPuddleSink);
struct CGruntPuddleSink {};

// The global the remove path hands the sink (_g_6bf3bc; the draw-delta mirror).
// Defined in SpriteResource.cpp/Projectile.cpp; declared extern "C" here so the
// value-load reloc-masks against the already-matched symbol.
extern "C" u32 g_engineFrameDelta;

// The one-char bute key "B" (0x60d1bc) the place/set paths look up is the SAME
// rdata as CInGameIcon.h's s_actKeyB (DATA 0x0020d1bc) - reuse it (don't
// re-declare the address; the duplicate-RVA guard would fire).

// The lookup-geometry sprite key "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2" (0x60c1c0).
extern char g_puddleSpriteKey[]; // s_..._0060c1c0

// ---------------------------------------------------------------------------
// CGruntPuddle : CUserLogic. Its own state begins at +0x40 (CUserLogic ends at
// +0x40). The dtor (0x10d10) adds no destructible members, so it folds the bare
// CUserLogic teardown.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CGruntPuddle);
class CGruntPuddle : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010cc0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTPUDDLE;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CGruntPuddle(CGameObject* obj);   // 0x040490
    virtual ~CGruntPuddle() OVERRIDE; // 0x010d10

    i32 Place(i32 a0, i32 a1, i32 a2, i32 a3); // 0x040c30
    i32 Remove();                              // 0x040d20
    void SetBute(char* key);                   // 0x07d810
    // FireActivation (0x40750): slot-4 (UserLogicVfunc2) override - resolve `id` in
    // the class dispatch table g_logicDispatch_6445e8; if the resolved entry holds a
    // handler, re-resolve and dispatch it __thiscall on `this`. Same archetype as
    // CTeleporter::FireActivation.
    void FireActivation(i32 id);
    // Serialize (0x40e50): two-chain (CUserLogic base + the +0x34 sub-object) then a
    // tag-dispatched field round-trip - tag 4 writes / tag 7 reads the 7 own i32
    // fields via the archive vtable, tag 8 re-resolves the placed sprite from the
    // game registry. Same archetype as CGruntHealthSprite::Serialize.
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d);

    // --- CGruntPuddle own fields (placeholders; offsets load-bearing) ---
    CAniElement* m_savedGeoId; // +0x40  geometry id (m_38->m_1a0.m_14 snapshot)
    char m_pad44[0x54 - 0x44];
    i32 m_tileX;      // +0x54  owner tile X (m_object->m_screenX >> 5)
    i32 m_tileY;      // +0x58  owner tile Y (m_object->m_screenY >> 5)
    i32 m_pending;    // +0x5c  not-yet-placed gate (ctor 1; cleared once placed)
    i32 m_placed;     // +0x60  "placed" flag
    i32 m_placeArg3;  // +0x64  Place() arg3 snapshot
    i32 m_placeArg0;  // +0x68  Place() arg0 snapshot
    i32 m_placeIndex; // +0x6c  Place() arg1 snapshot (icon-factory index)
};
VTBL(CGruntPuddle, 0x1e8124);

// The class's activation-dispatch table (CLogicActTable @0x6445e8); filled by
// RegisterLogic_6445e8 (LogicActRegistrars.cpp), read by FireActivation. Owner
// (DATA binding) is LogicActRegistrars.cpp; declared extern here so the loads
// reloc-mask.
extern CLogicActTable g_logicDispatch_6445e8;

// A dispatch-table entry: its first dword is the class activation handler, stored
// by the registrar as a free-fn ptr but dispatched __thiscall on `this` - a 4-byte
// single-inheritance PMF gives the plain `mov ecx,this; call [entry]` code.
typedef i32 (CGruntPuddle::*PuddleActHandler)();
struct CPuddleActEntry {
    PuddleActHandler m_fn;
};
SIZE_UNKNOWN(CPuddleActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
