// MsgDispatch.cpp - three near-identical command dispatchers (orphan COMDATs
// @0xaa0a0 / 0xaa1e0 / 0xaa460) that route a UI message (arg->m_7c->m_1c) to the
// handler sub-object (m_18): command 0 lazily constructs the handler (new CObj(arg)
// + slot-6 init, /GX EH-framed), the mid commands fan out to its vtable slots, and
// the rest serialize through the shared default. The three differ only in their
// constructed handler class (reloc-masked ctor) + EH funclet, so one body matches
// all three. Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>
#include <Gruntz/UserLogic.h> // CTileLogic base (handler is a CUserLogic leaf)

struct CMsgObj; // the bound game object (its +0x7c logic-control record is used here)

// The handler object (m_18): a 0x54-byte CTileLogic (CUserLogic) logic leaf (the SAME
// archetype as CToobSpikez - phase codes 0/0x1d/0x1e/0x50-0x53 select the inherited
// CUserLogic virtual slots 6/10..15, and the default falls through to the shared
// type-xfer serializer 0x16e4f0). Three dispatchers construct three different such
// leaves; the specific leaf identity is unrecovered (1-of-3), so the class is a
// structural CTileLogic leaf. Real polymorphic dispatch (no PMF view); the external
// ctor (reloc-masked) stamps the real vtable.
class CObj : public CTileLogic {
public:
    CObj(CMsgObj* arg);        // external, reloc-masked (the bound game object)
    char m_pad40[0x54 - 0x40]; // to the true 0x54 leaf size (CTileLogic is 0x40)
    //   phase-0 init   = Activate        (slot 6,  +0x18)
    //   phase 0x1e     = UserLogicVfunc8 (slot 10, +0x28)
    //   phase 0x1d     = UserLogicVfunc9 (slot 11, +0x2c)
    //   phase 0x52     = UserLogicVfuncA (slot 12, +0x30)
    //   phase 0x51     = UserLogicVfuncB (slot 13, +0x34)
    //   phase 0x50     = UserLogicVfuncC (slot 14, +0x38)
    //   phase 0x53     = UserLogicVfuncD (slot 15, +0x3c)
};

// The game object's +0x7c logic-control record (the CGameObjAux slot). Its m_18 and
// m_1c are genuinely-generic engine slots (m_18 holds a CProjectile / LightFx object /
// this CObj leaf across TUs; m_1c holds a phase int here but a bute-tree animset node
// pointer in StaticHazard) - so this logic subsystem types them for ITS use (a CObj
// leaf + an int phase), the cast-free per-use accessor the sibling logic pumps use
// (AnimWorkerHandlers). Not a fake view of one true shape - a typed facet of a real
// union slot.
struct CLogicCtrl {
    char _00[0x18];
    CObj* m_18; // +0x18  the live logic leaf
    u32 m_1c;   // +0x1c  phase/command id (unsigned switch)
};

// The default case runs the type-keyed record transfer/dispatch: NOT a bespoke
// "dispatch default" - it is the shared CTypeKeyColl serializer ProjTypeXfer
// (0x16e4f0, owned + matched in TypeKeyColl.cpp), which resolves the handler's
// type-registry entry and xfers it through the handler's own vtable slots. The
// handler (m_18) is passed as the record arg (declared to take CObj -> cast-free).
i32 ProjTypeXfer(CObj* rec); // 0x16e4f0

// The bound game object: only its +0x7c logic-control record is reached here.
struct CMsgObj {
    char _00[0x7c];
    CLogicCtrl* m_7c; // +0x7c
};

// The dispatcher body, identical across the three instantiations (the only
// retail difference is the reloc-masked ctor + EH funclet).
#define DISPATCH_BODY                                                                              \
    CLogicCtrl* rec = arg->m_7c;                                                                   \
    switch (rec->m_1c) {                                                                           \
        case 0: {                                                                                  \
            rec->m_1c = 0x3e8;                                                                     \
            CObj* o = new CObj(arg);                                                               \
            o->Activate();                                                                         \
            rec->m_18 = o;                                                                         \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_18->UserLogicVfunc9();                                                          \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_18->UserLogicVfunc8();                                                          \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_18->UserLogicVfuncC();                                                          \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_18->UserLogicVfuncB();                                                          \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_18->UserLogicVfuncA();                                                          \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_18->UserLogicVfuncD();                                                          \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            ProjTypeXfer(rec->m_18);                                                               \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x000aa0a0, 0xf1)
i32 Dispatch_aa0a0(CMsgObj* arg){DISPATCH_BODY}

RVA(0x000aa1e0, 0xf1)
i32 Dispatch_aa1e0(CMsgObj* arg){DISPATCH_BODY}

RVA(0x000aa460, 0xf1)
i32 Dispatch_aa460(CMsgObj* arg){DISPATCH_BODY}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CObj);
