#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/AniAdvanceCursor.h> // (ex DDrawBlitParam - folded onto CAniAdvanceCursor)
#include <Gruntz/ActReg.h>           // the shared CActReg coordinate-registry archetype
#include <Gruntz/LightFx.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h>        // CLightFxMgr (g_gameReg->m_logicPump @+0x78; Push)
#include <Image/ImageSet.h>           // CImageSet - the spec Lookup result (frames + index range)
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (spec/effect stores)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (the spec store; Ob 0x1b8008)
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // m_animRegistry (the effect store; Ptr 0x1b8438)
#include <Gruntz/LogicTypeTableInline.h>  // unrolled built-in logic-type registration
#include <Gruntz/SerialArchive.h>    // CSerialArchive Read(+0x2c)/Write(+0x30) for SerializeMove
#include <Gruntz/SerialObjRef.h>     // the +0x34 serialized-object-reference (Chain @0x8c00)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor (m_38+0x1a0 sink; Advance)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// The per-frame draw-delta mirror (_g_6bf3bc @0x6bf3bc) AdvanceAnim hands the anim sink.
extern "C" u32 g_engineFrameDelta;
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

// The spec/effect resource stores are the typed m_0c world root's m_imageRegistry
// (CDDrawWorkerRegistry, Ob band 0x1b8008) and m_animRegistry (CDDrawSubMgrLeaf, Ptr band
// 0x1b8438); the former LfxSpecStore/LfxEffectStore/LfxMapHolder views (included
// above) - the two store maps are DIFFERENT real MFC types (CMapStringToOb for the
// spec @0x1b8008, CMapStringToPtr for the effect @0x1b8438: proven by disasm of
// CLightFx::Activate @0x9d520; the labels used to be inverted here).

// The global game registry (?g_gameReg, *0x64556c); only the +0x78 logic pump is
// read here. (Declared as the engine's CGameRegistry via the existing DATA label.)
extern "C" CGameRegistry* g_gameReg;

// The global bute store RebindNode re-points the object map at ("A" section).
// g_buteTree (0x6bf620) + CButeTree::Find (0x16d190) live in the bute TUs;
// declared extern only so `g_buteTree.Find("A")` reloc-masks (CButeTree owns the
// DATA label).

// CImageSet (<Image/ImageSet.h>): its "value table @+0x14" is CImageSet::m_frames,
// its "key bounds @+0x64/+0x68" are m_minIndex/m_maxIndex - the spec result IS a
// CImageSet (pushed to the pump as one), read for the frame in its index range.)

// The bound object (m_38) IS the real CGameObject (<Gruntz/UserLogic.h>): the ex
// (the owner context), +0x190/m_194/m_layer(+0x198) are CGameObject's role-union
// resolved-node fields (source-def/z-clamp descriptor, reinterpreted here per the
// canonical's own note), +0x1a0 the per-leaf anim sub-object (CAniAdvanceCursor,
// reached by address), +0x1b4 the m_geoId geometry descriptor. The alleged +0x198
// "conflict" was the documented role-union, not a distinct field.

// CLightFxActEntry (the registry entry's per-frame handler PMF) is declared with
// the class in <Gruntz/LightFx.h> (included above).

// The class's activation-coordinate registry singleton (@0x645ad0): the fixed
// [2000,2010] range built by the shared registry ctor (0x408710). It IS the shared
// <Gruntz/ActReg.h> CActReg archetype - the empty CLightFxActReg subclass (whose
// the mangled symbol unique and DATA() rebinds it, so the archetype IS the type
// (same fold TileLogicPump's ActReg globals use).
DATA(0x00245ad0)
CActReg g_lightFxActReg; // 0x645ad0

// CLightFx::InitActReg @0x9d140 - construct the class's activation-coordinate
// registry singleton (g_lightFxActReg @0x645ad0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0009d140, 0x15)
void CLightFx::InitActReg() {
    ((CZDArrayDerived*)&g_lightFxActReg)->Construct(2000, 2010);
}

// CLightFx::RunAct @0x9d1c0 - the slot-4 (UserLogicVfunc2) impl: resolve the class
// coordinate-registry entry for id; if a handler PMF is bound there, re-resolve the
// entry and dispatch the PMF on this, else return the entry pointer. Two inline
// CActReg::ResolveEntry expansions (side-effecting -> cl cannot CSE across the guard).
// The SAME archetype as CEyeCandyAni::RunAct (0x0acbb0).
RVA(0x0009d1c0, 0x102)
i32 CLightFx::RunAct(i32 id) {
    CLightFxActEntry* e = (CLightFxActEntry*)g_lightFxActReg.ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CLightFxActEntry*)g_lightFxActReg.ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
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
    // spec lookup -> CMapStringToOb::Lookup (0x1b8008); out is CObject*& (reinterpret node).
    // The spec source is the worker's owner context (AnimWorkerObj::m_0c @+0xc).
    m_3c->m_0c->m_imageRegistry->m_10map.Lookup((const char*)spec, (CObject*&)node);
    i32 found = node;
    g_gameReg->m_logicPump->Push((CImageSet*)found, anchorA, 7);
    if (found != 0) {
        // The spec lookup result IS a CImageSet (it is pushed to the pump as one);
        // read the lowest-indexed frame in its [m_minIndex, m_maxIndex] range.
        CImageSet* en = (CImageSet*)found;
        i32 key = en->m_minIndex;
        // m_194/m_layer(+0x198) are CGameObject's role-union fields (source-def /
        // z-clamp descriptor); LightFx overwrites them with the resolved set/frame.
        m_38->m_194 = (char*)found;
        i32 val;
        if (key < en->m_minIndex || key > en->m_maxIndex) {
            val = 0;
        } else {
            val = (i32)en->m_frames[key];
        }
        m_38->m_layer = (CGameObjLayer*)val;
        m_38->m_190 = key;
    }
    node = 0;
    m_38->m_flags |= 2;
    m_anchorA = anchorA;
    m_anchorB = anchorB;
    // effect lookup -> CMapStringToPtr::Lookup (0x1b8438) via the object's owner
    // context (CGameObject::m_0c @+0xc); out is void*&.
    m_38->m_0c->m_animRegistry->m_10.Lookup((const char*)effect, (void*&)node);
    if (node != 0) {
        node = 0;
        m_38->m_0c->m_animRegistry->m_10.Lookup((const char*)effect, (void*&)node);
        m_layerBase = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0((CAniElement*)node);
        RebindNode();
    }
    return 0;
}

