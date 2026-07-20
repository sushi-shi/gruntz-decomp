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

// The parse-source entry format tag: the first dword of the keyed-store entry (what
// GetEntryTag() returns) - a packed 3-char fourcc spelled LOW BYTE FIRST, so the
// literal is the file extension reversed ("VAW" -> .WAV). Only the arms proven by a
// live compare in the tree are listed; do not extend by guessing.
//
// Values only: GetEntryTag()'s return type stays i32, so its mangled name - and the
// RVA(0x00139800) binding - is untouched. Enumerators lower to the same immediates.
//
// NOTE: CImage.cpp carries a sibling `ImageFormatTag` (IMGTAG_PMB/XCP/DIR/DIP) over the
// SAME GetEntryTag() dword - the image arms of this one family. The two want merging
// into this enum; left alone here only to avoid churning a file another lane owns.
typedef enum ParseEntryTag {
    PARSETAG_VAW = 0x574156, // "VAW" -> .WAV  sound entry (CDDrawSubMgrLeafScan::ScanTree gate)
    PARSETAG_INA = 0x414e49, // "INA" -> .ANI  animation entry (the sibling Leaf/SurfacePair gate)
} ParseEntryTag;

#include <Bute/Hash.h> // the REAL CHashElement (the +0x1c node's base)

// The +0x10 mapped-source object: m_baseOffset is its base file offset, m_mapping the
// live mapping pointer (0 when not mapped). External shape, accessed by offset.
// (ParseMappedSource is GONE - the "mapped source" at +0x10 IS the owning CSymTab
// scope: its +0x0c "baseOffset" is CSymTab::m_baseOffset and its +0x48 "mapping"
// is CSymTab::m_mappedBuf, same 0-gate. One +0x10 field, one pointee.)
class CSymTab; // <Bute/SymTab.h>

// (ParseVReader is GONE - the +0x34 "virtual reader" IS CRezItmBase (<Rez/RezMgr.h>):
// same slot [2] (+0x08) Read(i32,i32,u32,void*), and the value stored there is the
// parser's active rez node. The old shim faked the dispatch with a vptr pad.)
class CRezItmBase;

// The intrusive hash-node prefix embedded at CParseSource+0x1c: the parse-slot table
// hashes on this node. IDENTITY PROVEN (VW2 2026-07-16) - it IS a CHashElement
// (<Bute/Hash.h>) wearing the concrete parse-slot key-hash vtable @0x5ef740; the
// deferred layout check HOLDS. Three independent proofs:
//
//  1. SLOT COUNT. The retail vtable @0x1ef740 has exactly ONE slot (vtable_scan:
//     0x1ef744 starts the next vtable) == CHashElement's single virtual Hash().
//     The old view declared TWO virtuals - the second slot was FABRICATED, and cl
//     was emitting an 8-byte ??_7 where retail's is 4.
//  2. THE SLOT BODY. Slot 0 -> 0x13c230, a Ghidra recovery gap; its raw bytes are
//         mov eax,[ecx+0x14]  ; this->m_record   <- CHashElement::m_record @+0x14
//         mov ecx,[ecx+0x0c]  ; this->m_owner    <- CHashElement::m_owner  @+0x0c
//         mov edx,[eax]       ; m_record's FIRST dword == the key ("key first")
//         push edx
//         call 0x13c240       ; CHash::HashStr(key), __thiscall on m_owner
//         ret
//     i.e. literally CHashElement::Hash() == m_owner->HashStr(m_record->key). The
//     +0x14 / +0x0c offsets are read straight out of the binary.
//  3. THE +0x30 FIELD. Element @+0x1c => m_record lands at +0x1c+0x14 == +0x30,
//     which is the field Init (0x1396f0) zeroes then sets to `this` - and
//     CParseSource+0x00 is m_name, the key. So the ex "m_selfLink self back-pointer"
//     IS m_node1c.m_record: the element points at its own record, key first, exactly
//     as CHashElement documents. The old view's `i32 m_data[4]` (+0x04..+0x13) is
//     CHashElement's m_link/m_owner/m_bucket, dword for dword.
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
SIZE(CParseSlotHashNode, 0x18);
VTBL(CParseSlotHashNode, 0x001ef740);

// struct (not class): the retail Init return mangles PAU (?Init@CParseSource@@QAEPAU1@XZ),
// i.e. CParseSource is a struct, so the SymParser call site pairs by name.
struct CParseSource {
    // Ghidra placeholder-named these two "CParseSource::BeginParse/EndParse"
    // (0x139960 / 0x1399d0); same 0x139xxx class + identical layout as SetPos/Read.
    // 0x139800: return the first dword of the keyed-store entry (*(int*)m_entry).
    // NON-inline (declared here, defined in ParseSource.cpp): retail keeps it a
    // real out-of-line 6-byte function CALLED at all 8 sites (CImage::Resolve,
    // LoadImage_163e50, ...), never inlined - an inline body here would inline it.
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

// --- vtable catalog ---

#endif // GRUNTZ_CPARSESOURCE_H
