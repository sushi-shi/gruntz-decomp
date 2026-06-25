// IStream.h - the MSVC 5.0 pre-standard iostreams `istream` (statically linked
// from LIBCMT.LIB, built with _MT). These are LIBRARY classes, not engine code;
// reconstructed because they occupy real `.text` bytes in retail GRUNTZ.EXE.
// Layout is verbatim from the VC5 header ISTREAM.H: `istream : virtual public ios`,
// own data `_fGline`/`x_gcount` after the vbptr, so the ios virtual base sits at
// object +0x0c (vbtable entry {0, 0x0c}).
//
// `istream : virtual public ios` is modeled with the REAL virtual base so the
// member-access codegen (`mov eax,[this]; mov eax,[eax+4]; add eax,this`, the
// vbtable indirection) falls out of cl. The istream/ios vtables point at slots
// outside this cluster (deleting-dtor thunk, do_ipfx, ...), so the class stays
// non-polymorphic; the ctor that stamps the retail vftable (0x16b510) carries the
// /GX frame and lives elsewhere. The bp (streambuf) virtual calls are written
// through the SbView cast (see StrStream.h) so `call [reg+slot]` matches retail.
#ifndef GRUNTZ_CRT_ISTREAM_H
#define GRUNTZ_CRT_ISTREAM_H

#include <Crt/OStream.h>
#include <Crt/StrStream.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// istream : virtual public ios. Own data after the vbptr: _fGline (0x04),
// x_gcount (0x08); the virtual base ios sits at object +0x0c.
// ---------------------------------------------------------------------------
class istream : virtual public ios {
public:
    i32 ipfx(i32 need = 0);
    void isfx() {
        bp->unlock(); // unlockbuf
        unlock();
    }

    i32 get();
    istream& read(char* s, i32 n);

    void eatwhite(); // 0x16bb10 (extern; reloc-masked)

private:
    i32 _fGline;  // 0x04
    i32 x_gcount; // 0x08
};

#endif // GRUNTZ_CRT_ISTREAM_H
