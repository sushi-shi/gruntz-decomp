// DiscoveredEh.cpp - trace-discovered /GX leaf destructors re-homed from
// src/Stub/Discovered.cpp (engine_discovered is base-profile; these dtors carry a
// C++ exception-handling frame, so they need the eh profile). Only OFFSETS + code
// bytes are load-bearing.
//
// Item-7 owner audit (sema xref):
//  - ~CU55 (0x16460): its only caller is its own deleting-dtor
//    (0x16430, vtable slot); RTTI name unrecovered (Ghidra ClassUnknown_55_016460).
//    It is a byte-identical twin of ~CImgHolder (0x16500, Dialogs.cpp) - a dialog
//    image-holder freeing an embedded CImageList - so it belongs to the dialog
//    cluster, but no real class name is recoverable; kept here with this note.
//  - ~CButeStore (0x174d70): the real class CButeStore has a TU (src/Bute/
//    ButeStoreClear.cpp, CButeStore::ClearRecursive). It is NOT folded there because
//    the two CButeStore models diverge: ButeStoreClear.cpp uses <Bute/ButeMgr.h>'s
//    FLAT CButeStore, while this dtor is multiple-inheritance-modeled (CContainerErr
//    @+0 + CObj50 @+8, two vptr re-stamps + two base dtors). Folding requires
//    reconciling the two models (extend ButeMgr.h's CButeStore to MI) - deferred to
//    a depth pass so it can't regress ButeStoreClear.cpp; kept here meanwhile.
#include <Ints.h>
#include <rva.h>

// DeleteImageList @0x1c6a5c IS MFC CImageList::DeleteImageList (afxcmn); minimal local decl.
SIZE_UNKNOWN(CImageList);
class CImageList {
public:
    void DeleteImageList();
};

// ---------------------------------------------------------------------------
// tomalla-55 @0x016460 - a byte-identical twin of ~CImgHolder (0x16500,
// Dialogs.cpp): a derived holder whose /GX dtor frees an embedded CImageList
// (DeleteImageList 0x1c6a5c) between the implicit own vptr stamp (0x5e8cd4) and the
// folded base re-stamp (0x5e8cb4). The empty inline base dtor folds as the LAST
// store; the non-trivial base earns the /GX frame. Real-virtual model
// (docs/patterns/eh-dtor-implicit-vptr-stamp-first.md sub-case 2); cl emits the
// ??_7 stamps, which reloc-mask.
struct CU55Base {
    virtual ~CU55Base() {}
};
struct CU55 : CU55Base {
    void DeleteImageList();   // 0x1c6a5c (NAFXCW CImageList::DeleteImageList)
    virtual ~CU55() OVERRIDE; // 0x016460
};
RVA(0x00016460, 0x46)
CU55::~CU55() {
    ((CImageList*)this)->DeleteImageList();
}

