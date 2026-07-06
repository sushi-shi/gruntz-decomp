#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <DDrawMgr/DDrawBlitParam.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/LightFx.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h>           // CLightFxMgr (g_gameReg->m_logicPump @+0x78; Push)
#include <Gruntz/LogicTypeTableInline.h> // unrolled built-in logic-type registration
// LightFx.cpp - the "LightFx" tile-logic game object (C:\Proj\Gruntz), a
// CUserLogic leaf (ctor 0x9cf00; RTTI .?AVCUserLogic@@). Two leaf methods are
// reconstructed here, in ascending retail-RVA order:
//   * Activate  (0x9d520) - bind the effect spec from the object bute map.
//   * RebindNode(0x9d770) - re-point the object bute map at the "A" section.
//
// The spine (CUserLogic base, m_14/m_prevAnimSetNode/m_38/m_3c) is in <Gruntz/UserLogic.h>;
// CLightFx adds the +0x40/+0x54/+0x58 leaf words. Engine callees (the bute-map
// Lookup, the per-frame logic-table push, the layer SetNode) are modeled NO-body
// so their call displacements reloc-mask. Field names are placeholders; only the
// OFFSETS + code bytes are load-bearing.

// ---------------------------------------------------------------------------
// The CMap core embedded at +0x10 inside each spec/effect store. Lookup(key,
// &out) fills *out with the mapped node and returns nonzero on hit. __thiscall,
// ret 8 (2 args), matched in the engine map TU - NO-body so the call reloc-masks.
struct LfxMapCore {
    i32 Lookup(i32 key, i32* out); // 0x1b8008 / 0x1b8438
};

// A spec/effect store: the CMap core sits at +0x10 (the engine call is
// `mov ecx,store; add ecx,0x10; Lookup`).
struct LfxNodeMap {
    char m_pad00[0x10];
    LfxMapCore m_10; // +0x10  the CMap core
};

// The two store objects the lookup chains walk to. The bound object's holder
// (m_3c->m_0c / m_38->m_0c) points at a record whose +0x10 holds the spec store
// and +0x2c the effect store.
struct LfxMapHolder {
    char m_pad00[0x10];
    LfxNodeMap* m_10; // +0x10  spec store
    char m_pad14[0x2c - 0x14];
    LfxNodeMap* m_2c; // +0x2c  effect store
};
struct LfxMapSource {
    char m_pad00[0xc];
    LfxMapHolder* m_0c; // +0xc
};

// The global game registry (?g_gameReg, *0x64556c); only the +0x78 logic pump is
// read here. (Declared as the engine's CGameRegistry via the existing DATA label.)
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The global bute store RebindNode re-points the object map at ("A" section).
// g_buteTree (0x6bf620) + CButeTree::Find (0x16d190) live in the bute TUs;
// declared extern only so `g_buteTree.Find("A")` reloc-masks (CButeTree owns the
// DATA label).
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The bound object's effect node (the Lookup result): +0x14 a value table indexed
// by the clamped key, +0x64/+0x68 the key bounds.
struct LfxEffectNode {
    char m_pad00[0x14];
    i32* m_14; // +0x14  value table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  key lo
    i32 m_68; // +0x68  key hi
};

// The bound object's layer sub-descriptor (m_38+0x1a0): SetNode caches the
// resolved effect node. __thiscall ret 4 (1 arg), 0x15c2d0; modeled NO-body.

// The bound object proper (m_38): +0x08 flags, +0x0c the map holder, +0x190..0x198
// the resolved-node triple, +0x1a0 the layer sub-descriptor, +0x1b4 the layer base.
// Kept as a per-TU concrete view (NOT folded into CGameObject): +0x198 here is an
// i32 resolved-node value, conflicting with CGameObject's CGameObjLayer* z-clamp
// descriptor at the same offset; +0x1a0 is the class-specific LfxLayerSink.
struct LfxObj {
    char m_pad00[0x8];
    i32 m_08;           // +0x08  flag word (|= 2)
    LfxMapHolder* m_0c; // +0x0c
    char m_pad10[0x190 - 0x10];
    i32 m_190; // +0x190 resolved key
    i32 m_194; // +0x194 resolved node
    i32 m_198; // +0x198 resolved value
    char m_pad19c[0x1a0 - 0x19c];
    i32 m_1a0; // +0x1a0 CDDrawBlitParam sub-descriptor (Setup_15c2d0)
    char m_pad1a4[0x1b4 - 0x1a4];
    i32 m_1b4; // +0x1b4 layer base
};

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CLightFx::*LightFxHandler)();
struct CLightFxActEntry {
    LightFxHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645ad0): the fixed
// [2000,2010] range built by the shared registry ctor (0x408710). CLightFxActReg is
// the shared <Gruntz/ActReg.h> CActReg archetype (was a per-file duplicate of its
// layout + ResolveEntry); it keeps its own placeholder name so the DATA-pinned
// global symbol is unchanged.
struct CLightFxActReg : public CActReg {};
DATA(0x00245ad0)
extern CLightFxActReg g_lightFxActReg; // 0x645ad0

// CLightFx::InitActReg @0x9d140 - construct the class's activation-coordinate
// registry singleton (g_lightFxActReg @0x645ad0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0009d140, 0x15)
void CLightFx::InitActReg() {
    g_lightFxActReg.Construct(2000, 2010);
}

