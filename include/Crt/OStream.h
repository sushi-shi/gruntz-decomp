// OStream.h - the MSVC 5.0 pre-standard iostreams `ios` / `ostream` (statically
// linked from LIBCMT.LIB, built with _MT). These are LIBRARY classes, not engine
// code; reconstructed because they occupy real `.text` bytes in retail GRUNTZ.EXE.
// Layout is taken verbatim from the VC5 headers IOS.H / OSTREAM.H (the _MT build,
// so ios carries LockFlg + a 24-byte _CRT_CRITICAL_SECTION). The placeholder tracer
// keyed this cluster off the malloc in the ostream ctor (0x16bfa0).
//
// `ostream : virtual public ios` is modeled with the REAL virtual base so the
// member-access codegen (`mov eax,[this]; mov eax,[eax+4]; add eax,this`, the
// vbtable indirection) falls out of cl. The ios/ostream vtables point at slots
// outside this cluster (the deleting dtor thunk, do_opfx, ...), so the classes
// stay non-polymorphic with an explicit `m_vptr` member; the ctor that stamps the
// retail vftable (0x16bfa0) lives in src/Stub. The bp (streambuf) virtual calls
// are written through the SbView cast (see StrStream.h) to match `call [reg+slot]`.
#ifndef GRUNTZ_CRT_OSTREAM_H
#define GRUNTZ_CRT_OSTREAM_H

#include <Crt/StrStream.h>
#include <rva.h>

// _flush(FILE*) - the C-stdio flush the unitbuf/stdio sync path drives, plus the
// stdout/stderr FILE handles it flushes (real .data globals; reloc-masked pins).
extern "C" i32 __cdecl crt_flush(void* stream); // 0x125b50
DATA(0x00618ca8)
extern char g_iob_stdout[]; // _iob[1]
DATA(0x00618cc8)
extern char g_iob_stderr[]; // _iob[2]

// The shared empty string the char-insertion path pads against (a real .data
// global, reloc-masked DATA() pin).
DATA(0x006293f4)
extern char g_emptyString[];

// ---------------------------------------------------------------------------
// ios - the iostream state base. Field layout from IOS.H (_MT build). sizeof
// 0x54 (vptr..x_width = 0x34, LockFlg 0x34, x_lock 24 bytes -> 0x4c, +pad 0x54).
// ---------------------------------------------------------------------------
class ios {
public:
    enum io_state {
        goodbit = 0x00,
        eofbit = 0x01,
        failbit = 0x02,
        badbit = 0x04
    };
    enum {
        skipws = 0x0001,
        unitbuf = 0x2000,
        stdio = 0x4000
    };

    // ios::lock / unlock - take the per-object critical section iff LockFlg<0.
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

    // --- layout (offsets are load-bearing) ---
    void* m_vptr;     // 0x00 vptr
    streambuf* bp;    // 0x04
    i32 state;        // 0x08
    i32 ispecial;     // 0x0c
    i32 ospecial;     // 0x10
    i32 isfx_special; // 0x14
    i32 osfx_special; // 0x18
    i32 x_delbuf;     // 0x1c
    void* x_tie;      // 0x20 ostream*
    i32 x_flags;      // 0x24
    i32 x_precision;  // 0x28
    char x_fill;      // 0x2c
    i32 x_width;      // 0x30
    i32 LockFlg;      // 0x34
    char x_lock[24];  // 0x38 - _CRT_CRITICAL_SECTION
};

// ---------------------------------------------------------------------------
// ostream : virtual public ios. Own data: x_floatused at +0x04 (after the
// vbptr). The virtual base ios sits at object +0x08.
// ---------------------------------------------------------------------------
class ostream : virtual public ios {
public:
    i32 opfx();
    void osfx();
    ostream& flush(); // 0x16be90 (reconstructed elsewhere; extern here)
    ostream& write(const char* s, i32 n);
    ostream& operator<<(u8 c);

private:
    ostream& writepad(const char* a, const char* b); // 0x16c2d0 (extern)

    i32 x_floatused; // 0x04
};

#endif // GRUNTZ_CRT_OSTREAM_H