// ---------------------------------------------------------------------------
// BoomerangCmdDispatch @0x0de9e0 - a __cdecl /GX command dispatcher over the
// boomerang-launch state of a game object (re-homed from src/Stub/Discovered.cpp).
// arg = the launching object; arg->m_7c = its boomerang-launch command state
// (BoomState: m_18 = the active boomerang, m_1c = the command tag). Switches on the
// tag: tag 0 sets the tag to 0x3e8 and `new CBoomerang(arg)` (size 0x260, ctor
// 0xe0650), runs its slot-6 virtual, and stores it at m_18; tags 0x1d/0x1e/0x50..
// 0x53 dispatch other CUserLogic-inherited slots (11/10/14/13/12/15) on m_18; tag
// 0x3e8 is a no-op; the default hands m_18 to ProjTypeXfer (0x16e4f0). The heap
// `new` earns the /GX frame (the object is EH-tracked until stored at m_18).
// EXACT. The /GX new-expression cleanup funclet + __ehstate transitions match once
// the command tag is UNSIGNED (the switch range checks are `ja`, not `jg`).
// @identity-TODO: the dispatcher's own owner class is unrecovered (only inbound edge
// is ILT thunk 0x158c from an unrecovered fn); BoomState/BoomHost are placeholder
// views of the launcher's boomerang-launch sub-state (offsets load-bearing). The
// dispatched CBoomerang is the fully-modeled CBoomerang : CProjectile (its 5 vtable
// slots reloc-mask); the local CBoomerang below is a size-0x260 view of it (no
// shared CBoomerang header exists yet - Boomerang.cpp defines it TU-locally).
struct CGameObject;   // CBoomerang ctor arg (the launcher/owner)
struct CXferArchive;  // ProjTypeXfer arg
i32 ProjTypeXfer(CXferArchive* a); // 0x16e4f0 (default-case handler; reloc-masked)
// The active boomerang, dispatched polymorphically through its CUserLogic-family
// vtable (slots 6/10..15 used); m_18's concrete type is CBoomerang.
struct BoomObj {
    virtual void s0();  virtual void s1();  virtual void s2();  virtual void s3();
    virtual void s4();  virtual void s5();  virtual void s6();  virtual void s7();
    virtual void s8();  virtual void s9();  virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
};
struct CBoomerang : BoomObj {
    CBoomerang(CGameObject* owner); // 0xe0650 (declared-only; reloc-masked call + vptr stamp)
    char m_pad4[0x260 - 4];         // sizeof 0x260 (the retail operator-new byte count)
};
struct BoomState {
    char m_pad0[0x18];
    BoomObj* m_18; // +0x18 the active boomerang
    u32 m_1c;      // +0x1c command tag (unsigned -> the switch uses `ja`, not `jg`)
};
struct BoomHost {
    char m_pad0[0x7c];
    BoomState* m_7c; // +0x7c the boomerang-launch command state
};
RVA(0x000de9e0, 0xf4)
i32 BoomerangCmdDispatch_de9e0(void* arg) {
    BoomState* st = ((BoomHost*)arg)->m_7c;
    switch (st->m_1c) {
        case 0: {
            st->m_1c = 0x3e8;
            CBoomerang* b = new CBoomerang((CGameObject*)arg);
            b->s6();
            st->m_18 = b;
            break;
        }
        case 0x1d:
            st->m_18->s11();
            break;
        case 0x1e:
            st->m_18->s10();
            break;
        case 0x50:
            st->m_18->s14();
            break;
        case 0x51:
            st->m_18->s13();
            break;
        case 0x52:
            st->m_18->s12();
            break;
        case 0x53:
            st->m_18->s15();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)st->m_18);
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CButeStore @0x174d70 - a multiple-inheritance /GX dtor. The most-derived class
// re-stamps its primary vptr (+0, 0x5e94ac) and its secondary base vptr (+8,
// 0x5e949c) at entry, runs ClearRecursive(0) (0x16e070), then destructs its bases
// in reverse declaration order: the secondary base @+8 (~CObj50 0x16dfc0, with the
// `this ? this+8 : 0` adjust) then the primary base @0 (~CContainerErr 0x16da60).
// Real-polymorphic multiple-inheritance model so cl emits both implicit vptr stamps
// + both base-dtor CALLs (all reloc-mask). The non-trivial bases earn the /GX frame.
struct CContainerErr {
    virtual ~CContainerErr(); // 0x16da60 (external)
    char m_pad[8 - 4];        // vptr@+0; CObj50 begins at +8
};
struct CObj50 {
    virtual ~CObj50(); // 0x16dfc0 (external; stamps 0x5f04d8 internally)
};
struct CButeStore : CContainerErr, CObj50 {
    virtual ~CButeStore() OVERRIDE; // 0x174d70
    void ClearRecursive(struct CButeStoreNode*); // 0x16e070 (real takes CButeStoreNode*; 0 = null)
};
RVA(0x00174d70, 0x70)
CButeStore::~CButeStore() {
    ClearRecursive(0);
}

SIZE_UNKNOWN(BoomObj);
SIZE(CBoomerang, 0x260);
SIZE_UNKNOWN(BoomState);
SIZE_UNKNOWN(BoomHost);
SIZE_UNKNOWN(CU55Base);
SIZE_UNKNOWN(CU55);
RELOC_VTBL(CU55, 0x001e8cd4); // vtable reloc-masks a bound datum (dtor-stamp verified)
SIZE_UNKNOWN(CObj50);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
