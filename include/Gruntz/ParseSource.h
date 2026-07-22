#ifndef GRUNTZ_CPARSESOURCE_H
#define GRUNTZ_CPARSESOURCE_H

#include <Ints.h>
#include <rva.h>

typedef enum ParseEntryTag {
    PARSETAG_VAW = 0x574156, // "VAW" -> .WAV  sound entry (CDDrawSubMgrLeafScan::ScanTree gate)
    PARSETAG_INA = 0x414e49, // "INA" -> .ANI  animation entry (the sibling Leaf/SurfacePair gate)
} ParseEntryTag;

#include <Bute/Hash.h> // the REAL CHashElement (the +0x1c node's base)

class CSymTab; // <Bute/SymTab.h>

class CRezItmBase;

struct CParseSlotHashNode : public CHashElement {
    // The ctor zeroes m_record - EVIDENCE, not a guess: Init's `mov [eax+0x30],ecx`
    // sits between the vptr stamp and Init's OWN zero run (+0x34/+0x10/+0x00), i.e.
    // exactly where an inlined member ctor lands. That also explains the "dead"
    // +0x30 store retail keeps (cl does not DSE it across the inlined ctor); the
    // ex model pinned it with a `volatile` member instead.
    CParseSlotHashNode() {
        m_record = 0;
    }
    // slot 0 (0x13c230): m_owner->HashStr(m_record's key). Declared-only - its
    // owner is the leaf-symbol CHash instantiation, and typing that here would
    // need a downcast of the CHashBase* m_owner; the slot reloc-masks.
    virtual u32 Hash() OVERRIDE;
};
SIZE(0x18);
VTBL(CParseSlotHashNode, 0x001ef740);

struct CParseSource {
    // Ghidra placeholder-named these two "CParseSource::BeginParse/EndParse"
    // (0x139960 / 0x1399d0); same 0x139xxx class + identical layout as SetPos/Read.
    // 0x139800: return the first dword of the keyed-store entry (*(int*)m_entry).
    // NON-inline (declared here, defined in ParseSource.cpp): retail keeps it a
    // real out-of-line 6-byte function CALLED at all 8 sites (CImage::Resolve,
    // LoadImage, ...), never inlined - an inline body here would inline it.
    i32 GetEntryTag();
    i32 BeginParse();
    i32 EndParse();
    // Parse-slot init (0x1396f0): stamp the embedded hash-node (m_node1c), null the
    // bookkeeping fields, self-link m_selfLink. Returns this. (CSymParser::PopParseSlot.)
    CParseSource* Init();
    // The leaf-record fill/teardown pair (0x139710/0x1397a0, bodies in SymTab.cpp with
    // the rest of this class's band; the ex-CSymLeafBuilder methods).
    void Build(CSymTab* owner, const char* name, void* f4, void* rec, void* str2, i32 f3,
               i32 f1, void* f2, void* f6, void* arr, CRezItmBase* stream);
    void Teardown();
    i32 SetPos(i32 pos); // 0x139ae0 (out-of-line: m_cursor = pos; return 1)
    i32 ReadAt(void* dst, i32 pos, u32 len);
    i32 Read(void* dst, u32 len, i32 seekPos);

    char* m_name;                      // +0x00 source name
    void* m_entry;                     // +0x04 keyed-store entry (first dword = tag)
    void* m_typeTag;                   // +0x08  type tag (the Build f2 slot)
    u32 m_length;                      // +0x0c total byte length / limit
    CSymTab* m_owner;                  // +0x10  owning scope (Build stores it; the stream side
                                       //        reads its m_baseOffset/m_mappedBuf as the mapped window)
    i32 m_base;                        // +0x14 source base ptr
    i32 m_cursor;                      // +0x18 read cursor
    // +0x1c embedded parse-slot hash-node (0x18 B, vptr @0x5ef740, spans +0x1c..+0x33).
    // Its m_record (+0x30) is the ex "m_selfLink": Init points it at this record, whose
    // first dword (m_name) is the hash key.
    CParseSlotHashNode m_node1c;
    CRezItmBase* m_reader; // +0x34  the providing rez node (slot-2 virtual Read)
    i32 m_buffer;                      // +0x38 lazily-allocated inline byte buffer (as int address)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CPARSESOURCE_H
