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
#include <Gruntz/FileIOOwner.h> // canonical FileIOOwner (identity @identity-TODO)
#include <rva.h>

#include <stdlib.h> // srand (0x11fed0)
#include <time.h>   // time (0x120210)

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
