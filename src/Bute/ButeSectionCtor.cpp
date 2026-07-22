#include <Bute/ButeMgr.h> // the one CButeMgr (+ CBSecStream / CButeTail), shared
#include <rva.h>

VTBL(CBSecStream, 0x001f0510); // node primary (most-derived) vtable @+0x00
// The CBSecStream +0x08 second-base-in-derived vtable @0x5f0514 (cl-emitted from the
// CButeNodeEntry base). Bound here in the emitting TU (labels.py scans DATA_SYMBOL
// comments per-.cpp, not through headers).
VTBL2(CBSecStream, CButeNodeEntry, 0x001f0514)
// The +0x00 PRIMARY vtable @0x1f0510: cl names it through the ultimate polymorphic
// base (zErrHandling), NOT the simple ??_7CBSecStream@@6B@ that VTBL() emits, so the
// ctor's vptr-store reloc needs the through-base name. Same datum as CBSecStream's
// VTBL (its own vtable); the through-base name sorts last and wins the per-rva dedup.
VTBL2(CBSecStream, CContainerErr, 0x001f0510)

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
