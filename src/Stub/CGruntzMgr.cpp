#include <rva.h>
// CGruntzMgr.cpp - the remaining engine-label stubs for CGruntzMgr. The class and
// most of its methods are reconstructed in src/Gruntz/GruntzMgr.cpp (CGruntzMgr :
// public WAP32::CGameMgr, 0xa30); the ctor, ~CGruntzMgr, the ??_G scalar-deleting
// destructor, UnknownClose, AccrueScoreTime (0x861e0), OnCheckpointReached (0x8e6c0),
// DelayedQuit (0x8f530), RunModalDialog (0x90260) and LoadSaveMessageSprite (0x92420)
// now live there.
//
// 0x092f00 (CGruntzMgr::SaveGameAs, the save-as name dialog) is reconstructed in
// src/Gruntz/GruntzMgr.cpp (the dialog + its CString member are the /GX frame's two
// destructibles).
//
// 0x08fea0 is NOT a CGruntzMgr method at all: it constructs a fresh object (CFileIO
// at +0x124, CByteArray at +0x138, srand(time(0))), which contradicts the CGruntzMgr
// ctor's int store at +0x138 and overwrites +0x00 (a vptr) - a mis-attributed label
// kept here pending its true owner class.

// 0x8fea0 is a fresh-object CONSTRUCTOR (mis-attributed CGruntzMgr::InitCFileIOMember
// by the call-xref heuristic): it constructs a destructible CFileIO at +0x124 and a
// CByteArray at +0x138, zeros four scalars, and seeds the CRT rng. The destructible
// CFileIO forces the /GX EH frame (one EH state). Modeled as the ctor it is on a
// placeholder owner class; the member ctors/dtor + CRT calls reloc-mask.
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

struct UnknownFileIOOwner {
    UnknownFileIOOwner();
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
UnknownFileIOOwner::UnknownFileIOOwner() {
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_134 = 0;
    srand(time(0));
}
