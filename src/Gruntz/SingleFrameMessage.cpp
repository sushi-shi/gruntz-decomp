// SingleFrameMessage.cpp - a single-frame message eyecandy game-object
// (C:\Proj\Gruntz). One trace-discovered method:
//   RegisterActs @0x0ab710 - bind the per-frame handler to the "A" key.
//
// CSingleFrameMessage : CUserLogic. Only offsets / code bytes are load-bearing.
#include <Mfc.h> // RECT / CopyRect (the ctor centers the object in a bounds rect)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/WwdGameReg.h>   // g_gameReg->GetMessageBounds (on-screen message bounds)
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// The game registry singleton (canonical <Gruntz/WwdGameReg.h>); the ctor asks it
// for the on-screen message-bounds RECT. Declared extern only (wwdfile owns 0x24556c).
extern "C" WwdGameReg* g_gameReg;

// CSingleFrameMessage::Serialize @0x00f5a0 - the vtable slot-1 override: chain the
// shared CUserLogic serialize helper on `this`, and (only on success) the +0x34
// sub-object's chain, both over the same (ar, tag, c, d) tuple; normalize the second
// chain's result to a bool. Byte-identical to CEyeCandy::Serialize (0x00fcc0).
RVA(0x0000f5a0, 0x47)
i32 CSingleFrameMessage::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CSingleFrameMessage::~CSingleFrameMessage @0x0f640 - empty vtable-anchor dtor;
// folds the CUserLogic teardown (the /GX leaf-dtor archetype).
RVA(0x0000f640, 0x44)
CSingleFrameMessage::~CSingleFrameMessage() {}

// --- CSingleFrameMessage (0x0ab310), vptr 0x5e864c --- the ctor anchors the
// ??_7CSingleFrameMessage vtable in this TU. Folds the inline CUserLogic(obj) base,
// applies the message sprite, then centers the object in g_gameReg's message bounds.
RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_object->ApplyLookupSprite("GAME_MESSAGEZ", m_38->m_04);
    RECT bounds;
    RECT r;
    CopyRect(&r, g_gameReg->GetMessageBounds(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CSingleFrameMessage::*SingleFrameHandler)();
struct CSingleFrameActEntry {
    SingleFrameHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645ef0), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). Was a per-file
// duplicate of the <Gruntz/ActReg.h> CActReg archetype (layout + ResolveEntry); now
// derives from it, keeping its own placeholder name so the DATA-pinned global is
// unchanged.
struct CSingleFrameActReg : public CActReg {};
DATA(0x00245ef0)
CSingleFrameActReg g_singleFrameActReg; // 0x645ef0

// CSingleFrameMessage::InitActReg @0x0ab530 - construct the class's activation-
// coordinate registry singleton (g_singleFrameActReg @0x645ef0) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ab530, 0x15)
void CSingleFrameMessage::InitActReg() {
    ((CZDArrayDerived*)&g_singleFrameActReg)->Construct(2000, 2010);
}

// CSingleFrameMessage::RunAct @0x0ab5b0 - resolve the registry entry for id; if a
// handler is bound, re-resolve and invoke it as a PMF on this, else return the
// entry pointer. Same archetype as CAniCycle::RunAct.
RVA(0x000ab5b0, 0x102)
i32 CSingleFrameMessage::RunAct(i32 id) {
    CSingleFrameActEntry* e = (CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
}

// CSingleFrameMessage::RegisterActs @0x0ab710 - bind the class's per-frame handler
// (AdvanceAnim @0x0ab910) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ab710, 0x18d)
void CSingleFrameMessage::RegisterActs() {
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
    ((CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id))->m_fn =
        &CSingleFrameMessage::AdvanceAnim;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
SIZE_UNKNOWN(CSingleFrameActEntry);
SIZE_UNKNOWN(CSingleFrameActReg);
