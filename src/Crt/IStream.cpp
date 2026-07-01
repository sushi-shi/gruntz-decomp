#include <Crt/IoStream.h>
// IStream.cpp - MSVC 5.0 LIBCMT `istream` input methods. Library code
// reconstructed for the byte-match; see include/Crt/IoStream.h.

// get(char&) - extract one char (unformatted). ipfx(1) locks; on success pull a
// byte through bp->sbumpc(), set eofbit|failbit on EOF else bump x_gcount, store
// the byte, and isfx() drops the locks. Returns *this. Every ios-member touch
// (bp, state) reloads the vbptr - the virtual-base access retail emits.
RVA(0x0016a490, 0x7a)
istream& istream::get(char& c) {
    if (ipfx(1)) {
        i32 ch = bp->sbumpc();
        if (ch == -1) {
            state |= 3;
        } else {
            x_gcount++;
        }
        c = (char)ch;
        isfx();
    }
    return *this;
}
