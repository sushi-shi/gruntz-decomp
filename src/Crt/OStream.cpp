#include <Crt/OStream.h>
// OStream.cpp - MSVC 5.0 LIBCMT `ostream` member functions (non-EH leaves). The
// ios virtual base is reached through the vbtable (cl emits `mov eax,[this]; mov
// eax,[eax+4]; add eax,this`); the bp (streambuf) virtual calls go through the
// SbView cast so the `call [vtbl+slot]` dispatches match retail byte for byte.
// Library code reconstructed for the byte-match; see include/Crt/OStream.h.
// ---------------------------------------------------------------------------

// write(s, n) - sputn n bytes through bp; flag failbit|badbit on a short write.
RVA(0x0016ab20, 0x49)
ostream& ostream::write(const char* s, i32 n) {
    if (opfx()) {
        if (((SbView*)bp)->xsputn(s, n) != n) {
            state |= (failbit | badbit);
        }
        osfx();
    }
    return *this;
}

// opfx() - output prefix: lock, bail (failbit) if already in error, flush the
// tied stream, then lock the buffer. Returns 1 on success, 0 on error.
RVA(0x0016bd10, 0x7f)
i32 ostream::opfx() {
    lock();
    if (state) {
        state |= failbit;
        unlock();
        return 0;
    }
    if (x_tie) {
        ((ostream*)x_tie)->flush();
    }
    bp->lock();
    return 1;
}

// osfx() - output suffix: reset the field width, unitbuf-sync / stdio-flush,
// then drop the buffer + object locks.
RVA(0x0016bd90, 0xca)
void ostream::osfx() {
    x_width = 0;
    if (x_flags & unitbuf) {
        if (((SbView*)bp)->sync() == -1) {
            state = (failbit | badbit);
        }
    }
    if (x_flags & stdio) {
        if (crt_flush(g_iob_stdout) == -1) {
            state |= failbit;
        }
        if (crt_flush(g_iob_stderr) == -1) {
            state |= failbit;
        }
    }
    bp->unlock();
    unlock();
}

// operator<<(unsigned char) - insert one char, padded to the field width.
// @early-stop
// shrink-wrapped callee-save wall: retail defers `push ebx` past the if(opfx())
// guard (ebx only live on the taken path); cl pushes ebx in the prologue, which
// flips the matching `pop ebx;pop esi` order too. Body otherwise byte-identical
// (~90%). See docs/patterns/shrink-wrapped-callee-save-push.md (not /O2-steerable).
RVA(0x00192060, 0xb5)
ostream& ostream::operator<<(u8 c) {
    if (opfx()) {
        if (x_width != 0) {
            char buf[2];
            buf[0] = c;
            buf[1] = 0;
            writepad(g_emptyString, buf);
            osfx();
            return *this;
        }
        if (bp->sputc(c) == -1 && ((SbView*)bp)->overflow(c) == -1) {
            state |= (failbit | badbit);
        }
        osfx();
    }
    return *this;
}
