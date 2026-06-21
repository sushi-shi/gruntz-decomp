#include <rva.h>
// CDDrawSubMgrLeaf.cpp - VirtualMethodUnknown14/18 of the tomalla-named class
// CDDrawSubMgrLeaf (a CDirectDrawMgr surface/page sub-manager in the "Harry
// Potter" family). VirtualMethodUnknown14 is a standard readiness predicate
// shared by several Lucius-derived managers: reports ready (1) when the
// parent/root handle at +0x0c is present and the base status word at +0x04 is
// no longer the inactive -1 sentinel. VirtualMethodUnknown18 clears the parent
// map then zeroes the handle. Plain /O2 /MT leaves: no SEH frame.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

class CDDrawMapHolder {
public:
    void ClearUnknownMap();

    // Engine-label backlog stubs.
    void VirtualMethodUnknown14();
    ~CDDrawMapHolder();
};

class CDDrawSubMgrLeaf {
public:
    int VirtualMethodUnknown14();
    void VirtualMethodUnknown18();

    // Engine-label backlog stubs.
    void VirtualMethodUnknown1C();
    ~CDDrawSubMgrLeaf();

    void* m_vptr;              // +0x00
    int m_04;                  // +0x04  -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    int m_0c;                  // +0x0c  parent/root handle
};

// ---------------------------------------------------------------------------
// Ready when the parent handle is present and the status word is not -1.
// ---------------------------------------------------------------------------
RVA(0x1577a0, 0x16)
int CDDrawSubMgrLeaf::VirtualMethodUnknown14() {
    if (m_0c == 0) {
        goto fail;
    }
    if (m_04 != -1) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// Clears the parent map then zeroes a member field.
// ---------------------------------------------------------------------------
RVA(0x157ae0, 0x11)
void CDDrawSubMgrLeaf::VirtualMethodUnknown18() {
    ((class CDDrawMapHolder*)this)->ClearUnknownMap();
    m_0c = 0;
}

// Engine-label backlog stubs (moved from src/Stub/CDDrawSubMgrLeaf.cpp).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x152650, 0x5)
void CDDrawSubMgrLeaf::VirtualMethodUnknown1C() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1577c0, 0x1e)
CDDrawSubMgrLeaf::~CDDrawSubMgrLeaf() {}

// Engine-label backlog stubs (moved from src/Stub/CDDrawMapHolder.cpp).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157530, 0x17)
void CDDrawMapHolder::VirtualMethodUnknown14() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157550, 0x1e)
CDDrawMapHolder::~CDDrawMapHolder() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157bc0, 0xa2)
void CDDrawMapHolder::ClearUnknownMap() {}
