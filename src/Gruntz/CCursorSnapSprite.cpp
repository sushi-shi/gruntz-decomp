// CCursorSnapSprite.cpp - the cursor-snap sprite game object (C:\Proj\Gruntz).
//
// Two trace-discovered CCursorSnapSprite methods, defined in ascending retail-RVA
// order:
//   Serialize         @0x011880 - the two-chain Serialize override.
//   ~CCursorSnapSprite @0x011920 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CCursorSnapSprite : CUserLogic (RTTI .?AVCCursorSnapSprite@@). Only offsets /
// code bytes are load-bearing; names are placeholders for the recovered engine
// identities.
#include <Gruntz/CCursorSnapSprite.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CCursorSnapSprite::Serialize @0x011880 - chain the shared CUserLogic serialize
// helper on `this`, and (only on success) the +0x34 sub-object's chain; both run
// the same (ar, tag, c, d) tuple. Returns the second chain's success normalized
// to a bool (the retail neg/sbb/neg idiom). The SAME archetype as
// CFortressFlag::Serialize (0x46410), minus the tag-8 sprite fixup.
RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)((char*)this + 0x34))
               ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)
           != 0;
}

// CCursorSnapSprite::~CCursorSnapSprite @0x011920 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CFortressFlag
// (0x010e90) / ~CTeleporter (0x010dd0); the empty body is enough for cl.
RVA(0x00011920, 0x44)
CCursorSnapSprite::~CCursorSnapSprite() {}

// CCursorSnapSprite::CCursorSnapSprite @0x03a340 - fold the shared CUserLogic(obj)
// init, name the bound object, snapshot its geometry id (+0x40), apply the single-
// image-ani geometry, bind the "A" bute node, then flag the sub-object (+0x08
// bit 2, +0x40 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0003a340, 0x16e)
CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyName("GAME_CURSORSNAPSPRITE");
    m_geoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
}

#include <rva.h>
