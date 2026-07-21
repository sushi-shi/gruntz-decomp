#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
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
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor (m_38+0x1a0 sink; Advance)
#include <Wap32/ZVec.h>


DATA(0x00245ad0)
extern CActReg g_lightFxActReg; // 0x645ad0

RVA(0x0009d140, 0x15)
void CLightFx::InitActReg() {
    g_lightFxActReg.Construct(2000, 2010);
}

RVA(0x0009d1c0, 0x102)
void CLightFx::FireActivation(i32 id) {
    CLightFxActEntry* e = reinterpret_cast<CLightFxActEntry*>(g_lightFxActReg.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this->*(reinterpret_cast<CLightFxActEntry*>(g_lightFxActReg.ResolveEntry(id)))->m_fn)();
    }
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
    (reinterpret_cast<CLightFxActEntry*>(g_lightFxActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CLightFx::AdvanceAnim);
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
    CObject* nodeOb = 0;
    // spec lookup -> CMapStringToOb::Lookup (0x1b8008); out is CObject*& (reinterpret node).
    // The spec source is the worker's owner context (AnimWorkerObj::m_0c @+0xc).
    m_3c->m_0c->m_imageRegistry->m_10map.Lookup(reinterpret_cast<const char*>(spec), nodeOb);
    node = reinterpret_cast<i32>(nodeOb);
    i32 found = node;
    g_gameReg->m_logicPump->Push(reinterpret_cast<CImageSet*>(found), anchorA, 7);
    if (found != 0) {
        // The spec lookup result IS a CImageSet (it is pushed to the pump as one);
        // read the lowest-indexed frame in its [m_minIndex, m_maxIndex] range.
        CImageSet* en = reinterpret_cast<CImageSet*>(found);
        i32 key = en->m_minIndex;
        // m_194/m_layer(+0x198) are CGameObject's role-union fields (source-def /
        // z-clamp descriptor); LightFx overwrites them with the resolved set/frame.
        m_38->m_194 = reinterpret_cast<char*>(found);
        i32 val;
        if (key < en->m_minIndex || key > en->m_maxIndex) {
            val = 0;
        } else {
            val = reinterpret_cast<i32>(static_cast<CImage*>(en->m_items.GetAt(key)));
        }
        m_38->m_layer = reinterpret_cast<CImage*>(val);
        m_38->m_190 = key;
    }
    node = 0;
    m_38->m_flags |= 2;
    m_anchorA = anchorA;
    m_anchorB = anchorB;
    // effect lookup -> CMapStringToPtr::Lookup (0x1b8438) via the object's owner
    // context (CGameObject::m_0c @+0xc); out is void*&.
    m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(reinterpret_cast<const char*>(effect), reinterpret_cast<void*&>(node));
    if (node != 0) {
        node = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(reinterpret_cast<const char*>(effect), reinterpret_cast<void*&>(node));
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(reinterpret_cast<CAniElement*>(node));
        RebindNode();
    }
    return 0;
}

RVA(0x0009d660, 0xc8)
i32 CLightFx::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, a3, a4) == 0) {
        return 0;
    }
    if (Chain(static_cast<CSerialArchive*>(ar), mode, a3, reinterpret_cast<CGameObject*>(a4)) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            (static_cast<CSerialArchive*>(ar))->Write(&m_anchorA, 4);
            (static_cast<CSerialArchive*>(ar))->Write(&m_anchorB, 4);
            break;
        case 7:
            (static_cast<CSerialArchive*>(ar))->Read(&m_anchorA, 4);
            (static_cast<CSerialArchive*>(ar))->Read(&m_anchorB, 4);
            break;
        case 8:
            g_gameReg->m_logicPump->Push(reinterpret_cast<CImageSet*>(m_38->m_194), m_anchorA, 7);
            break;
    }
    return 1;
}

RVA(0x0009d770, 0x25)
i32 CLightFx::RebindNode() {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    return 0;
}

RVA(0x0009d7b0, 0x40)
i32 CLightFx::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_28 && !m_38->m_1a0.m_20 && m_anchorB) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// CLightFx::~CLightFx (0x12430) - the /GX leaf dtor. CLightFx adds no destructible
// members beyond CUserLogic and shares its vtable, so the most-derived vptr store
// is dead-eliminated and the dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr
// call 0x16d2a0), store the CUserBase vptr (0x5e70b4). Byte-identical in shape to
// the established leaf-dtor archetype.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CLightFx() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CLightFx@@UAE@XZ 0x00012430 0x44

RVA(0x0009cdc0, 0xf1)
i32 LightFxLogicDispatch(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    switch (static_cast<u32>(reinterpret_cast<size_t>(aux->m_1c))) {
        case 0:
            aux->m_1c = reinterpret_cast<void*>(0x3e8);
            {
                CLightFx* p = new CLightFx(obj);
                (static_cast<CUserLogic*>(p))->Activate();
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
            ProjTypeXfer(aux->m_logic);
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
CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_anchorA = 2;
    m_anchorB = 1;
}
