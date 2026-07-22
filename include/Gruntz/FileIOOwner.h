// FileIOOwner.h - FileIOOwner, a per-transition file/level loader context (ctor
// 0x8fea0), built as a STACK-LOCAL inside CGruntzMgr::ChangeState.
//
// @identity-TODO: its RTTI class name is unrecovered (Ghidra placeholder). VERIFIED
// not a CGruntzMgr method and not CFile: the caller constructs it as a stack
// temporary and it CONTAINS a destructible CFile at +0x124 and a CDWordArray at
// +0x138. Defined in src/Gruntz/UnknownFileIOCtor.cpp.
//
// The member views are the real engine classes (no local shadow); only offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_FILEIOOWNER_H
#define GRUNTZ_FILEIOOWNER_H

#include <rva.h>
#include <Io/FileStream.h> // real CFile (also pulls <Mfc.h> -> CDWordArray)

struct FileIOOwner {
    FileIOOwner();
    i32 m_0;
    i32 m_4;
    i32 m_8;
    char m_padc[0x124 - 0xc];
    CFile m_124; // +0x124 (destructible -> the single /GX EH state)
    i32 m_134;     // +0x134
    // ::CDWordArray - its ctor 0x1b4b43 stamps ??_7CDWordArray@@6B@ (0x1ec29c);
    // CByteArray's ctor is 0x1b527e (vtable 0x1ed28c) and retail never calls it here.
    CDWordArray m_138; // +0x138 (constructed last; no cleanup emitted here)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_FILEIOOWNER_H
