// NetSessionNode.cpp - the DirectPlay session/player enumeration list nodes
// (C:\Proj\NetMgr). Two sibling WAP collection-node types whose payload the
// CNetMgr session-enumeration walk (CNetMgr::Find, NetMgr.cpp) traverses:
//
//   CNetPlayerListNode (own vtbl 0x5f0760) - holds a deep copy of a 0x50-byte
//       DPSESSIONDESC2 at +0x04 plus two heap-duplicated name strings whose
//       pointers live INSIDE that copy (+0x34 lpszSessionName, +0x38 lpszPassword).
//       Init (0x1795a0) memcpy's the desc, forces dwSize=0x50, then strdup's the
//       two names. Its ~dtor (0x1793b0) defers the two frees to the shared
//       CWapNodeB::FreeStrings helper (Font.cpp, 0x179680, reloc-masked).
//   CNetSessionNode (own vtbl 0x5f0778) - holds two CString members (+0x08/+0x0c)
//       and two raw heap buffers (+0x14/+0x18). Its ~dtor (0x179420) frees the
//       buffers, clears +0x04/+0x20, then the CString members + base subobject
//       fold in.
//
// Both derive from the engine CObject-like collection-node base whose dtor
// vtable lives at 0x5e8cb4 (the shared CObject grand-base). Both nodes are now
// REAL POLYMORPHIC: cl auto-emits their own vtables (0x5f0760 / 0x5f0778, orphan
// reloc-masked) and auto-stamps them in the ctor (the node factories in NetMgr.cpp
// now `new` these classes) and dtor. The CString member teardown + the
// destructible base subobject give the dtors their /GX EH frame.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing.
#include <Ints.h>
#include <Font/Font.h>  // canonical CWapNodeB (string-cleanup base, ?FreeStrings@CWapNodeB@)
#include <Net/NetMgr.h> // canonical CNetPlayerListNode / CNetSessionNode / CNetSessionDesc
#include <rva.h>
#include <Rez/RezMgr.h> // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
#include <string.h>     // strlen/memcpy (inlined repne scas / rep movs)

// Both node types derive from the shared engine grand-base Wap::CObject (RTTI
// "CObject", 5-slot interface, grand-base dtor vtable @0x5e8cb4). vtable_hierarchy
// confirms each node's 5 slots are exactly the CObject interface (slots 0/2/3/4 the
// inherited ILT thunks 0x1bef01/0x0028ec/0x00106e/0x004034, slot 1 the destructor
// override) - so neither node adds a new virtual. Real polymorphic: cl folds the
// empty ~CObject grand-base re-stamp (reloc-masks 0x5e8cb4) LAST into each leaf
// dtor, and the destructible base subobject supplies the leaf dtor's /GX EH frame.
// The node factories (AddPlayerNode/AddSessionNode, NetMgr.cpp) `new` these classes
// so cl auto-stamps the own vtables (0x5f0760 / 0x5f0778) in the ctors - no manual
// stamp anywhere. Class defs are canonical in <Net/NetMgr.h>; the own-vtable VTBLs
// live here (this TU owns the two ??_7 RVAs, so no dup-DATA with the header).
VTBL(CNetPlayerListNode, 0x001f0760); // own (most-derived) vtable
VTBL(CNetSessionNode, 0x001f0778);    // own (final) vtable

// ===========================================================================
// CNetPlayerListNode::~CNetPlayerListNode  @0x1793b0
// Stamp the most-derived vtable (0x5f0760), defer the two name frees + m_type
// clear to CWapNodeB::FreeStrings, then the base subobject restamps 0x5e8cb4.
// /GX frame from the destructible base subobject.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CNetPlayerListNode own-vptr
// stamp in the ENTRY state (stamp-first, == retail), then FreeStrings, then the
// empty ~Wap::CObject folds the grand-base re-stamp last. /GX frame from the
// destructible base subobject. (eh-dtor-implicit-vptr-stamp-first.md.)
RVA(0x001793b0, 0x46)
CNetPlayerListNode::~CNetPlayerListNode() {
    ((CWapNodeB*)this)->FreeStrings();
}

// ===========================================================================
// CNetSessionNode::~CNetSessionNode  @0x179420
// Stamp the most-derived vtable (0x5f0778), clear m_04/m_20, free the two raw
// buffers (m_18 then m_14), then the CString members + base subobject fold in.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CNetSessionNode own-vptr stamp
// in the ENTRY state, clears m_04/m_20, frees the two raw buffers, then the two
// CString members + the empty ~Wap::CObject (grand-base re-stamp) fold in last.
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

// ===========================================================================
// CNetPlayerListNode::Init  @0x1795a0
// Deep-copy the 0x50-byte DPSESSIONDESC2 into +0x04, force dwSize=0x50, then
// strdup the session name + password into +0x34/+0x38 (overwriting the copied
// pointers). Returns 0 on a null desc, 1 otherwise. __thiscall, ret 4.
// ===========================================================================
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
        m_desc.m_lpszName = (char*)RezAlloc(strlen(src->m_lpszName) + 8);
        strcpy(m_desc.m_lpszName, src->m_lpszName);
    }
    if (src->m_lpszPassword && strlen(src->m_lpszPassword)) {
        m_desc.m_lpszPassword = (char*)RezAlloc(strlen(src->m_lpszPassword) + 8);
        strcpy(m_desc.m_lpszPassword, src->m_lpszPassword);
    }
    return 1;
}
