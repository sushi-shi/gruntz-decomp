// StateDispatch.cpp - a message/state-id dispatcher (0x9b770).
//
// __stdcall(ctx, a1, a2) reached with a context whose +0x7c state object carries
// the current state id (+0x1c) and the active handler (+0x18).  A sparse switch
// over the id either (id 0) operator-new's + constructs a fresh 0x54-byte handler
// (id then 0x3e8), initialises it (vtable +0x18) and installs it, or routes one of
// a handful of ids to the active handler's matching virtual slot, or (default)
// hands the active handler to a fallback.  Always returns 1.
//
// Field names are placeholders; only the OFFSETS, the switch case VALUES and the
// vtable SLOT offsets are load-bearing.  The construction is spelled as a real `new
// CStateHandler(ctx)` (the honest shape - retail's new-site is op-new + a bare
// out-of-line ctor call at 0x9b8b0) and the handler is a real 16-slot polymorphic
// view of CUserLogic (not the old PMF table + RezAlloc hack).
#include <rva.h>

struct CStateCtx; // fwd: the new-expression passes it to the handler ctor

// The default case runs the type-keyed record transfer/dispatch: NOT a bespoke
// "fallback" - it is the shared CTypeKeyColl serializer ProjTypeXfer (0x16e4f0,
// owned + matched in TypeKeyColl.cpp), which resolves the handler's type-registry
// entry (ar->m_14->m_1c) and xfers it through the handler's own vtable slots. The
// active handler (m_18) is passed as the archive-record arg.
struct CXferArchive;                // TypeKeyColl.cpp-local archive-record view
i32 ProjTypeXfer(CXferArchive* ar); // 0x16e4f0

// IDENTITY: the created/active handler is a 0x54-byte CUserLogic (its ctor 0x9b8b0
// stamps ??_7CUserLogic@@6B@ / ??_7CUserBase@@6B@; 16-slot vtable @0x5e705c). The
// canonical CUserLogic lives in <Gruntz/UserLogic.h>, currently FROZEN mid-ODR-merge
// by a parallel worker; so this TU keeps a self-contained polymorphic view with the
// SAME 16-slot layout (CUserLogic : CUserBase - real virtuals, not a PMF table). The
// virtual CALLS reloc-mask to the vtable offsets (0x18=[6], 0x28=[10], 0x2c=[11],
// 0x30=[12], 0x34=[13], 0x38=[14], 0x3c=[15]) and the ctor `call` reloc-masks to
// 0x9b8b0 - identical bytes regardless of the placeholder name. The external ctor
// means NO vtable is emitted here (no key fn) and the new-site is a bare `call <ctor>`
// exactly like retail. Only OFFSETS + slot indices are load-bearing.
// TODO(defer, matcher-2): fold this view onto the real CUserLogic once its header unfreezes.
struct CStateHandler {
    virtual void Slot00();         // [0] override (CUserBase)
    virtual void Serialize();      // [1] override
    virtual void Slot02();         // [2] override
    virtual void Slot03();         // [3]
    virtual void Slot04();         // [4]
    virtual void Finalize();       // [5]
    virtual void Slot18();         // [6] +0x18 init
    virtual void Slot07();         // [7]
    virtual void Slot08();         // [8]
    virtual void Slot09();         // [9]
    virtual void Slot28();         // [10] +0x28
    virtual void Slot2c();         // [11] +0x2c
    virtual void Slot30();         // [12] +0x30
    virtual void Slot34();         // [13] +0x34
    virtual void Slot38();         // [14] +0x38
    virtual void Slot3c();         // [15] +0x3c
    CStateHandler(CStateCtx* ctx); // 0x9b8b0 ctor (via ILT thunk 0x404d)
    char _pad[0x54 - 4];           // vptr@0 + this pad -> sizeof 0x54
};

struct CStateObj {
    char m_pad00[0x18];
    CStateHandler* m_18; // +0x18 active handler
    i32 m_1c;            // +0x1c current state id
};
struct CStateCtx {
    char m_pad00[0x7c];
    CStateObj* m_7c; // +0x7c state object
};

// @early-stop
// throwing-operator-new /GX frame wall (docs/patterns/gx-frame-outofline-ctor.md +
// rezalloc-placement-new-no-eh-frame.md): retail's id-0 `new CUserLogic(ctx)` wraps
// the BARE out-of-line ctor call (0x9b8b0) in the operator-delete-on-ctor-throw
// cleanup -> a full /GX frame (push -1/fs:0, [esp+0x10] state 0-during-ctor / -1-after,
// saved raw ptr [esp+0x18]) + every case `jmp`ing one shared fs:0-restoring epilogue.
// MSVC5 reconstruction CANNOT re-raise that frame for a bare out-of-line ctor `new`
// (verified: real-virtuals+new, +declared-dtor, and inline-derived-wrapping-out-of-line-
// base-ctor all stay frameless; the last one regresses to 23% on regalloc). The frame
// mechanism needs the throwing call inside an INLINE ctor that ALSO stamps a vtable,
// which would add a new-site vptr store retail does not have. Body/switch/dispatch are
// byte-faithful; ~32% is the frameless plateau. Deferred to the final sweep.
RVA(0x0009b770, 0xf1)
i32 __stdcall StateDispatch(CStateCtx* ctx, i32 a1, i32 a2) {
    CStateObj* st = ctx->m_7c;
    switch (st->m_1c) {
        case 0: {
            st->m_1c = 0x3e8;
            CStateHandler* h = new CStateHandler(ctx);
            h->Slot18();
            st->m_18 = h;
            break;
        }
        case 0x1d:
            st->m_18->Slot2c();
            break;
        case 0x1e:
            st->m_18->Slot28();
            break;
        case 0x50:
            st->m_18->Slot38();
            break;
        case 0x51:
            st->m_18->Slot34();
            break;
        case 0x52:
            st->m_18->Slot30();
            break;
        case 0x53:
            st->m_18->Slot3c();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)st->m_18);
            break;
    }
    return 1;
}
SIZE_UNKNOWN(CStateCtx);
SIZE_UNKNOWN(CStateHandler);
SIZE_UNKNOWN(CStateObj);
