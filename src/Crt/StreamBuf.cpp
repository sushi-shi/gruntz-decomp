#include <Crt/StrStream.h>
// StreamBuf.cpp - MSVC 5.0 LIBCMT `streambuf` base methods (non-EH leaves).
// Library code reconstructed for the byte-match; see include/Crt/StrStream.h.
//
// The class is modeled non-polymorphic with a manually-stored vptr (the retail
// vtable address, reloc-masked) so the `mov [this], &vtbl` stores + the explicit
// `call [vptr+slot]` virtual dispatches match retail byte for byte.
// ---------------------------------------------------------------------------

// out_waiting() - bytes pending in the put area: _pptr - _pbase, or 0.
RVA(0x00021280, 0x10)
i32 streambuf::out_waiting() const {
    return (_pptr >= _pbase) ? (_pptr - _pbase) : 0;
}

// streambuf() - zero the buffer pointers, x_lastc/LockFlg = EOF/-1, init the lock.
// @early-stop
// zero/constant-register-pinning wall: retail hoists `-1` into ecx up front and
// keeps it live for x_lastc + LockFlg; cl reloads `or eax,-1` late. Body otherwise
// byte-identical. See docs/patterns/zero-register-pinning.md (no /O2 source lever).
RVA(0x0016b070, 0x42)
streambuf::streambuf() {
    _fAlloc = 0;
    _fUnbuf = 0;
    _base = 0;
    _ebuf = 0;
    _pbase = 0;
    _pptr = 0;
    _epptr = 0;
    _eback = 0;
    _gptr = 0;
    _egptr = 0;
    m_vptr = (void*)streambuf_vtbl;
    x_lastc = -1;
    LockFlg = -1;
    crt_lockinit(&x_lock);
}

// ~streambuf() - delete the lock, sync, free the buffer if owned.
RVA(0x0016b150, 0x35)
streambuf::~streambuf() {
    m_vptr = (void*)streambuf_vtbl;
    crt_lockterm(&x_lock);
    sync();
    if (_fAlloc && _base) {
        operator delete(_base);
    }
}

// xsputn(s, n) - copy n bytes through the put area, falling back to overflow().
RVA(0x0016b1d0, 0x58)
i32 streambuf::xsputn(const char* s, i32 n) {
    i32 count = 0;
    while (n-- != 0) {
        if (!_fUnbuf && _pptr < _epptr) {
            *_pptr = *s;
            _pptr++;
        } else {
            if (((SbView*)this)->overflow((u8)*s) == -1) {
                break;
            }
        }
        s++;
        count++;
    }
    return count;
}

// sync() - 0 if both the get and put areas are drained, else -1.
RVA(0x0016b320, 0x1b)
i32 streambuf::sync() {
    if (_gptr < _egptr || _pptr > _pbase) {
        return -1;
    }
    return 0;
}

// setb(b, eb, a) - establish the reserve area, freeing the old one if owned.
RVA(0x0016b3a0, 0x33)
void streambuf::setb(char* b, char* eb, i32 a) {
    if (_fAlloc && _base) {
        operator delete(_base);
    }
    _base = b;
    _fAlloc = a;
    _ebuf = eb;
}

// sbumpc() - read the current char and advance the get pointer.
// @early-stop
// regalloc wall: in the buffered get path retail keeps `_gptr` in eax and zero-
// extends the byte through ecx (`xor ecx,ecx; mov cl,[eax]; mov eax,ecx`); cl
// keeps `_gptr` in ecx and reads straight into al. Structure byte-identical;
// branch-order/operand swap not source-steerable (cf. zero-register-pinning.md).
RVA(0x0016cd00, 0x4b)
i32 streambuf::sbumpc() {
    if (_fUnbuf) {
        if (x_lastc == -1) {
            return ((SbView*)this)->underflow();
        }
        i32 c = x_lastc;
        x_lastc = -1;
        return c;
    }
    if (_gptr < _egptr) {
        i32 c = (u8)*_gptr;
        _gptr++;
        return c;
    }
    i32 r = ((SbView*)this)->underflow();
    _gptr++;
    return r;
}
