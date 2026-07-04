// ButeSectionCtor.cpp - the constructor of a Bute "section" object (0x170210).
//
// An 8-state /GX EH constructor: it default-constructs a leading CString (+0x10),
// three embedded CButeNode-family config-tree nodes (+0x18/+0x48/+0x74; each base-
// constructed by 0x16dff0 - CButeNodeBase::CButeNodeBase(tag,n) - then re-stamped
// with its two most-derived vftables: @0x5f0510 at +0x00, the +0x08 sub-object
// vftable @0x5f0514), three trailing CStrings (+0x100/+0x104/+0x108) and a small
// +0x10f sub-object (0x16f680), zeroes the scalar fields, then Empty()s the last two
// strings (+0x108 then +0x100). Returns this (implicit ctor return).
//
// Field names are placeholders; only the OFFSETS, the construction order and the
// stamped vtable/data pointers are load-bearing. All sub-object ctors/dtors are
// external / reloc-masked.
//
// COLLECTION-EMBEDDING pass: the eight destructible members (the CString +0x10, the
// three CButeNode streams, the three trailing CStrings and the +0x10f object) are
// modeled as REAL C++ members so cl emits the 8-trylevel /GX EH frame + its staged
// member-unwind state machine itself (the former manual raw-offset construction
// emitted no frame and matched only ~5%).
#include <rva.h>
#include <Mfc.h> // real CString (ctor 0x1b9b93 / Empty 0x1b9c69 / ~CString 0x1b9cde)
#include <Ints.h>
#include <Globals.h> // g_streamTag (0x574de0), g_streamData (0x5f0514)

// The node's two most-derived vftables, re-stamped over the tables the base ctor
// laid down (reloc-masked DIR32 data). g_streamData @0x5f0514 is NOT data: it is
// &g_streamVtbl+4 -- the vptr carried by the node's +0x08 sub-object (the family's
// "sub vftable", cf. g_buteNodeSubVtbl / g_buteTreeSubVtbl).
//
// These stamps target the embedded NODES' own vftables, not CButeSection's vptr
// (CButeSection is non-polymorphic: +0x00 is the zeroed scalar below). They stay
// manual stamps rather than C++ virtuals: the node lives in unmatched TUs and the
// whole zPTree family (zPTree/CButeTree/CButeNode) is deliberately manual-vtbl-
// modeled -- a real +0x00 vptr breaks its matched ctor layout (src/Bute/ButeNode.cpp).
// Foreign sub-object re-stamps stay manual.
DATA(0x001f0510)
extern void* g_streamVtbl; // 0x5f0510  node's primary vftable (4 slots)

// The embedded CButeNode-family config-tree node (+0x18/+0x48/+0x74). An inline
// ctor that base-constructs (CButeNodeBase 0x16dff0) then re-stamps its two most-
// derived vftables; an external non-trivial dtor so the enclosing ctor's /GX frame
// tracks it as a destructible member (its teardown fires only from the unwind
// funclet, reloc-masked). Size 0x2c = the +0x48..+0x74 slot; the +0x44 gap word is
// a CButeSection scalar the ctor leaves untouched.
struct CBSecStream {
    char m_pad[0x2c];
    void NodeBaseCtor(void* tag, i32 n); // 0x16dff0  CButeNodeBase base ctor
    CBSecStream() {
        NodeBaseCtor(&g_streamTag, 2);
        *(void**)this = &g_streamVtbl;
        *((void**)this + 2) = &g_streamData;
    }
    ~CBSecStream(); // external (reloc-masked in the /GX unwind funclet)
};

// The small +0x10f sub-object: an out-of-line (external) ctor 0x16f680 that just
// returns this (mov eax,ecx; ret) plus a non-trivial external dtor, so it is the 8th
// destructible member (trylevel 7).
struct CBSecObj10f {
    CBSecObj10f();  // 0x16f680  external ctor
    ~CBSecObj10f(); // external dtor
};

class CButeSection {
public:
    i32 m_00;                // +0x00  (zeroed at trylevel 7)
    i32 m_04;                // +0x04  (untouched)
    i32 m_08;                // +0x08  (zeroed)
    u8 m_0c;                 // +0x0c  (= 1)
    u8 m_0d;                 // +0x0d  (zeroed)
    u8 m_0e;                 // +0x0e  (untouched)
    u8 m_0f;                 // +0x0f  (untouched)
    CString m_10;            // +0x10  leading CString
    i32 m_14;                // +0x14  (zeroed)
    CBSecStream m_18;        // +0x18  config-tree node
    i32 m_44;                // +0x44  (untouched gap word)
    CBSecStream m_48;        // +0x48  config-tree node
    CBSecStream m_74;        // +0x74  config-tree node
    char m_a0[0x100 - 0xa0]; // +0xa0 (untouched)
    CString m_100;           // +0x100
    CString m_104;           // +0x104
    CString m_108;           // +0x108
    u8 m_10c;                // +0x10c (zeroed)
    u8 m_10d;                // +0x10d (zeroed)
    u8 m_10e;                // +0x10e (zeroed)
    CBSecObj10f m_10f;       // +0x10f
    CButeSection();
};

RVA(0x00170210, 0x118)
CButeSection::CButeSection() {
    m_00 = 0;
    m_14 = 0;
    m_08 = 0;
    m_0c = 1;
    m_10c = 0;
    m_10d = 0;
    m_10e = 0;
    m_0d = 0;
    m_108.Empty();
    m_100.Empty();
}

SIZE_UNKNOWN(CBSecObj10f);
SIZE_UNKNOWN(CBSecStream);
SIZE_UNKNOWN(CButeSection);
