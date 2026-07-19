// ButeSectionCtor.cpp - CButeMgr::CButeMgr (0x170210), the config-manager's default
// constructor (its own 1-fn TU; the linker placed this COMDAT in the 0x170210 slot,
// apart from butemgr's 0x21xxx band).
//
// An 8-state /GX EH constructor: it default-constructs the error CString (+0x10),
// the three embedded CBSecStream config-tree sub-trees (+0x18/+0x48/+0x74; each
// base-constructed by 0x16dff0 - zPTree::zPTree(teardown, n) - then re-stamped with
// CBSecStream's two most-derived vftables: @0x5f0510 at +0x00, the +0x08 sub-object
// vftable @0x5f0514), the three trailing CStrings (+0x100/+0x104/+0x108) and the
// +0x10f tail object (0x16f680), zeroes the scalar fields, then Empty()s the last
// two strings (+0x108 then +0x100). Returns this (implicit ctor return).
//
// (Ex "CButeSection::CButeSection": the CButeSection twin class is DISSOLVED - the
// 0x113-byte layout it modeled is CButeMgr's own, member for member; ~CButeMgr
// (0x213c0) destroys exactly what this ctor constructs, and the ctor's 0x1f0510
// stamps prove the three sub-trees are CBSecStream.)
//
// COLLECTION-EMBEDDING pass: the eight destructible members are REAL C++ members so
// cl emits the 8-trylevel /GX EH frame + its staged member-unwind state machine
// itself (the former manual raw-offset construction emitted no frame and matched
// only ~5%).
#include <Bute/ButeMgr.h> // the one CButeMgr (+ CBSecStream / CButeTail), shared

// The CBSecStream +0x08 second-base-in-derived vtable @0x5f0514 (cl-emitted from the
// CButeNodeEntry base). Bound here in the emitting TU (labels.py scans @data-symbol
// comments per-.cpp, not through headers).
// @data-symbol: ??_7CBSecStream@@6BCButeNodeEntry@@@ 0x001f0514 0x4
// The +0x00 PRIMARY vtable @0x1f0510: cl names it through the ultimate polymorphic
// base (zErrHandling), NOT the simple ??_7CBSecStream@@6B@ that VTBL() emits, so the
// ctor's vptr-store reloc needs the through-base name. Same datum as CBSecStream's
// VTBL (its own vtable); the through-base name sorts last and wins the per-rva dedup.
// @data-symbol: ??_7CBSecStream@@6BCContainerErr@@@ 0x001f0510

// CButeTail's ctor (0x16f680, a 3-byte `mov eax,ecx; ret` - empty __thiscall ctor
// returning `this`) lives in a foreign .text band (<this obj's 0x170210), an orphan
// COMDAT reconstructed in its own 1-fn TU src/Bute/BSecObj10fCtor.cpp; the CALL here
// binds through that definition's RVA.

RVA(0x00170210, 0x118)
CButeMgr::CButeMgr() {
    m_streamBase = 0;
    m_errCallback = 0;
    m_lineNo = 0;
    m_countLine = 1;
    m_captureText = 0;
    m_writeMode = 0;
    m_10e = 0;
    m_0d = 0;
    m_str108.Empty();
    m_tagName.Empty();
}
