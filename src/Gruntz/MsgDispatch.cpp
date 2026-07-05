// MsgDispatch.cpp - three near-identical command dispatchers (orphan COMDATs
// @0xaa0a0 / 0xaa1e0 / 0xaa460) that route a UI message (arg->m_7c->m_1c) to the
// handler sub-object (m_18): command 0 lazily constructs the handler (new CObj(arg)
// + slot-6 init, /GX EH-framed), the mid commands fan out to its vtable slots, and
// the rest serialize through the shared default. The three differ only in their
// constructed handler class (reloc-masked ctor) + EH funclet, so one body matches
// all three. Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>
#include <Gruntz/UserLogic.h> // CTileLogic base + CGameObject / CGameObjAux (+0x7c record)

// The handler object (m_18): a 0x54-byte CTileLogic (CUserLogic) logic leaf (the SAME
// archetype as CToobSpikez - phase codes 0/0x1d/0x1e/0x50-0x53 select the inherited
// CUserLogic virtual slots 6/10..15, and the default falls through to the shared
// type-xfer serializer 0x16e4f0). Three dispatchers construct three different such
// leaves; the specific leaf identity is unrecovered (1-of-3), so the class is a
// structural CTileLogic leaf. Real polymorphic dispatch (no PMF view); the external
// ctor (reloc-masked) stamps the real vtable.
class CObj : public CTileLogic {
public:
    CObj(CGameObject* arg);    // external, reloc-masked
    char m_pad40[0x54 - 0x40]; // to the true 0x54 leaf size (CTileLogic is 0x40)
    //   phase-0 init   = Activate        (slot 6,  +0x18)
    //   phase 0x1e     = UserLogicVfunc8 (slot 10, +0x28)
    //   phase 0x1d     = UserLogicVfunc9 (slot 11, +0x2c)
    //   phase 0x52     = UserLogicVfuncA (slot 12, +0x30)
    //   phase 0x51     = UserLogicVfuncB (slot 13, +0x34)
    //   phase 0x50     = UserLogicVfuncC (slot 14, +0x38)
    //   phase 0x53     = UserLogicVfuncD (slot 15, +0x3c)
};

// The record is the game object's +0x7c sub-object (canonical CGameObjAux): its
// generic m_18 holds the live logic leaf (a CObj here - a proven-heterogeneous slot,
// so the leaf downcasts at the call sites) and its generic m_1c carries the phase
// code (unsigned switch: 0 build, 0x1d/0x1e/0x50-0x53 dispatch, 0x3e8 idle).

// The default case runs the type-keyed record transfer/dispatch: NOT a bespoke
// "dispatch default" - it is the shared CTypeKeyColl serializer ProjTypeXfer
// (0x16e4f0, owned + matched in TypeKeyColl.cpp), which resolves the handler's
// type-registry entry (ar->m_14->m_1c) and xfers it through the handler's own
// vtable slots. The handler (m_18) is passed as the archive-record arg.
struct CXferArchive;                // TypeKeyColl.cpp-local archive-record view
i32 ProjTypeXfer(CXferArchive* ar); // 0x16e4f0

// The dispatcher body, identical across the three instantiations (the only
// retail difference is the reloc-masked ctor + EH funclet).
#define DISPATCH_BODY                                                                              \
    CGameObjAux* rec = arg->m_7c;                                                                  \
    switch ((u32)rec->m_1c) {                                                                      \
        case 0: {                                                                                  \
            rec->m_1c = (void*)0x3e8;                                                              \
            CObj* o = new CObj(arg);                                                               \
            o->Activate();                                                                         \
            rec->m_18 = o;                                                                         \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            ((CObj*)rec->m_18)->UserLogicVfunc9();                                                 \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            ((CObj*)rec->m_18)->UserLogicVfunc8();                                                 \
            break;                                                                                 \
        case 0x50:                                                                                 \
            ((CObj*)rec->m_18)->UserLogicVfuncC();                                                 \
            break;                                                                                 \
        case 0x51:                                                                                 \
            ((CObj*)rec->m_18)->UserLogicVfuncB();                                                 \
            break;                                                                                 \
        case 0x52:                                                                                 \
            ((CObj*)rec->m_18)->UserLogicVfuncA();                                                 \
            break;                                                                                 \
        case 0x53:                                                                                 \
            ((CObj*)rec->m_18)->UserLogicVfuncD();                                                 \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            ProjTypeXfer((CXferArchive*)rec->m_18);                                                \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x000aa0a0, 0xf1)
i32 Dispatch_aa0a0(CGameObject* arg){DISPATCH_BODY}

RVA(0x000aa1e0, 0xf1)
i32 Dispatch_aa1e0(CGameObject* arg){DISPATCH_BODY}

RVA(0x000aa460, 0xf1)
i32 Dispatch_aa460(CGameObject* arg){DISPATCH_BODY}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CObj);
