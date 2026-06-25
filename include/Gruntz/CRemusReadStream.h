// CRemusReadStream.h - a positioned byte-reader over a Remus parse source (the
// 0x139xxx stream family; trace placeholder ClassUnknown_85). It serves bytes
// from one of three backing stores, in priority order:
//   - a memory-mapped source object at +0x10 (when its m_48 mapping is live),
//   - an inline byte buffer at +0x38,
//   - else a virtual reader object at +0x34 (vtable slot 2 = Read).
// m_18 is the read cursor; m_0c is the byte limit. m_14 is the source base ptr.
// Non-polymorphic for these two methods; names are placeholders, offsets + code
// bytes are load-bearing. The sibling ctor/clear/peek methods (0x139710 /
// 0x1397a0 / 0x139a40) live in other TUs (still trace stubs).
#ifndef GRUNTZ_CREMUSREADSTREAM_H
#define GRUNTZ_CREMUSREADSTREAM_H

#include <Ints.h>
#include <rva.h>

// The +0x10 mapped-source object: m_0c is its base file offset, m_48 the live
// mapping pointer (0 when not mapped). External shape, accessed by offset.
struct RemusMappedSource {
    char pad_00[0x0c];
    i32 m_0c;  // +0x0c base offset
    char pad_10[0x48 - 0x10];
    i32 m_48;  // +0x48 mapping base (0 = inactive)
};

// The +0x34 virtual reader: its vtable slot 2 (+0x08) is Read(base, pos, len,
// dst) -> bytes read. Modeled as a polymorphic class so the `mov edx,[ecx];
// call [edx+8]` thiscall dispatch falls out with no cast.
class RemusVReader {
public:
    virtual void VSlot0();
    virtual void VSlot1();
    virtual i32 Read(i32 base, i32 pos, u32 len, void* dst);
};

class CRemusReadStream {
public:
    // Ghidra placeholder-named these two "RemusParseSource::BeginParse/EndParse"
    // (0x139960 / 0x1399d0); same 0x139xxx class + identical layout as SetPos/Read.
    i32 BeginParse();
    i32 EndParse();
    i32 SetPos(i32 pos);
    i32 ReadAt(void* dst, i32 pos, u32 len);
    i32 Read(void* dst, u32 len, i32 seekPos);

    char* m_00;             // +0x00 source name
    void* m_04;             // +0x04 keyed-store entry
    i32 m_08;               // +0x08
    u32 m_0c;               // +0x0c byte limit
    RemusMappedSource* m_10; // +0x10 mapped source
    i32 m_14;               // +0x14 source base ptr
    i32 m_18;               // +0x18 read cursor
    char pad_1c[0x34 - 0x1c];
    RemusVReader* m_34;     // +0x34 virtual reader
    i32 m_38;               // +0x38 inline byte buffer
};

#endif // GRUNTZ_CREMUSREADSTREAM_H
