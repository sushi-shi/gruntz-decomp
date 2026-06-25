// DataBuffer.h - a small RezAlloc-backed serialized buffer holder
// (tracer placeholder ClassUnknown_4). Non-polymorphic (no vtable / no RTTI),
// 16 bytes. Reconstructed from the 0x150180..0x1503ed cluster.
//
// The class owns a heap blob (RezAlloc'd, RezFree'd) with a "valid" gate, a
// byte/element count and an id. A sibling reader (FUN_005501f0 / FUN_00550250)
// pulls the count + the bytes out of a CFile-like reader (vtable slot +0x3c)
// into this buffer. The placeholder name CDataBuffer reflects that shape; the
// real RTTI name is unknown (no vtable -> no class descriptor).
//
// Layout (offsets + code bytes are load-bearing; field names are placeholders):
//   +0x00 m_00 - i32  "valid"/loaded flag (Set -> 1, Clear -> 0; gates Free)
//   +0x04 m_04 - u32  size (the byte count handed to RezAlloc)
//   +0x08 m_08 - the RezAlloc'd data blob (RezFree'd on Set/Free)
//   +0x0c m_0c - i32  id (Set arg2; NOT cleared by the ctor)
#ifndef GRUNTZ_DATABUFFER_H
#define GRUNTZ_DATABUFFER_H

#include <Mfc.h> // CFile / CMemFile (the readers slurp through MFC files)

#include <Ints.h>

class CDataBuffer {
public:
    CDataBuffer();             // 0x150180
    void Reset();              // 0x150190 (if m_00 -> Free)
    i32 Set(u32 size, i32 id); // 0x1501a0
    void Free();               // 0x1503c0

    // The serialized-buffer readers. ReadFrom pulls a 4-byte count then that many
    // bytes out of a CFile (vtable slot +0x3c = CFile::Read); LoadFromFile /
    // LoadFromMem wrap it around a local CFile / CMemFile (both /GX EH frames).
    i32 ReadFrom(CFile* file, i32 id);          // 0x1501f0
    i32 LoadFromFile(const char* path, i32 id); // 0x150250
    i32 LoadFromMem(void* buf, u32 len, i32 id); // 0x150330

    i32 m_00;
    u32 m_04;
    void* m_08;
    i32 m_0c;
};

#endif // GRUNTZ_DATABUFFER_H
