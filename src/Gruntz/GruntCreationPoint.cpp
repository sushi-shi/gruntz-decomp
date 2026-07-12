// GruntCreationPoint.cpp - the grunt creation-point game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CGruntCreationPoint methods, defined in ascending
// retail-RVA order:
//   ~CGruntCreationPoint @0x010730 - the /GX leaf dtor (folds the CUserLogic teardown).
//   AdvanceAnim          @0x03ecc0 - the per-frame animation-advance (ret 0).
//
// CGruntCreationPoint : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/AnimSink.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CGruntCreationPoint::*CreationPointHandler)();
struct CCreationPointActEntry {
    CreationPointHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x644700), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). Was a per-file
// duplicate of the <Gruntz/ActReg.h> CActReg archetype (layout + ResolveEntry); now
// derives from it, keeping its own placeholder name so the DATA-pinned global is
// unchanged.
struct CCreationPointActReg : public CActReg {};
DATA(0x00244700)
extern CCreationPointActReg g_creationPointActReg; // 0x644700

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Defined in SpriteResource.cpp/Projectile.cpp; declared extern "C"
// here so the value-load reloc-masks against the already-matched symbol.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// CGruntCreationPoint::~CGruntCreationPoint @0x010730 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x00010730, 0x44)
CGruntCreationPoint::~CGruntCreationPoint() {}

// ---------------------------------------------------------------------------
// The level sprite-ref table (g_gameReg->m_74). GetSel(i, bAlt) (via the 0x4165
// GetByIndex thunk) returns the selected sprite handle for ref-row i; modeled
// NO-body so the call reloc-masks.
// One ref-index array slot (8-byte stride; first dword is the ref-row index).
struct CreationRefSlot {
    i32 m_idx; // +0x00  ref-row index
    i32 m_04;  // +0x04  arm gate (the +0x18 probe reads slot[+3].m_idx)
};
// The global game registry (WwdGameReg, RVA 0x24556c; wwdfile owns the DATA
// label). m_134 == 1 is the "direct selector" mode; otherwise the ref-index
// array at +0x158 (row stride 71 slots) resolves the selector.
struct CreationGameReg {
    char m_pad0[0x74];
    CSpriteRefTable* m_74; // +0x74  level sprite-ref table
    char m_pad78[0x134 - 0x78];
    i32 m_134; // +0x134 mode discriminator
    char m_pad138[0x158 - 0x138];
    CreationRefSlot m_158[1]; // +0x158 base of the ref-index array
};
DATA(0x0024556c)
extern "C" CreationGameReg* g_gameReg;

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

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
CGruntCreationPoint::CGruntCreationPoint(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    if (m_object->m_latchedAnimId != 5) {
        m_object->m_latchedAnimId = 5;
        m_object->m_flags |= 0x20000;
    }
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);

    i32 key = m_object->m_124;
    i32 idx;
    if (g_gameReg->m_134 == 1) {
        idx = key;
    } else if (g_gameReg->m_158[key * 71 + 3].m_idx != 0) {
        idx = g_gameReg->m_158[key * 71].m_idx;
    } else {
        m_38->m_flags |= 0x10000;
        idx = (i32)obj;
    }
    i32 sel = g_gameReg->m_74->GetSel(idx, 0);

    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xa;
    m_object->m_drawFillArg = sel;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// ChannelSlots_FindFree (0x33e1 thunk) - the ref-slot fallback selector when the
// ref-index array has no armed row. External/no-body (reloc-masked).
extern "C" i32 ChannelSlots_FindFree();

#include <Gruntz/SerialObjRef.h> // the +0x34 sub-object's serialize chain

// CGruntCreationPoint::Serialize @0x03e7a0 - chain the shared CUserLogic serialize
// helper, then the +0x34 sub-object's chain; on the post-load tag (tag == 8),
// re-resolve the selected sprite from g_gameReg's ref-index array (the SAME selector
// as the ctor: direct when m_134==1, else the +0x158 row (stride 71), else a free
// channel slot) and re-seed the bound object's draw trio.
// @early-stop
// GetSel inline-vs-call + regalloc wall (~72%): the body is byte-faithful (the
// SerializeChain + the +0x34 Chain + the m_134-first / ref-array (stride 71) / free-
// channel selector + the sel==0 fallback + the draw-trio store all match retail).
// Residual: retail CALLS the out-of-line CSpriteRefTable::GetSel (0xe23c0) here while
// MSVC inlines the header-inline copy into this small fn (an inlining-heuristic
// difference, not source-steerable), and it pins `this` in edi vs retail's esi. The
// ctor @0x3e520 shares the same wall (~80%).
RVA(0x0003e7a0, 0xd7)
i32 CGruntCreationPoint::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    if (!SerialRef34()->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    if (tag != 4 && tag == 8) {
        i32 idx;
        if (g_gameReg->m_134 == 1) {
            idx = m_object->m_124;
        } else if (g_gameReg->m_158[m_object->m_124 * 71 + 3].m_idx != 0) {
            idx = g_gameReg->m_158[m_object->m_124 * 71].m_idx;
        } else {
            idx = ChannelSlots_FindFree();
        }
        i32 sel = g_gameReg->m_74->GetSel(idx, 0);
        if (sel == 0) {
            sel = g_gameReg->m_74->GetSel(1, sel);
        }
        m_object->m_drawActive = 1;
        m_object->m_drawFillCmd = 0xa;
        m_object->m_drawFillArg = sel;
    }
    return 1;
}

// CGruntCreationPoint::InitActReg @0x03e8e0 - construct the class's activation-
// coordinate registry singleton (g_creationPointActReg @0x644700) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710, through the 0x3742
// import thunk). Free init thunk.
RVA(0x0003e8e0, 0x15)
void CGruntCreationPoint::InitActReg() {
    ((CZDArrayDerived*)&g_creationPointActReg)->Construct(2000, 2010);
}

// CGruntCreationPoint::FireActivation @0x03e960 - look the activation coordinate
// up in the class registry (g_creationPointActReg); if the resolved entry carries a
// registered handler PMF, resolve it again and dispatch it __thiscall on `this`.
// The SAME archetype as CParticlez::FireActivation (0x046d30) - the double
// ResolveEntry + PMF dispatch.
RVA(0x0003e960, 0x102)
void CGruntCreationPoint::FireActivation(i32 coord) {
    CCreationPointActEntry* e = (CCreationPointActEntry*)g_creationPointActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CCreationPointActEntry* e2 =
            (CCreationPointActEntry*)g_creationPointActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeCount;
        void** list = (void**)g_typeNodes;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CCreationPointActEntry*)g_creationPointActReg.ResolveEntry(id))->m_fn =
        &CGruntCreationPoint::AdvanceAnim;
}

// CGruntCreationPoint::AdvanceAnim @0x03ecc0 - re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and
// return 0. Same archetype as CSimpleAnimation::AdvanceAnim (0x0abf70).
RVA(0x0003ecc0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    return 0;
}

SIZE_UNKNOWN(CAnimSink);
SIZE_UNKNOWN(CCreationPointActEntry);
SIZE_UNKNOWN(CCreationPointActReg);
SIZE_UNKNOWN(CCreationSpriteRefTable);
SIZE_UNKNOWN(CreationGameReg);
SIZE_UNKNOWN(CreationRefSlot);
