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
// vtable SLOT offsets are load-bearing.  The construction body is byte-faithful;
// the operator-new ctor-in-flight /GX frame is the residual.
#include <rva.h>

// Engine operator new (reloc-masked __cdecl leaf).
extern "C" void* SdOperatorNew(unsigned int); // 0x1b9b46

// The fallback the default case hands the active handler to (__cdecl, 1 arg).
extern "C" void SdFallback(void* handler); // 0x16e4f0

// The active/created handler: a polymorphic object whose dispatched slots live at
// vtable byte offsets 0x18/0x28/0x2c/0x30/0x34/0x38/0x3c.  Declared-only virtuals
// (no ??_7 emitted here; the object is constructed elsewhere / stamped by its own
// ctor) reach those slots; v18 is the post-construction init.
struct CStateHandler {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18(); // +0x18 init
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28(); // +0x28
    virtual void v2c(); // +0x2c
    virtual void v30(); // +0x30
    virtual void v34(); // +0x34
    virtual void v38(); // +0x38
    virtual void v3c(); // +0x3c
    // The fresh-handler ctor (ILT thunk 0x404d): returns this in eax.
    CStateHandler* Ctor(void* ctx); // 0x404d
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
// operator-new ctor-in-flight /GX frame: the sparse switch, the seven vtable-slot
// dispatches and the fresh-handler construction (id 0) are byte-faithful, but the
// case-0 throwing-new carries retail's push -1/fs:0 cleanup frame the manual
// operator-new body omits, shifting the switch's jump-table-relative layout.
RVA(0x0009b770, 0xf1)
i32 __stdcall StateDispatch(CStateCtx* ctx, i32 a1, i32 a2) {
    CStateObj* st = ctx->m_7c;
    switch (st->m_1c) {
        case 0: {
            st->m_1c = 0x3e8;
            CStateHandler* obj = (CStateHandler*)SdOperatorNew(0x54);
            CStateHandler* h;
            if (obj != 0) {
                h = obj->Ctor(ctx);
            } else {
                h = 0;
            }
            h->v18();
            st->m_18 = h;
            break;
        }
        case 0x1d:
            st->m_18->v2c();
            break;
        case 0x1e:
            st->m_18->v28();
            break;
        case 0x50:
            st->m_18->v38();
            break;
        case 0x51:
            st->m_18->v34();
            break;
        case 0x52:
            st->m_18->v30();
            break;
        case 0x53:
            st->m_18->v3c();
            break;
        case 0x3e8:
            break;
        default:
            SdFallback(st->m_18);
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Class metadata (SIZE sweep) - hosted at TU EOF; labels.py scans tree-wide.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CStateCtx);
SIZE_UNKNOWN(CStateHandler);
SIZE_UNKNOWN(CStateObj);
