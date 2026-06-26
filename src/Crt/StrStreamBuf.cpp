#include <Crt/StrStream.h>
#include <string.h>
// StrStreamBuf.cpp - MSVC 5.0 LIBCMT `strstreambuf` methods. The ctor / dtor
// carry the /GX SEH frame (a fully-constructed base subobject must be unwound),
// so this is the `eh` unit; str() / underflow() are plain leaves that ride along.
// Library code reconstructed for the byte-match; see include/Crt/StrStream.h.
// ---------------------------------------------------------------------------

// strstreambuf() - default dynamic-mode ctor: allocate-on-demand, no static buf.
RVA(0x00169000, 0x28)
strstreambuf::strstreambuf() {
    m_vptr = (void*)strstreambuf_vtbl;
    x_dynamic = 1;
    x_bufmin = 1;
    x_static = 0;
    x_alloc = 0;
    x_free = 0;
}

// strstreambuf(n) - dynamic-mode ctor with a caller-chosen alloc granularity
// (setbuf(0,n) stores it iff non-zero). /GX frame from the constructed base.
RVA(0x00169050, 0x5f)
strstreambuf::strstreambuf(i32 n) {
    m_vptr = (void*)strstreambuf_vtbl;
    x_dynamic = 1;
    x_static = 0;
    x_alloc = 0;
    x_free = 0;
    setbuf(0, n);
}

// strstreambuf(allocf, freef) - dynamic-mode ctor with caller-supplied
// allocate/free hooks.
RVA(0x001690b0, 0x34)
strstreambuf::strstreambuf(void* (*allocf)(i32), void (*freef)(void*)) {
    x_bufmin = 1;
    x_dynamic = 1;
    m_vptr = (void*)strstreambuf_vtbl;
    x_alloc = allocf;
    x_static = 0;
    x_free = freef;
}

// strstreambuf(b, n, curp) - wrap a caller buffer [b, b+n) (n<0 => unbounded,
// n==0 => strlen), get area starting at b, put area at curp (if any).
RVA(0x00169160, 0xbc)
strstreambuf::strstreambuf(char* b, i32 n, char* curp) {
    m_vptr = (void*)strstreambuf_vtbl;
    x_static = 1;
    x_dynamic = 0;
    char* end;
    if (n == 0) {
        end = b + strlen(b);
    } else if (n < 0) {
        end = (char*)-1;
    } else {
        end = b + n;
    }
    setb(b, end, 0);
    _eback = b;
    _gptr = b;
    if (curp != 0) {
        _egptr = curp;
        x_lastc = -1;
        _pbase = curp;
        _pptr = curp;
        _epptr = end;
    } else {
        _egptr = end;
        x_lastc = -1;
        _pbase = 0;
        _pptr = 0;
        _epptr = 0;
    }
}

// ~strstreambuf() - free a dynamically-grown buffer (via x_free or delete), then
// let ~streambuf run.
// @early-stop
// /GX dtor scheduling wall: retail emits the vptr store (`mov [esi],&vtbl`) eagerly
// right after the prologue, BEFORE reading x_dynamic and writing the entry trylevel;
// cl reads x_dynamic + writes the trylevel first. Body otherwise byte-identical (the
// EH scopetable `push $8` falls out once the order matches). See
// docs/patterns/eh-dtor-vptr-stamp-vs-trylevel-order.md (not /O2-steerable).
RVA(0x00169220, 0x6c)
strstreambuf::~strstreambuf() {
    m_vptr = (void*)strstreambuf_vtbl;
    if (x_dynamic && _base) {
        if (x_free) {
            x_free(_base);
        } else {
            operator delete(_base);
        }
    }
}

// str() - freeze the buffer and return its base.
RVA(0x001692b0, 0xb)
char* strstreambuf::str() {
    char* b = _base;
    x_dynamic = 0;
    return b;
}

// underflow() - re-sync the get area onto freshly written put data, then peek.
// @early-stop
// regalloc wall: retail loads `_egptr` first and pins `_gptr` in esi (callee-saved)
// across the body; cl loads `_gptr` first into edx. Identical structure/offsets, only
// the register picks + first-load order differ. Caching `_gptr` in a local drops a
// callee-save push (diverges further). See docs/patterns/zero-register-pinning.md.
RVA(0x00169490, 0x49)
i32 strstreambuf::underflow() {
    if (_gptr >= _egptr) {
        if (_egptr < _pptr) {
            _gptr = _base - _eback + _gptr;
            _eback = _base;
            _egptr = _pptr;
            x_lastc = -1;
        }
        if (_gptr >= _egptr) {
            return -1;
        }
    }
    return (u8)*_gptr;
}