// ===========================================================================
// CLightFx::SerializeMove  (0x9d660, slot 1)  - chain the base CUserLogic serialize
// (bail 0 on failure) + the +0x34 serialized-object-reference (CSerialObjRef::Chain
// via the 0x1aff thunk, bail 0), then per mode: 4 = write, 7 = read the (anchorA,
// anchorB) pair; 8 = re-push the effect (m_38->m_194) into the logic pump. Return 1.
// ===========================================================================
RVA(0x0009d660, 0xc8)
i32 CLightFx::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, a3, a4) == 0) {
        return 0;
    }
    if (((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, mode, a3, (CGameObject*)a4) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            ((CSerialArchive*)ar)->Write(&m_anchorA, 4);
            ((CSerialArchive*)ar)->Write(&m_anchorB, 4);
            break;
        case 7:
            ((CSerialArchive*)ar)->Read(&m_anchorA, 4);
            ((CSerialArchive*)ar)->Read(&m_anchorB, 4);
            break;
        case 8:
            g_gameReg->m_logicPump->Push((CImageSet*)m_38->m_194, m_anchorA, 7);
            break;
    }
    return 1;
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

// ===========================================================================
// CLightFx::AdvanceAnim  (0x9d7b0)  - the per-frame handler PMF: advance the bound
// object's animation sink (m_38 + 0x1a0) to the current draw-delta (g_engineFrameDelta), then -
// if the sink is active (m_1c8) but idle (!m_1c0) and anchorB is latched - set the
// bound object's "advance" flag (m_flags |= 0x10000). Returns 0. Same sink archetype as
// CEyeCandyAni::AdvanceAnim.
// ===========================================================================
RVA(0x0009d7b0, 0x40)
i32 CLightFx::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_28 && !m_38->m_1a0.m_20 && m_anchorB) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// or homed to headers: LfxObj -> CGameObject, LfxMapSource -> AnimWorkerObj::m_0c,
// LfxEffectNode -> CImageSet, CLightFxActReg -> the CActReg archetype,
// the canonical CDDrawSurfaceMgr members. No struct/class is defined in this .cpp.

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

// The default-state fall-through helper IS the real shared coordinate/type-registry
// resolve at 0x16e4f0 (?ProjTypeXfer@@YAHPAUCXferArchive@@@Z, __cdecl); the record
// arg is the active logic leaf, reinterpreted as the archive record it drives.

// LightFxLogicDispatch @0x9cdc0 - the LightFx logic-type per-frame state dispatcher
// (__cdecl free fn bound into the class's logic dispatch table). Reaches the object's
// embedded logic record (obj->m_7c) and switches on its state tag (m_1c): state 0
// lazily constructs the CLightFx (new 0x5c, ctor 0x9cf00), activates it (slot 6), and
// latches state 0x3e8; each Op<NN> state dispatches the like-named CUserLogic slot on
// the bound logic; state 0x3e8 is the built/idle no-op. The /GX EH frame comes from the
// `new` (delete-on-unwind funclet). SAME archetype as LogicRecordDispatch.cpp's
// LogicDispatchA (0xf1, all four 100%); the built type here is the real CLightFx.
RVA(0x0009cdc0, 0xf1)
i32 LightFxLogicDispatch(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    switch ((u32)(size_t)aux->m_1c) {
        case 0:
            aux->m_1c = (void*)0x3e8;
            {
                CLightFx* p = new CLightFx(obj);
                ((CUserLogic*)p)->Activate();
                aux->m_logic = p;
            }
            break;
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)aux->m_logic);
            break;
    }
    return 1;
}

// CLightFx::CLightFx (0x9cf00) - the /GX EH-framed ctor (the EngStr temp the shared
// CUserLogic(obj) prologue builds forces the frame). This TU inlines the built-in
// logic-type registration (the "unrolled" prologue); the per-class tail seeds the
// leaf anchors (m_anchorA = 2, m_anchorB = 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0009cf00, 0x1a5)
CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_anchorA = 2;
    m_anchorB = 1;
}
