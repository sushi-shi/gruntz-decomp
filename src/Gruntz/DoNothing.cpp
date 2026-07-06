// DoNothing.cpp - the inert "do nothing" tile-logic game-object family
// (C:\Proj\Gruntz): CDoNothing and its sibling CDoNothingNormal.
//
// One dev TU, formerly split across DoNothing.cpp + DoNothingNormalDtor.cpp +
// DoNothingNormalLogic.cpp (all /GX, all this family). Methods in ascending
// retail-RVA order:
//   CDoNothing::GetTypeTag         @0x00f6b0 - 6-byte logic-type id accessor (0x3ec)
//   CDoNothing::~CDoNothing        @0x00f770 - /GX leaf dtor (CUserLogic teardown)
//   CDoNothingNormal::~CDoNothingNormal @0x00f8a0 - /GX leaf dtor
//   HandlerA9E00 (DoNothingNormal pump) @0x0a9e00 - the __cdecl logic-worker pump
//   CDoNothing::CDoNothing         @0x0ac1d0 - the ctor (BigActHeight de-prioritize)
//
// CDoNothing / CDoNothingNormal : CUserLogic (base hierarchy from
// <Gruntz/UserLogic.h>). Only offsets / code bytes are load-bearing; names are
// placeholders for the recovered engine identities.
#include <Gruntz/DoNothing.h>
#include <Gruntz/DoNothingNormalDtor.h>
#include <Gruntz/LogicTypeId.h>

#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// DoNothingNormal logic-worker pump (@0x0a9e00) local views. The SAME archetype as
// LogicWorkerHandlers.cpp's four pumps (0xa9cc0 / 0xaa5a0 / 0xaa960 / 0xaf0a0):
// the __cdecl per-message pump that NEWS the CDoNothingNormal logic record on init
// and dispatches the record's vtable slots on every later state tag.
//
// The one structural difference from the four sibling pumps: case 0 inlines the
// CDoNothingNormal leaf ctor instead of calling a single external full ctor. That
// leaf ctor chains the OUT-OF-LINE shared CUserLogic(owner) base (CUserLogic_058cd0
// @0x58cd0, reached through the 0x3828 ILT thunk; owned by the CUserLogic TU) -
// which does NOT set m_34/m_38/m_3c - then sets m_34/m_38/m_3c, stamps the
// CDoNothingNormal vtable (0x5e859c) AFTER those stores, and raises owner->m_08
// bit 0. Because the vtable re-stamp lands after the member stores (not the
// compiler's auto position) and its contents are the engine class's, the record is
// modeled NON-polymorphically with a manual vptr field + a declared-only
// polymorphic interface (EngRec) for the dispatch lowering.
//
// Field names are placeholders; only OFFSETS + the emitted code bytes are load-
// bearing (campaign doctrine).

struct Owner;

// The OUT-OF-LINE shared CUserLogic(CGameObject*) base ctor (CUserLogic_058cd0
// @0x58cd0). External/no-body so the chained `call` reloc-masks. It sets
// m_0c/m_10/m_14/m_04/m_08/m_28/m_2c + the throwing link + AddLogic*; it does NOT
// set m_34/m_38/m_3c (the leaf does). Modeled as a non-polymorphic base class so
// the leaf record chains it; the throwing link ctor inside forces the /GX EH frame.
struct CUserLogicOOL {
    virtual void Vf0();          // +0x00  declared-only vptr anchor (polymorphic base)
    CUserLogicOOL(Owner* owner); // 0x58cd0
    char m_pad04[0x34 - 0x04];   // +0x04..+0x33
    Owner* m_34;                 // +0x34
    Owner* m_38;                 // +0x38
    void* m_3c;                  // +0x3c
};

// Real polymorphic: DnnRec (a distinct polymorphic leaf) makes cl auto-stamp its own
// ??_7DnnRec at ctor entry, replacing the old vptr-MIDDLE manual stamp of the engine
// CDoNothingNormal vtable (0x5e859c; realized as ??_7CDoNothingNormal below via
// RealizeCDoNothingNormal). The auto-stamp lands at ctor entry (after the base ctor)
// rather than after the member stores - an accepted codegen shift (see @early-stop).
struct DnnRec : CUserLogicOOL {
    char m_leaf[0x54 - 0x40]; // +0x40..+0x53
    DnnRec(Owner* owner);
};

// The dispatch interface: a polymorphic class with the same vtable slot layout as
// the engine record (slot 6 @ +0x18 = activate; slots 10..15 @ +0x28..+0x3c = the
// per-state handlers). Declared-only + never constructed here, so no ??_7 is
// emitted; a record* is reinterpreted to it only to lower `mov eax,[rec]; call
// [eax+N]`.
class EngRec {
public:
    virtual void s0();       // +0x00
    virtual void s1();       // +0x04
    virtual void s2();       // +0x08
    virtual void s3();       // +0x0c
    virtual void s4();       // +0x10
    virtual void s5();       // +0x14
    virtual void Activate(); // +0x18  (slot 6)
    virtual void s7();       // +0x1c
    virtual void s8();       // +0x20
    virtual void s9();       // +0x24
    virtual void V28();      // +0x28
    virtual void V2C();      // +0x2c
    virtual void V30();      // +0x30
    virtual void V34();      // +0x34
    virtual void V38();      // +0x38
    virtual void V3C();      // +0x3c
};

