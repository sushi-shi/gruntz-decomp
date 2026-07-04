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
extern "C" void* RezAlloc(unsigned int); // 0x1b9b46

// The fallback the default case hands the active handler to (__cdecl, 1 arg).
extern "C" void SdFallback(void* handler); // 0x16e4f0

// The active/created handler: a FOREIGN 0x54-byte engine object. Its ??_7 and the
// dispatched slot bodies (0x18 init, 0x28/0x2c/0x30/0x34/0x38/0x3c) are
// unreconstructed engine code, so the honest model is a manual vptr at +0 into a
// vtable struct naming ONLY the used slots (the gaps are documented `char` pad),
// NOT a fabricated run of virtuals. The fresh handler is operator-new'd + stamped
// by its own external ctor (0x404d). Class COMPLETE before the T::* typedef so the
// PMF stays 4 bytes (docs/patterns/pmf-complete-class-4byte.md).
struct CStateHandlerVtbl;
struct CStateHandler {
    CStateHandlerVtbl* vptr; // +0x00
    char _pad[0x54 - 4];
    CStateHandler* Ctor(void* ctx); // 0x404d ctor: returns this in eax
    void Slot18();                  // +0x18 init
    void Slot28();                  // +0x28
    void Slot2c();                  // +0x2c
    void Slot30();                  // +0x30
    void Slot34();                  // +0x34
    void Slot38();                  // +0x38
    void Slot3c();                  // +0x3c
};
typedef void (CStateHandler::*StateFn)();
struct CStateHandlerVtbl {
    char _00[0x18];
    StateFn s18; // +0x18
    char _1c[0x28 - 0x1c];
    StateFn s28; // +0x28
    StateFn s2c; // +0x2c
    StateFn s30; // +0x30
    StateFn s34; // +0x34
    StateFn s38; // +0x38
    StateFn s3c; // +0x3c
};
inline void CStateHandler::Slot18() {
    (this->*(vptr->s18))();
}
inline void CStateHandler::Slot28() {
    (this->*(vptr->s28))();
}
inline void CStateHandler::Slot2c() {
    (this->*(vptr->s2c))();
}
inline void CStateHandler::Slot30() {
    (this->*(vptr->s30))();
}
inline void CStateHandler::Slot34() {
    (this->*(vptr->s34))();
}
inline void CStateHandler::Slot38() {
    (this->*(vptr->s38))();
}
inline void CStateHandler::Slot3c() {
    (this->*(vptr->s3c))();
}

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
            CStateHandler* obj = (CStateHandler*)RezAlloc(0x54);
            CStateHandler* h;
            if (obj != 0) {
                h = obj->Ctor(ctx);
            } else {
                h = 0;
            }
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
            SdFallback(st->m_18);
            break;
    }
    return 1;
}
SIZE_UNKNOWN(CStateCtx);
SIZE_UNKNOWN(CStateHandler);
SIZE_UNKNOWN(CStateHandlerVtbl);
SIZE_UNKNOWN(CStateObj);
