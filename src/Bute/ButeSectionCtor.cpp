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
#include <Bute/ButeSection.h> // the CButeSection class (+ CBSecStream / CBSecObj10f), shared

// The CBSecStream +0x08 second-base-in-derived vtable @0x5f0514 (cl-emitted from the
// CButeNodeEntry base). Bound here in the emitting TU (labels.py scans @data-symbol
// comments per-.cpp, not through headers).
// @data-symbol: ??_7CBSecStream@@6BCButeNodeEntry@@@ 0x001f0514 0x4

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
