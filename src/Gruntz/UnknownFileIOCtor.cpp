// FileIOOwner: the constructor (0x8fea0) of a large STACK-LOCAL loader-context
// object built inside CGruntzMgr::ChangeState_8fab0 (0x8fab0). VERIFIED not a
// CGruntzMgr method and not CFileIO: the caller constructs it as a stack temporary
// (`lea ecx,[esp+0x558]; call 0x8fea0`), and the object CONTAINS a destructible
// CFileIO at +0x124 and a CByteArray at +0x138, zeros four scalars, and seeds the
// CRT rng (srand(time(0))). ChangeState destructs it (0x8fd94) in its EH cleanup.
// So it is a distinct per-transition file/level loader context whose RTTI class name
// is unrecovered (Ghidra placeholder "FileIOOwner"); it stays in its own TU (the
// earlier CFileIO/Io/CGruntzMgr hypotheses are all refuted by the above evidence).
//
// The destructible CFileIO forces the /GX EH frame (one EH state). Modeled as the
// ctor it is on the placeholder owner class; the member ctors/dtor + CRT calls
// reloc-mask.
#include <rva.h>

#include <stdlib.h> // srand (0x11fed0)
#include <time.h>   // time (0x120210)

struct UfoFileIO {
    UfoFileIO();  // 0x1befd7
    ~UfoFileIO(); // destructible -> /GX frame
    char m_pad[0x10];
};
struct UfoByteArray {
    UfoByteArray(); // 0x1b4b43 (CByteArray; trivial dtor in this frame)
    char m_pad[0x10];
};

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
