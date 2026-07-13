// FileIOOwner: the constructor (0x8fea0) of a large STACK-LOCAL loader-context
// object built inside CGruntzMgr::ChangeState_8fab0 (0x8fab0). VERIFIED not a
// CGruntzMgr method and not CFileIO: the caller constructs it as a stack temporary
// (`lea ecx,[esp+0x558]; call 0x8fea0`), and the object CONTAINS a destructible
// CFileIO at +0x124 and a CByteArray at +0x138, zeros four scalars, and seeds the
// CRT rng (srand(time(0))). ChangeState destructs it (0x8fd94) in its EH cleanup.
// So it is a distinct per-transition file/level loader context whose RTTI class name
// is unrecovered (Ghidra placeholder "FileIOOwner") - an IDENTITY-RECOVERY TODO, not
// a keep. It stays in its own TU (the earlier CFileIO/Io/CGruntzMgr hypotheses are
// all refuted by the above evidence).
//
// The member views are the real engine classes (no local shadow): the destructible
// CFileIO (<Io/FileStream.h>, ctor 0x1befd7) forces the /GX EH frame (one EH state);
// the CByteArray (<Mfc.h>, ctor 0x1b4b43) is constructed last so its cleanup is not
// needed in this frame. Both member ctors/the dtor + the CRT calls reloc-mask.
#include <Io/FileStream.h> // real CFileIO (also pulls <Mfc.h> -> CByteArray)
#include <rva.h>

#include <stdlib.h> // srand (0x11fed0)
#include <time.h>   // time (0x120210)

struct FileIOOwner {
    FileIOOwner();
    i32 m_0;
    i32 m_4;
    i32 m_8;
    char m_padc[0x124 - 0xc];
    CFileIO m_124; // +0x124 (destructible -> the single /GX EH state)
    i32 m_134;     // +0x134
    // ::CDWordArray - its ctor 0x1b4b43 stamps ??_7CDWordArray@@6B@ (0x1ec29c);
    // CByteArray's ctor is 0x1b527e (vtable 0x1ed28c) and retail never calls it here.
    CDWordArray m_138; // +0x138 (constructed last; no cleanup emitted here)
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
SIZE_UNKNOWN(FileIOOwner);
