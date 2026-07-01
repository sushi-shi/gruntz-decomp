// IoStream.h - MSVC 5.0 LIBCMT `ios` / `istream` (pre-standard iostreams,
// statically linked from LIBCMT.LIB). LIBRARY classes, reconstructed for the
// byte-match like the `streambuf` base in <Crt/StrStream.h>.
//
// `istream` VIRTUALLY inherits `ios`, so every access to an ios-subobject member
// lowers to the vbptr indirection retail uses: `mov reg,[this]` (load the vbptr),
// `mov reg,[reg+4]` (the ios offset from vbtable slot 1), `mov ..,[reg+this+off]`.
// The most-derived istream ctor (0x16b480) puts the ios subobject at +0xc and
// stamps the vbtable @0x5f0460 / ios vtable @0x5f045c; we define ONLY the leaf
// method here (no ctor), so no vtable/vbtable data is emitted and nothing to
// diverge. `ios` is modeled non-polymorphic (a plain vptr slot at +0, stamped
// externally), _MT layout (LockFlg + a 24-byte critical section).
#ifndef GRUNTZ_CRT_IOSTREAM_H
#define GRUNTZ_CRT_IOSTREAM_H

#include <Crt/StrStream.h>

// ---------------------------------------------------------------------------
// ios - the iostream base. Field offsets from IOS.H (_MT build), pinned by the
// istream ctor (0x16b480) and ipfx (0x16b720): bp@4, state@8, LockFlg@0x34,
// x_lock@0x38. sizeof == 0x50.
// ---------------------------------------------------------------------------
class ios {
public:
    // unlock() - drop the per-object critical section iff LockFlg<0.
    void unlock() {
        if (LockFlg < 0) {
            crt_mtunlock(&x_lock);
        }
    }

protected:
    void* m_vptr;       // 0x00  ios vtable (stamped by the ctor, out of scope here)
    streambuf* bp;      // 0x04  the associated streambuf
    i32 state;          // 0x08  status bits (eofbit=1 | failbit=2 | badbit=4)
    char m_pad0c[0x28]; // 0x0c  x_tie / x_flags / x_precision / x_fill / x_width ...
    i32 LockFlg;        // 0x34
    char x_lock[24];    // 0x38  _CRT_CRITICAL_SECTION (6 dwords)
};

// ---------------------------------------------------------------------------
// istream : virtual public ios. Own layout {vbptr@0, m_4@4, x_gcount@8}; the ios
// virtual-base subobject sits at +0xc in the most-derived istream.
// ---------------------------------------------------------------------------
class istream : virtual public ios {
public:
    istream& get(char& c); // 0x16a490

    // ipfx(need) - the input-prefix lock/skip protocol (0x16b720, external ->
    // reloc-masked rel32). isfx() - the suffix: unlock the streambuf then the ios
    // (both gated by LockFlg<0, so bp->unlock() reuses streambuf::unlock inline).
    i32 ipfx(i32 need);
    void isfx() {
        bp->unlock();
        unlock();
    }

protected:
    i32 m_4;      // 0x04
    i32 x_gcount; // 0x08  chars extracted by the last unformatted input
};

#endif // GRUNTZ_CRT_IOSTREAM_H
