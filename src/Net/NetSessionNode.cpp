#include <Ints.h>
#include <Net/NetMgr.h> // canonical CNetPlayerListNode / CNetSessionNode / CNetSessionDesc
#include <rva.h>
#include <Rez/RezMgr.h> // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
#include <string.h>     // strlen/memcpy (inlined repne scas / rep movs)

VTBL(CNetSessionNode, 0x001f0778); // own (final) vtable

RVA(0x001793b0, 0x46)
CNetPlayerListNode::~CNetPlayerListNode() {
    FreeStrings();
}

// ===========================================================================
// CNetSessionNode::~CNetSessionNode  @0x179420
// Stamp the most-derived vtable (0x5f0778), clear m_04/m_20, free the two raw
// buffers (m_18 then m_14), then the CString members + base subobject fold in.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CNetSessionNode own-vptr stamp
// in the ENTRY state, clears m_04/m_20, frees the two raw buffers, then the two
// CString members + the empty ~CObject (grand-base re-stamp) fold in last.
// /GX frame from the destructible CString members + base subobject.
// @early-stop
// 97.6% (was 90.6%): own-vptr stamp now compiler-emitted stamp-first; residual is
// the /GX trylevel ordering across the two folded ~CString member teardowns vs the
// grand-base fold - an EH-state-machine schedule detail, not source-steerable. The
// teardown logic is byte-faithful. Final-sweep candidate.
RVA(0x00179420, 0x8a)
CNetSessionNode::~CNetSessionNode() {
    m_sessionId = 0;
    m_listPosition = 0;
    if (m_ownedBufferA) {
        RezFree(m_ownedBufferA);
    }
    m_ownedBufferA = 0;
    if (m_ownedBufferB) {
        RezFree(m_ownedBufferB);
    }
    m_ownedBufferB = 0;
    // m_0c, m_08 CString members + base subobject (stamp 0x5e8cb4) fold here.
}

RVA(0x001795a0, 0xdb)
i32 CNetPlayerListNode::Init(CNetSessionDesc* src) {
    if (!src) {
        return 0;
    }
    memcpy(&m_desc, src, 0x50);
    m_desc.m_dwSize = 0x50;
    m_desc.m_lpszName = 0;
    m_desc.m_lpszPassword = 0;
    if (src->m_lpszName && strlen(src->m_lpszName)) {
        m_desc.m_lpszName = static_cast<char*>(::operator new(strlen(src->m_lpszName) + 8));
        strcpy(m_desc.m_lpszName, src->m_lpszName);
    }
    if (src->m_lpszPassword && strlen(src->m_lpszPassword)) {
        m_desc.m_lpszPassword = static_cast<char*>(::operator new(strlen(src->m_lpszPassword) + 8));
        strcpy(m_desc.m_lpszPassword, src->m_lpszPassword);
    }
    return 1;
}
