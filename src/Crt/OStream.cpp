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

// seekp(sp) - reposition the put pointer via bp->seekpos, fail-flagging on error.
// @early-stop
// vbase-pointer CSE/regalloc wall (~84%): logic + the read-before-lock setstate
// idiom are byte-exact, but retail hoists the ios vbase address into a callee-saved
// reg and holds it across the inlined lock/unlock; cl re-derives it from the vbtable
// per access. Caching via ios*/ios& injects a null-check retail lacks, so neither
// spelling closes it (regalloc choice, not source-steerable).
RVA(0x0016c4d0, 0x98)
ostream& ostream::seekp(i32 sp) {
    bp->lock();
    if (((SbView*)bp)->seekpos(sp, 2) == -1) {
        i32 newst = state | failbit;
        lock();
        state = newst;
        unlock();
    }
    bp->unlock();
    return *this;
}

// tellp() - report the current put position via bp->seekoff(0, cur, out).
// @early-stop
// same ios-vbase CSE/regalloc wall as seekp (~82%): logic byte-exact, residual is
// the held-vbase-pointer register choice across the inlined lock/unlock.
RVA(0x0016c610, 0x99)
i32 ostream::tellp() {
    bp->lock();
    i32 sp = ((SbView*)bp)->seekoff(0, 1, 2);
    if (sp == -1) {
        i32 newst = state | failbit;
        lock();
        state = newst;
        unlock();
    }
    bp->unlock();
    return sp;
}

// operator<<(streambuf*) - pump every char from sb through bp until EOF.
RVA(0x0016c6b0, 0x88)
ostream& ostream::operator<<(streambuf* sb) {
    if (opfx()) {
        i32 c = sb->sbumpc();
        while (c != -1) {
            if (bp->sputc(c) == -1) {
                state |= failbit;
                break;
            }
            c = sb->sbumpc();
        }
        osfx();
    }
    return *this;
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
