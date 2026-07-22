#include <Gruntz/SpriteRefTable.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/AnimSink.h>
#include <Wap32/ZVec.h>

typedef i32 (CUserLogic::*CreationPointHandler)();

VTBL(CGruntCreationPoint, 0x001e81d4);


// CGruntCreationPoint::~CGruntCreationPoint @0x010730 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntCreationPoint() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x00010730, 0x44, ??1CGruntCreationPoint@@UAE@XZ)

#include <Gruntz/GameRegistry.h>

// CGruntCreationPoint::CGruntCreationPoint @0x3e520 - fold the shared
// CUserLogic(obj) init, flag the sub-object (+0x08 bit 1 via m_74==5 init), bind
// the cycle geometry "GAME_CYCLE100", resolve the selected sprite handle from
// g_gameReg's ref-index array (or the direct selector when m_134==1), re-seed the
// bound sprite's state trio + snap its screen position to the tile grid, then bind
// the "A" bute node.
//
// @early-stop
// register-pinning/spill wall (docs/patterns/zero-register-pinning.md +
// eh-ctor-vptr-restamp-position.md): body byte-identical (every op/offset/imm/
// branch incl. the m_134/ref-array selector branch matches retail). Residual:
// retail spills `obj` to its arg slot and reloads it once in the rare else branch,
// freeing edi for the constant 1; MSVC keeps obj in edi and pins constant 2 in
// ebx (extra push ebx -> a 4th callee-saved reg, shifting every stack-slot offset).
// Not source-steerable (global regalloc). ~80%.
RVA(0x0003e520, 0x1fd)
CGruntCreationPoint::CGruntCreationPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    if (m_object->m_sortKey != 5) {
        m_object->m_sortKey = 5;
        m_object->m_flags |= 0x20000;
    }
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);

    i32 key = m_object->m_124;
    i32 idx;
    if (g_gameReg->m_134 == 1) {
        idx = key;
    } else if (g_gameReg->m_options[key].m_liveGate != 0) {
        idx = g_gameReg->m_options[key].m_008;
    } else {
        m_38->m_flags |= 0x10000;
        idx = reinterpret_cast<i32>(obj);
    }
    i32 sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);

    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xa;
    m_object->m_drawFillArg = sel;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}


#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
#include <Gruntz/Play.h> // ChannelSlots_FindFree (ex .cpp extern)

// g_creationPointActReg (0x00244700): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x00244700, 0x0, ?g_creationPointActReg@@3UCActReg@@A)

// CGruntCreationPoint::Serialize @0x03e7a0 - chain the shared CUserLogic serialize
// helper, then the +0x34 sub-object's chain; on the post-load tag (tag == 8),
// re-resolve the selected sprite from g_gameReg's ref-index array (the SAME selector
// as the ctor: direct when m_134==1, else the +0x158 row (stride 71), else a free
// channel slot) and re-seed the bound object's draw trio.
// @early-stop
// GetSel inline-vs-call + regalloc wall (~72%): the body is byte-faithful (the
// SerializeMove + the +0x34 Chain + the m_134-first / ref-array (stride 71) / free-
// channel selector + the sel==0 fallback + the draw-trio store all match retail).
// Residual: retail CALLS the out-of-line CSpriteRefTable::GetSel (0xe23c0) here while
// MSVC inlines the header-inline copy into this small fn (an inlining-heuristic
// difference, not source-steerable), and it pins `this` in edi vs retail's esi. The
// ctor @0x3e520 shares the same wall (~80%).
RVA(0x0003e7a0, 0xd7)
i32 CGruntCreationPoint::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }
    if (tag != 4 && tag == 8) {
        i32 idx;
        if (g_gameReg->m_134 == 1) {
            idx = m_object->m_124;
        } else if (g_gameReg->m_options[m_object->m_124].m_liveGate != 0) {
            idx = g_gameReg->m_options[m_object->m_124].m_008;
        } else {
            idx = ChannelSlots_FindFree();
        }
        i32 sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        if (sel == 0) {
            sel = g_gameReg->m_spriteFactory->GetSel(1, sel);
        }
        m_object->m_drawActive = 1;
        m_object->m_drawFillCmd = 0xa;
        m_object->m_drawFillArg = sel;
    }
    return 1;
}

RVA(0x0003e8e0, 0x15)
void CGruntCreationPoint::InitActReg() {
    g_creationPointActReg.Construct(2000, 2010);
}

RVA(0x0003e960, 0x102)
void CGruntCreationPoint::FireActivation(i32 coord) {
    CreationPointHandler* e = reinterpret_cast<CreationPointHandler*>(g_creationPointActReg.ResolveEntry(coord));
    if (*e != 0) {
        CreationPointHandler* e2 = reinterpret_cast<CreationPointHandler*>(g_creationPointActReg.ResolveEntry(coord));
        (this->*(*e2))();
    }
}

// CGruntCreationPoint::RegisterActs @0x03eac0 - bind the per-frame handler
// (AdvanceAnim @0x03ecc0) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0003eac0, 0x18d)
void CGruntCreationPoint::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<CreationPointHandler*>(g_creationPointActReg.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntCreationPoint::AdvanceAnim);
}

RVA(0x0003ecc0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

