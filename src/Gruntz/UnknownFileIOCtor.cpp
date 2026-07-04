// FileIOOwner: a fresh-object constructor (0x8fea0) mis-attributed to
// CGruntzMgr::InitCFileIOMember by the call-xref heuristic. It is NOT a CGruntzMgr
// method: it constructs a destructible CFileIO at +0x124 and a CByteArray at +0x138,
// zeros four scalars, and seeds the CRT rng - which contradicts the CGruntzMgr ctor's
// int store at +0x138 and overwrites +0x00 (a vptr). Its true owner class is not yet
// identified, so it lives in its own TU (like GruntzMgrHelper) rather than
// the wrong class's file. Re-homed from src/Stub/CGruntzMgr.cpp.
//
// The destructible CFileIO forces the /GX EH frame (one EH state). Modeled as the
// ctor it is on a placeholder owner class; the member ctors/dtor + CRT calls
// reloc-mask.
#include <rva.h>

struct UfoFileIO {
    UfoFileIO();  // 0x1befd7
    ~UfoFileIO(); // destructible -> /GX frame
    char m_pad[0x10];
};
struct UfoByteArray {
    UfoByteArray(); // 0x1b4b43 (CByteArray; trivial dtor in this frame)
    char m_pad[0x10];
};
extern "C" void srand(unsigned seed); // 0x11fed0
extern "C" long time(long* t);        // 0x120210

struct FileIOOwner {
    FileIOOwner();
    i32 m_0;
    i32 m_4;
    i32 m_8;
    char m_padc[0x124 - 0xc];
    UfoFileIO m_124;    // +0x124 (destructible)
    i32 m_134;          // +0x134
    UfoByteArray m_138; // +0x138
};

// @confidence: med
// @source: call-xref
RVA(0x0008fea0, 0x6d)
FileIOOwner::FileIOOwner() {
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_134 = 0;
    srand(time(0));
}
SIZE_UNKNOWN(UfoByteArray);
SIZE_UNKNOWN(UfoFileIO);
SIZE_UNKNOWN(FileIOOwner);
