// CButeSectionCtor.cpp - the constructor of a Bute "section" object (0x170210).
//
// An 8-state /GX EH constructor: it default-constructs a leading CString (+0x10),
// three embedded Bute streams (+0x18/+0x48/+0x74, each base-constructed by
// 0x16dff0 then stamped with its derived vtable @0x5f0510 + the +0x08 data pointer
// @0x5f0514), three trailing CStrings (+0x100/+0x104/+0x108) and a small +0x10f
// sub-object (0x16f680), zeroes the scalar fields, then runs a finaliser on the
// last string + the +0x10f object.  Returns this.
//
// Field names are placeholders; only the OFFSETS, the construction order and the
// stamped vtable/data pointers are load-bearing.  All sub-object ctors/leaves are
// external / reloc-masked.
// @early-stop
// 8-state /GX ctor-in-flight EH frame: every sub-object ctor call, vtable stamp
// and field store is byte-faithful, but the manual (raw-offset) construction emits
// no per-member EH-state unwind frame, so retail's 8-trylevel frame + its staged
// epilogues are the residual.  The final-sweep upgrade is to model the embedded
// streams + CStrings as real destructible members so cl emits the frame itself.
#include <rva.h>

// The stream base ctor argument tag (a .text descriptor @0x574de0).
DATA(0x00174de0)
extern u8 g_streamTag; // 0x574de0

// The embedded streams' derived vtable + its +0x08 data pointer (reloc-masked).
DATA(0x001f0510)
extern void* g_streamVtbl; // 0x5f0510  (scalar-deleting dtor table)
DATA(0x001f0514)
extern void* g_streamData; // 0x5f0514

// Sub-object ctors / leaves (all __thiscall, no body -> rel32 reloc-masks).
struct CBSecString {
    void Ctor();     // 0x1b9b93  default CString ctor
    void Finalize(); // 0x1b9c69  (Empty / free-data finaliser)
};
struct CBSecStream {
    void Ctor(i32 n, void* tag); // 0x16dff0  base stream ctor
};
struct CBSecObj10f {
    void Ctor();     // 0x16f680
    void Finalize(); // 0x1b9c69
};

class CButeSection {
public:
    CButeSection* Construct(); // 0x170210
};

RVA(0x00170210, 0x118)
CButeSection* CButeSection::Construct() {
    char* self = (char*)this;
    ((CBSecString*)(self + 0x10))->Ctor();

    ((CBSecStream*)(self + 0x18))->Ctor(2, &g_streamTag);
    *(void**)(self + 0x18) = &g_streamVtbl;
    *(void**)(self + 0x20) = &g_streamData;

    ((CBSecStream*)(self + 0x48))->Ctor(2, &g_streamTag);
    *(void**)(self + 0x48) = &g_streamVtbl;
    *(void**)(self + 0x50) = &g_streamData;

    ((CBSecStream*)(self + 0x74))->Ctor(2, &g_streamTag);
    *(void**)(self + 0x74) = &g_streamVtbl;
    *(void**)(self + 0x7c) = &g_streamData;

    ((CBSecString*)(self + 0x100))->Ctor();
    ((CBSecString*)(self + 0x104))->Ctor();
    ((CBSecString*)(self + 0x108))->Ctor();
    ((CBSecObj10f*)(self + 0x10f))->Ctor();

    *(i32*)(self + 0x00) = 0;
    *(i32*)(self + 0x14) = 0;
    *(i32*)(self + 0x08) = 0;
    *(u8*)(self + 0x0c) = 1;
    *(u8*)(self + 0x10c) = 0;
    *(u8*)(self + 0x10d) = 0;
    *(u8*)(self + 0x10e) = 0;
    *(u8*)(self + 0x0d) = 0;

    ((CBSecString*)(self + 0x108))->Finalize();
    ((CBSecObj10f*)(self + 0x10f))->Finalize();
    return this;
}
