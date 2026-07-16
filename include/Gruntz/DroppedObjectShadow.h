// DroppedObjectShadow.h - the dropped-object shadow eyecandy (C:\Proj\Gruntz),
// a CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). The /GX leaf dtor + the
// 1-arg ctor (0xc7490) are reconstructed here. NOTE: 0xc62e0 (Ghidra-labeled
// "LoadAttributes@CDroppedObjectShadow") was a trace mis-attribution - it is
// really CObjectDropper::Update (a LARGER sibling class); reconstructed byte-exact
// in src/Gruntz/ObjectDropper.cpp. Offsets + code bytes are the load-bearing
// facts; field names are placeholders.
#ifndef GRUNTZ_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_CDROPPEDOBJECTSHADOW_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

// The serialize stream: the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it). Pointer-only here, so the fwd decl + typedef suffice;
// an elaborated `struct CSerialArchive*` would re-declare a DISTINCT class and
// silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

class CDroppedObjectShadow : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012620, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DROPPEDOBJECTSHADOW;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CDroppedObjectShadow(CGameObject* obj);   // 0xc7490 (1-arg leaf ctor)
    virtual ~CDroppedObjectShadow() OVERRIDE; // 0x12670 (folds the CUserLogic teardown)
    // The slot-1 serialize impl (plain method: ?Serialize name + RVA pin, vtable
    // slot reloc-masked, like CDroppedObject::Serialize).
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0xc7b40
    // The activation-registry facet (ex ActRegSiblings.cpp's "CSiblingActorB" -
    // identity recovered: its registry construct 0xc76d0 sits right after this
    // class's ctor, and its per-frame Advance spawns the "DroppedObject" sprite
    // on the drop frame - the shadow IS the dropper's drop herald):
    static void InitActReg();       // 0xc76d0 (build g_shadowActReg over [2000,2010])
    void FireActivation(i32 coord); // 0xc7750 (look up + fire the registered handler)
    static void RegisterActs();     // 0xc78b0 (bind Advance to the "A" key)
    i32 Advance();                  // 0xc7ab0 (per-frame: advance anim; drop frame -> spawn)
    CAniElement* m_savedGeoId;               // +0x40  m_38->m_1a0.m_14 snapshot
    char m_pad44[0x54 - 0x44];
};
VTBL(CDroppedObjectShadow, 0x1e787c);
SIZE(CDroppedObjectShadow, 0x54);

// The shadow's activation entry: its first dword is the registered handler,
// dispatched __thiscall on `this` (4-byte single-inheritance PMF; CDroppedObjectShadow
// is complete above so the PMF stays 4 bytes). Was the .cpp-local CShadowActEntry view.
struct CShadowActEntry {
    i32 (CDroppedObjectShadow::*m_fn)();
};
SIZE_UNKNOWN(CShadowActEntry);

#endif // GRUNTZ_CDROPPEDOBJECTSHADOW_H
