// ButeSection.h - CButeSection, the Bute "section" config-object: a config-tree
// section holding a leading CString (+0x10), three embedded CButeNode-family config-
// tree streams (+0x18/+0x48/+0x74), three trailing CStrings (+0x100/+0x104/+0x108)
// and a small +0x10f sub-object. Its 8-state /GX EH constructor (0x170210) is
// reconstructed in src/Bute/ButeSectionCtor.cpp.
//
// The global resource-config manager g_resButeMgr (?g_resButeMgr@@3UResButeMgr@@A
// @0x6453d8) IS one such section: its compiler-generated dynamic initializer (the
// 0x82b20 _$E thunk, homed in GameText.cpp) tail-constructs it in place.
//
// Field names are placeholders; only the OFFSETS + construction order + the stamped
// vtable/data pointers are load-bearing (campaign doctrine).
#ifndef SRC_BUTE_BUTESECTION_H
#define SRC_BUTE_BUTESECTION_H

#include <rva.h>
#include <Mfc.h> // real CString members (ctor 0x1b9b93 / Empty 0x1b9c69 / ~CString 0x1b9cde)
#include <Ints.h>
#include <Globals.h>        // g_streamTag (0x574de0)
#include <Bute/PTreeNode.h> // the shared zPTree config-tree node family

// The embedded CButeNode-family config-tree node (+0x18/+0x48/+0x74). REAL
// POLYMORPHIC (ALL-VTABLES): a zPTree-derived node (the SAME family + base ctor
// 0x16dff0 as CButeCfgNode174d, src/Bute/ButeNode.cpp), so cl auto-stamps its two
// most-derived vftables (primary @0x5f0510 at +0x00, second-base-in-derived
// @0x5f0514 at +0x08) in the ctor. External non-trivial dtor so the enclosing
// CButeSection ctor's /GX frame tracks it as a destructible member.
struct CBSecStream : zPTree {
    CBSecStream() : zPTree(&g_streamTag, 2) {}
    virtual ~CBSecStream() OVERRIDE; // slot 0 zPTree dtor; external (reloc-masked /GX unwind)
};
VTBL(CBSecStream, 0x001f0510); // node primary (most-derived) vtable @+0x00
// The +0x08 second-base-in-derived vtable @0x5f0514 (cl-emitted from the CButeNodeEntry
// base); bind the retail datum to that real symbol.
// @data-symbol: ??_7CBSecStream@@6BCButeNodeEntry@@@ 0x001f0514 0x4

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

SIZE_UNKNOWN(CBSecObj10f);
SIZE_UNKNOWN(CBSecStream);
SIZE_UNKNOWN(CButeSection);

#endif // SRC_BUTE_BUTESECTION_H
