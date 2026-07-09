// BoundaryLowerDtors.cpp - /GX destructors recovered from the engine_boundary
// backlog (lower half, RVA < 0x133370). Each is a real C++ destructor whose
// destructible base/member subobjects force the synchronous-EH (/GX) frame the
// retail compiler emits (push -1 / push handler / fs:0 chain + state writes). The
// owning class names are placeholders (declared in <Gruntz/BoundaryLowerDtorsViews.h>);
// only OFFSETS + code bytes are load-bearing. Vtable stamps + member-dtor callees
// reloc-mask. (See RezBufferObjectDtor.cpp.)
//
// ATTRIBUTION STATUS (matcher-1 re-home pass): all 6 are @orphan - genuinely
// unrecovered placeholder identities. Each is reached ONLY via an ILT jmp thunk from a
// `scalar_deleting_destructor' whose own class is also unrecovered; RTTI cannot
// attribute the COMDAT-folded body, so there is no named class TU to home them into.
// Evidence gathered (for a later identity-recovery pass):
//   CWorker39f20 (0x39f20)   <- ??_G @0x3a1a0 + CGruntzMgr::ChangeState_8fab0 (a state
//                               object ChangeState tears down; ~CRezBufferObject shape).
//   CCredits390a0 (0x390a0)  <- CGruntzMgr::ChangeState_8fab0 (a page/credits STORE that
//                               owns an MFC CFile+CByteArray; NOT CCreditsState, whose
//                               real dtor is 0x8d5e0).
//   CMenuState8d000 (0x8d000) <- ??_G @0x8cfd0; a CState state whose dtor runs
//                               Reset(0xf9840) then chains BaseCleanup(0xfa150). NOT the
//                               real CMenuState (whose ??1 is 0x8ce60) - a different,
//                               unrecovered state class.
//   CHolder8c400 (0x8c400)   <- ??_G @0x8c3d0 (owns an MFC CImageList).
//   CButeTree21a (0x21310)   <- ??_G @0x212e0; MI: base ~CContainerErr (0x16da60, vtable
//   CButeTree21b (0x21570)   <- ??_G @0x174d30; 0x1e94ac) + a zPTree base; body calls
//                               CButeStore::ClearRecursive (0x16e070). Two distinct
//                               derived classes sharing the base vtables - names unrecovered.
#include <Ints.h>
#include <Gruntz/GameModeBase.h>
#include <Gruntz/BoundaryUpper2Views.h>
#include <rva.h>
#include <Gruntz/BoundaryLowerDtorsViews.h> // placeholder /GX dtor classes

// DeleteImageList @0x1c6a5c IS MFC CImageList::DeleteImageList (afxcmn); minimal local decl.
SIZE_UNKNOWN(CImageList);
class CImageList {
public:
    void DeleteImageList();
};

// The 0x16e070 leaf is CButeStore::ClearRecursive (header-less butestoreclear class); local decl
// (the CButeStoreNode* arg is load-bearing for the mangled name).
struct CButeStoreNode;
class CButeStore {
public:
    void ClearRecursive(CButeStoreNode* node);
};

// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p); // 0x1b9b82

// ===========================================================================
// 0x039f20 - ~CWorker39f20 (/GX): stamp the derived vtable (0x5e971c), RezFree the
// +0x04 heap buffer, then fold the CObject base subobject (restamp the base dtor
// vtable 0x5e8cb4). Byte-shape identical to ~CRezBufferObject. __thiscall.
// ===========================================================================
RVA(0x00039f20, 0x51)
CWorker39f20::~CWorker39f20() {
    if (m_4) {
        RezFree(m_4);
    }
}

// ===========================================================================
// 0x08c400 - ~CHolder8c400 (/GX): stamp the derived vtable (0x5e8cd4), run the
// +0x00 teardown (0x1c6a5c), then fold the base subobject (restamp 0x5e8cb4).
// __thiscall.
// ===========================================================================
RVA(0x0008c400, 0x46)
CHolder8c400::~CHolder8c400() {
    ((CImageList*)this)->DeleteImageList();
}

// ===========================================================================
// 0x0390a0 - ~CCredits390a0 (/GX): run the explicit cleanup (0x17b570), then let
// the two owned members at +0x138 (dtor 0x1b4b76) and +0x124 (dtor 0x1bf121) fold
// in reverse declaration order. __thiscall.
// ===========================================================================
RVA(0x000390a0, 0x5d)
CCredits390a0::~CCredits390a0() {
    ((CPageStore17b510*)this)->Close();
}

// ===========================================================================
// 0x08d000 - ~CMenuState8d000 (/GX): stamp the derived vtable (0x5e9d74), run the
// teardown body (0x2919), then fold the CState base subobject (restamp 0x5ea21c +
// base dtor 0x3f53). __thiscall.
// ===========================================================================
// @early-stop
// EH-dtor base-stamp idiom wall (95.2%): retail stamps the CState base vtable in
// the DERIVED dtor (mov [esi],0x5ea21c) just before the base cleanup call (0x3f53,
// a cleanup that assumes the vtable already stamped); the polymorphic-base model
// stamps it inside the base dtor instead, so our derived omits that one 6-byte
// store. Derived-vtable stamp, Teardown body, /GX frame + states all byte-faithful.
RVA(0x0008d000, 0x55)
CMenuState8d000::~CMenuState8d000() {
    ((CGameModeBase*)this)->Reset();
}

// ===========================================================================
// 0x021310 / 0x021570 - ~CButeTree (/GX, multiple inheritance): stamp both base
// vtables (0x5e94ac @+0, 0x5e949c @+8), run the body teardown (0x16e070), then fold
// the +0x08 second base (dtor 0x16dfc0, MI this-adjust null guard) and the +0x00
// first base (dtor 0x16da60). Two distinct derived classes share the base vtables.
// __thiscall.
// ===========================================================================
RVA(0x00021310, 0x70)
CButeTree21a::~CButeTree21a() {
    ((CButeStore*)this)->ClearRecursive(0);
}
RVA(0x00021570, 0x70)
CButeTree21b::~CButeTree21b() {
    ((CButeStore*)this)->ClearRecursive(0);
}
