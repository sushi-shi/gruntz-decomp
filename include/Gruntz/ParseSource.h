// ParseSource.h - a positioned byte-reader over a ButeMgr parse source (the
// 0x139xxx stream family; trace placeholder tomalla-85). It serves bytes
// from one of three backing stores, in priority order:
//   - a memory-mapped source object at m_mapped (when its m_mapping is live),
//   - an inline byte buffer at m_buffer,
//   - else a virtual reader object at m_reader (vtable slot 2 = Read).
// m_cursor is the read cursor; m_length is the byte limit. m_base is the source base ptr.
// Non-polymorphic for these two methods; names are placeholders, offsets + code
// bytes are load-bearing. The sibling ctor/clear/peek methods (0x139710 /
// 0x1397a0 / 0x139a40) live in other TUs (still trace stubs).
#ifndef GRUNTZ_CPARSESOURCE_H
#define GRUNTZ_CPARSESOURCE_H

#include <Ints.h>
#include <rva.h>

// The +0x10 mapped-source object: m_baseOffset is its base file offset, m_mapping the
// live mapping pointer (0 when not mapped). External shape, accessed by offset.
struct ParseMappedSource {
    char pad_00[0x0c];
    i32 m_baseOffset; // +0x0c base file offset
    char pad_10[0x48 - 0x10];
    i32 m_mapping; // +0x48 mapping base (0 = inactive)
};

// The +0x34 virtual reader: its vtable slot 2 (+0x08) is Read(base, pos, len,
// dst) -> bytes read. Modeled as a polymorphic class so the `mov edx,[ecx];
// call [edx+8]` thiscall dispatch falls out with no cast.
class ParseVReader {
public:
    char _vft0[4]; // +0x00 foreign/base object vptr (reduced view; not owned/dispatched)
    virtual void VSlot1();
    virtual i32 Read(i32 base, i32 pos, u32 len, void* dst);
};

class CParseSource {
public:
    // Ghidra placeholder-named these two "CParseSource::BeginParse/EndParse"
    // (0x139960 / 0x1399d0); same 0x139xxx class + identical layout as SetPos/Read.
    // 0x139800: return the first dword of the keyed-store entry (*(int*)m_entry).
    RVA(0x00139800, 0x6)
    i32 GetEntryTag() {
        return *(i32*)m_entry;
    }
    i32 BeginParse();
    i32 EndParse();
    RVA(0x00139ae0, 0xf)
    i32 SetPos(i32 pos) {
        m_cursor = pos;
        return 1;
    }
    i32 ReadAt(void* dst, i32 pos, u32 len);
    i32 Read(void* dst, u32 len, i32 seekPos);

    char* m_name;                // +0x00 source name
    void* m_entry;               // +0x04 keyed-store entry (first dword = tag)
    i32 m_08;                    // +0x08 (role unproven)
    u32 m_length;                // +0x0c total byte length / limit
    ParseMappedSource* m_mapped; // +0x10 mapped source
    i32 m_base;                  // +0x14 source base ptr
    i32 m_cursor;                // +0x18 read cursor
    char pad_1c[0x34 - 0x1c];
    ParseVReader* m_reader; // +0x34 virtual reader
    i32 m_buffer;           // +0x38 lazily-allocated inline byte buffer (as int address)
};

// --- vtable catalog ---

#endif // GRUNTZ_CPARSESOURCE_H