// The worker held at owner->m_7c; only the pump fields are modeled.
struct Worker {
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    char m_pad04[0x18 - 0x04];
    DnnRec* m_18; // +0x18  the live record
    u32 m_1c;     // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to the pump; its worker hangs at +0x7c.
struct Owner {
    char m_pad00[0x08];
    u32 m_08; // +0x08
    char m_pad0c[0x7c - 0x0c];
    Worker* m_7c; // +0x7c
};

// The engine default message pump for any unhandled state (0x16e4f0, __cdecl,
// takes the record). Reloc-masked rel32 - no body.
extern "C" void Worker_DefaultPump(DnnRec* rec);

// The CDoNothingNormal record allocator (referenced by RealizeCDoNothingNormal and
// the DnnRec new below).
void* operator new(u32);

// case 0: the inlined CDoNothingNormal leaf ctor (base OOL ctor + leaf tail).
inline DnnRec::DnnRec(Owner* owner) : CUserLogicOOL(owner) {
    m_34 = owner;
    m_38 = owner;
    m_3c = owner->m_7c;
    m_38->m_08 |= 1;
}

// ---------------------------------------------------------------------------
// CDoNothing::GetTypeTag @0x00f6b0 - return the class's logic-type id. The same
// 6-byte `mov eax,<id>; ret` virtual archetype as CTileTriggerTransition::
// GetTypeTag (0x011730).
RVA(0x0000f6b0, 0x6)
LogicTypeId CDoNothing::GetTypeTag() {
    return LOGIC_DONOTHING; // 0x3ec
}

// CDoNothing::~CDoNothing @0x00f770 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call
// 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible link forces the
// /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70; the empty body is
// enough for cl.
RVA(0x0000f770, 0x44)
CDoNothing::~CDoNothing() {}

// CDoNothingNormal::~CDoNothingNormal @0x0000f8a0 - folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical to ~CDoNothing
// @0x0000f770.
RVA(0x0000f8a0, 0x44)
CDoNothingNormal::~CDoNothingNormal() {}

// Realize ??_7CDoNothingNormal@@6B@ (0x1e859c): retail's dtor folds straight to the
// CUserLogic teardown and never references the leaf vtable (so ~CDoNothingNormal only
// emits the base ??_7CUserLogic/??_7CUserBase restamps), and the logic-worker ctor
// stamps the leaf vtable vptr-MIDDLE - neither anchors the leaf COMDAT. A spurious
// `new CDoNothingNormal` references the implicit vptr-FIRST leaf ctor, whose stamp
// (the escaping object keeps it) emits ??_7CDoNothingNormal. Unpaired (no RVA) ->
// matching-neutral; it does NOT touch the 0xf8a0 dtor codegen.
CDoNothingNormal* RealizeCDoNothingNormal();
CDoNothingNormal* RealizeCDoNothingNormal() {
    return new CDoNothingNormal();
}

// HandlerA9E00 @0x0a9e00 - re-homed from src/Stub/CDoNothingNormal.cpp (where the
// this/ecx trace mis-named it the CDoNothingNormal ctor - it is really the __cdecl
// per-message pump).
//
// The switch key worker->m_1c is UNSIGNED (u32); MSVC5 then emits the range checks
// as unsigned ja/jbe, matching retail byte-for-byte (switch-key-unsigned-ja-vs-jg).
//
// @early-stop
// case-0 leaf-ctor-tail scheduling/regalloc coin-flip (~97.9%, topic:wall
// topic:scheduling): the whole pump + the new/base-ctor/EH-state framework are
// byte-identical (verified base vs target with llvm-objdump -dr). Two residual
// deltas, both in the inlined case-0 tail: (1) MSVC now auto-stamps the ??_7DnnRec
// vptr at ctor entry (after the base ctor), whereas retail stamps the leaf vtable
// vptr-MIDDLE, after the `m_3c = owner->m_7c` load/store; (2) MSVC lowers
// `m_38->m_08 |= 1` as an 8-byte load / or al,1 /
// store through the live param edi, whereas retail reloads m_38 from [esi+0x38]
// and does the 7-byte `or dword [eax+0x8],1` RMW - making the body 1 byte longer.
// Neither is source-steerable (tried reordered stores, a temp for the load, and a
// cast-through-pointer vptr write - all identical codegen). Parked for the final
// sweep.
RVA(0x000a9e00, 0x10c)
i32 HandlerA9E00(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            DnnRec* sub = new DnnRec(owner);
            ((EngRec*)sub)->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            ((EngRec*)rec->m_18)->V2C();
            break;
        case 0x1e:
            ((EngRec*)rec->m_18)->V28();
            break;
        case 0x50:
            ((EngRec*)rec->m_18)->V38();
            break;
        case 0x53:
            ((EngRec*)rec->m_18)->V3C();
            break;
        case 0x52:
            ((EngRec*)rec->m_18)->V30();
            break;
        case 0x51:
            ((EngRec*)rec->m_18)->V34();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// CDoNothing::CDoNothing @0xac1d0 - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), flag the sub-object, then run the
// shared BigActHeight "big-act" de-prioritize tail (the SAME archetype as
// CSimpleAnimation / CEyeCandy).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ac1d0, 0x1a5)
CDoNothing::CDoNothing(CGameObject* obj) : CTileLogic(obj) {
    m_38->m_flags |= 1;
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

SIZE_UNKNOWN(CUserLogicOOL);
SIZE_UNKNOWN(DnnRec);
SIZE_UNKNOWN(EngRec);
SIZE_UNKNOWN(Owner);
SIZE_UNKNOWN(Worker);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
