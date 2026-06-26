// StrStream.h - the MSVC 5.0 pre-standard iostreams `streambuf` / `strstreambuf`
// (statically linked from LIBCMT.LIB). These are LIBRARY classes, not engine
// code; we reconstruct their bodies because they still occupy real `.text` bytes
// in retail GRUNTZ.EXE. Layout is taken verbatim from the VC5 headers STREAMB.H /
// STRSTREA.H (built with _MT, so streambuf carries LockFlg + a 24-byte
// _CRT_CRITICAL_SECTION) - see docs/libraries-and-funcid.md s1.4.
//
// The classes are modeled NON-polymorphic: the ctors store the retail vtable
// address directly (`mov [this], &vtbl`, a reloc-masked DATA() extern) because the
// vtables point at slots outside this cluster (the deleting dtor thunk, overflow,
// seekoff, ...), so letting cl emit its own vtable would diverge. Virtual
// dispatch in the bodies is written as an explicit indirect call through the
// stored vptr, matching `call [reg+off]` byte for byte.
#ifndef GRUNTZ_CRT_STRSTREAM_H
#define GRUNTZ_CRT_STRSTREAM_H

#include <rva.h>

// operator delete / free, called by the buffer teardown (reloc-masked rel32).
void operator delete(void*);

// _CRT_CRITICAL_SECTION init/destroy helpers (InitializeCriticalSection /
// DeleteCriticalSection thunks). Modeled extern-only -> reloc-masked.
extern "C" void __cdecl crt_lockinit(void* pcs); // 0x16c9c0
extern "C" void __cdecl crt_lockterm(void* pcs); // 0x16c9d0

// _mtlock / _mtunlock (EnterCriticalSection / LeaveCriticalSection thunks driving
// the streambuf/ios lock helpers). Modeled extern-only -> reloc-masked.
extern "C" void __cdecl crt_mtlock(void* pcs);   // 0x16c9e0
extern "C" void __cdecl crt_mtunlock(void* pcs); // 0x16c9f0

// The retail vtables (referenced by absolute address from the ctors). Declared
// as data; DATA() pins the symbol so the `mov [this], imm32` store is masked.
DATA(0x005f042c)
extern void* const streambuf_vtbl[];
DATA(0x005f0344)
extern void* const strstreambuf_vtbl[];

// ---------------------------------------------------------------------------
// streambuf - the abstract base. Field layout from STREAMB.H (_MT build).
// sizeof == 0x4c.
// ---------------------------------------------------------------------------
class ostream;

class streambuf {
public:
    streambuf();
    ~streambuf();

    i32 out_waiting() const;
    i32 sbumpc();

    // in_avail() - bytes available in the get area: _egptr - _gptr, or 0.
    i32 in_avail() const {
        return (_gptr < _egptr) ? (i32)(_egptr - _gptr) : 0;
    }

    i32 sync();
    i32 xsputn(const char* s, i32 n);

    // streambuf::lock / unlock - take the per-object critical section iff LockFlg<0.
    void lock() {
        if (LockFlg < 0) {
            crt_mtlock(&x_lock);
        }
    }
    void unlock() {
        if (LockFlg < 0) {
            crt_mtunlock(&x_lock);
        }
    }

    // sputc(i) - store one byte into the put area, else overflow(i). Defined
    // out-of-line below (needs SbView for the overflow dispatch).
    i32 sputc(i32 i);

    friend class ostream;

protected:
    void setb(char* b, char* eb, i32 a);

    // --- layout (offsets are load-bearing) ---
    void* m_vptr;    // 0x00
    i32 _fAlloc;     // 0x04
    i32 _fUnbuf;     // 0x08
    i32 x_lastc;     // 0x0c
    char* _base;     // 0x10
    char* _ebuf;     // 0x14
    char* _pbase;    // 0x18
    char* _pptr;     // 0x1c
    char* _epptr;    // 0x20
    char* _eback;    // 0x24
    char* _gptr;     // 0x28
    char* _egptr;    // 0x2c
    i32 LockFlg;     // 0x30
    char x_lock[24]; // 0x34 - _CRT_CRITICAL_SECTION (6 dwords)
};

// A polymorphic vtable-VIEW of streambuf, cast onto `this` only to make the two
// indirect virtual dispatches (overflow @slot7, underflow @slot8) fall out as
// `mov reg,[this]; call [reg+slot]` - the data-layout class stays non-polymorphic
// with an explicit m_vptr (docs/patterns/explicit-mvptr-no-virtuals.md). The
// leading dummy slots are never called; they only place the real two at +0x1c/+0x20.
struct SbView {
    virtual void v0();                    // 0x00 ~ / deleting dtor
    virtual i32 sync();                   // 0x04
    virtual void* setbuf();               // 0x08
    virtual i32 seekoff();                // 0x0c
    virtual i32 seekpos();                // 0x10
    virtual i32 xsputn(const char*, i32); // 0x14
    virtual i32 xsgetn(char*, i32);       // 0x18 (slot 6)
    virtual i32 overflow(i32);            // 0x1c (slot 7)
    virtual i32 underflow();              // 0x20 (slot 8)
};

inline i32 streambuf::sputc(i32 i) {
    return (_pptr < _epptr) ? (u8)(*_pptr++ = (char)i) : ((SbView*)this)->overflow(i);
}

// ---------------------------------------------------------------------------
// strstreambuf : public streambuf. Field layout from STRSTREA.H.
// sizeof == 0x64.
// ---------------------------------------------------------------------------
class strstreambuf : public streambuf {
public:
    strstreambuf();
    strstreambuf(i32 n);
    strstreambuf(void* (*allocf)(i32), void (*freef)(void*));
    strstreambuf(char* b, i32 n, char* curp);
    ~strstreambuf();

    char* str();
    i32 underflow();

    // setbuf(b, n) - set the alloc granularity (0x1693f0, reloc-masked extern);
    // called by the int-sizing ctor. Returns this in retail; we discard it.
    streambuf* setbuf(char* b, i32 n);

private:
    i32 x_dynamic;         // 0x4c
    i32 x_bufmin;          // 0x50
    i32 _fAlloc2;          // 0x54
    i32 x_static;          // 0x58
    void* (*x_alloc)(i32); // 0x5c
    void (*x_free)(void*); // 0x60
};

#endif // GRUNTZ_CRT_STRSTREAM_H