// CLightFx::RegisterActs @0x9d320 - bind the per-frame handler (AdvanceAnim
// @0x9d7b0) to the activation key "A" via the shared name registry. The SAME
// archetype as CGruntCreationPoint::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0009d320, 0x18d)
void CLightFx::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CLightFxActEntry*)g_lightFxActReg.ResolveEntry(id))->m_fn = &CLightFx::AdvanceAnim;
}

// ===========================================================================
// CLightFx::Activate  (0x9d520)  - look the effect spec up in the bound object's
// spec map, run the per-frame logic push, prime the bound object's resolved-node
// triple from the effect node, latch the (anchorA, anchorB) pair, look the effect
// up in the effect map (twice), feed it to the layer descriptor, and rebind.
// ===========================================================================
// @early-stop
// 86.7% - scheduling/stack-slot wall (regalloc family, see
// docs/patterns/pin-local-for-callee-saved-reg.md + reread-member-view-pointer.md):
// the body is byte-identical in shape - promoting the spec-lookup result into the
// named local `found` already pins it callee-saved (edi) like retail (70%->86.7%),
// and re-reading m_38 per-region matches retail's reload pattern. The residual is
// (a) the `node` out-param stack slot lands at [esp+0xc] vs retail's [esp+0x14]
// (an 8-byte frame-layout pick that renames every slot ref) and (b) two 2-3 instr
// schedule swaps (the node=0 zero relative to the arg pushes; the m_54/m_58 stores
// vs the m_38 reload before the effect lookup). Logic 100% correct.
RVA(0x0009d520, 0xfd)
i32 CLightFx::Activate(i32 spec, i32 anchorA, i32 effect, i32 anchorB) {
    i32 node = 0;
    ((LfxMapSource*)m_3c)->m_0c->m_10->m_10.Lookup(spec, &node);
    i32 found = node;
    g_gameReg->m_logicPump->Push((CImageSet*)found, anchorA, 7);
    LfxObj* obj = (LfxObj*)m_38;
    if (found != 0) {
        LfxEffectNode* en = (LfxEffectNode*)found;
        i32 key = en->m_64;
        obj->m_194 = found;
        i32 val;
        if (key < en->m_64 || key > en->m_68) {
            val = 0;
        } else {
            val = en->m_14[key];
        }
        obj->m_198 = val;
        obj->m_190 = key;
    }
    node = 0;
    ((LfxObj*)m_38)->m_08 |= 2;
    m_anchorA = anchorA;
    m_anchorB = anchorB;
    ((LfxObj*)m_38)->m_0c->m_2c->m_10.Lookup(effect, &node);
    if (node != 0) {
        node = 0;
        ((LfxObj*)m_38)->m_0c->m_2c->m_10.Lookup(effect, &node);
        m_layerBase = ((LfxObj*)m_38)->m_1b4;
        ((CDDrawBlitParam*)&((LfxObj*)m_38)->m_1a0)->Setup_15c2d0((CDDrawBlitParamSrc*)node);
        RebindNode();
    }
    return 0;
}

// ===========================================================================
// CLightFx::RebindNode  (0x9d770)  - save the object map's current bute node into
// m_prevAnimSetNode, re-point it at the "A" section of g_buteTree, return 0.
// ===========================================================================
RVA(0x0009d770, 0x25)
i32 CLightFx::RebindNode() {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CLightFxActEntry);
SIZE_UNKNOWN(CLightFxActReg);
SIZE_UNKNOWN(LfxEffectNode);
SIZE_UNKNOWN(LfxLayerSink);
SIZE_UNKNOWN(LfxMapCore);
SIZE_UNKNOWN(LfxMapHolder);
SIZE_UNKNOWN(LfxMapSource);
SIZE_UNKNOWN(LfxNodeMap);
SIZE_UNKNOWN(LfxObj);

// ============================================================================
// merged from LightFxEh.cpp (the /GX EH-frame sibling; unit flags -> eh)
// ============================================================================
// LightFxEh.cpp - the /GX EH-framed CLightFx method(s), split off the frameless
// clightfx TU (C:\Proj\Gruntz). MSVC5's /GX frames the leaf dtor (its destructible
// +0x18 CUserLogic link forces the EH frame), so it cannot share the base TU's
// frameless flags without re-framing its 100% leaves. The split is matching-neutral
// (each function is RVA-keyed); see split-tu-eh-dtor-vs-frameless-cstring.md.

// CLightFx::~CLightFx (0x12430) - the /GX leaf dtor. CLightFx adds no destructible
// members beyond CUserLogic and shares its vtable, so the most-derived vptr store
// is dead-eliminated and the dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr
// call 0x16d2a0), store the CUserBase vptr (0x5e70b4). Byte-identical in shape to
// the established leaf-dtor archetype.
RVA(0x00012430, 0x44)
CLightFx::~CLightFx() {}

// CLightFx::CLightFx (0x9cf00) - the /GX EH-framed ctor (the EngStr temp the shared
// CUserLogic(obj) prologue builds forces the frame). This TU inlines the built-in
// logic-type registration (the "unrolled" prologue); the per-class tail seeds the
// leaf anchors (m_anchorA = 2, m_anchorB = 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0009cf00, 0x1a5)
CLightFx::CLightFx(CGameObject* obj) : CTileLogic(obj) {
    m_anchorA = 2;
    m_anchorB = 1;
}
