#include <Crt/IStream.h>
// IStream.cpp - MSVC 5.0 LIBCMT `istream` member functions (non-EH leaves). The
// ios virtual base is reached through the vbtable (cl emits `mov eax,[this]; mov
// eax,[eax+4]; add eax,this`); the bp (streambuf) virtual call (xsgetn @slot6)
// goes through the SbView cast so the `call [vtbl+slot]` dispatches match retail
// byte for byte. Library code reconstructed for the byte-match; see
// include/Crt/IStream.h. The ctor (0x16b510, /GX vbase init) lives separately.
// ---------------------------------------------------------------------------

// get() - read one char via the buffer; flag eofbit at end, else bump gcount.
RVA(0x0016a410, 0x7b)
i32 istream::get() {
    if (ipfx(1)) {
        i32 c = bp->sbumpc();
        if (c == -1) {
            state |= eofbit;
        } else {
            x_gcount++;
        }
        isfx();
        return c;
    }
    return -1;
}

// read(s, n) - pull n bytes through bp->xsgetn; flag eofbit|failbit on a short read.
RVA(0x0016a510, 0x7d)
istream& istream::read(char* s, i32 n) {
    if (ipfx(1)) {
        x_gcount = ((SbView*)bp)->xsgetn(s, n);
        if ((u32)x_gcount < (u32)n) {
            state |= (eofbit | failbit);
        }
        isfx();
    }
    return *this;
}

// ipfx(need) - input prefix: lock, bail (failbit) if already in error, flush the
// tied stream when the buffer can't satisfy `need`, lock the buffer, and (when
// need==0 and skipws) eat leading whitespace. Returns 1 on success, 0 on error.
// @early-stop
// regalloc wall (~96%): retail keeps the long-lived `need` param in edi (2 saved
// regs: esi=this, edi=need) and computes the in_avail buffer diff in edx; cl pins
// `need` in ebx and uses edi for the diff temp, adding a 3rd `push ebx`/`pop ebx`.
// Body from the tied-flush onward is byte-identical. Not /O2 source-steerable
// (the reverse of pin-local-for-callee-saved-reg.md: we need FEWER saved regs).
RVA(0x0016b720, 0xfb)
i32 istream::ipfx(i32 need) {
    lock();
    if (need) {
        x_gcount = 0;
    }
    if (state) {
        state |= failbit;
        unlock();
        return 0;
    }
    if (x_tie) {
        if (!need || need > bp->in_avail()) {
            ((ostream*)x_tie)->flush();
        }
    }
    bp->lock();
    if (!need && (x_flags & skipws)) {
        eatwhite();
        if (state) {
            state |= failbit;
            bp->unlock();
            unlock();
            return 0;
        }
    }
    return 1;
}
