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
// vtable is g_remusBaseDtorVtbl @0x5e8cb4 (the same grand-base the Remus/Severus
// DDraw nodes restamp). The nodes' own virtuals are not modeled, so their primary
// vtables are referenced by address as reloc-masked DATA externs and stamped
// manually; letting cl emit a vtable would diverge. The CString member teardown
// + the destructible base subobject give the dtors their /GX EH frame.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing.
#include <Ints.h>
#include <Mfc.h> // /GX EH-frame helpers
#include <rva.h>
#include <Rez/RezMgr.h> // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
#include <string.h>     // strlen/memcpy (inlined repne scas / rep movs)

// The two node primary vtables + the shared base dtor vtable (foreign engine
// data; referenced by address as reloc-masked DATA externs while the classes
// stay non-polymorphic). Names match NetMgr.h's extern "C" decls so the node
// factories' (AddPlayerNode/AddSessionNode) vtable stamps share the same symbol.
DATA(0x001f0760)
extern "C" void* g_netPlayerNodeVtbl; // 0x5f0760
DATA(0x001f0778)
extern "C" void* g_netSessionNodeVtbl; // 0x5f0778
// 0x5e8cb4 (CObject-like grand-base dtor vptr) - already DATA-pinned in the
// Remus/Severus TUs as g_remusBaseDtorVtbl; declared extern here so the base
// stamp reloc-masks.
extern void* g_remusBaseDtorVtbl;

// The collection-node base subobject: its dtor restamps the grand-base vptr
// (0x5e8cb4). Modeled as a value base so the trailing stamp lands AFTER the
// derived teardown (eh-dtor-subobject-vptr-restore-member.md).
struct CNetNodeBase {
    void* m_vptr; // +0x00
    ~CNetNodeBase() {
        m_vptr = &g_remusBaseDtorVtbl;
    }
};

// The shared CWapNodeB string-cleanup helper (Font.cpp 0x179680): frees the two
// owned buffers at +0x34/+0x38 and clears +0x04. Declared here only so
// ~CNetPlayerListNode's call to it reloc-masks against ?FreeStrings@CWapNodeB@@QAEXXZ.
struct CWapNodeB {
    void FreeStrings();
};

// ---------------------------------------------------------------------------
// CNetPlayerListNode - a deep copy of a DPSESSIONDESC2 + its duplicated names.
// ---------------------------------------------------------------------------
class CNetPlayerListNode : public CNetNodeBase {
public:
    ~CNetPlayerListNode();
    i32 Init(void* desc);

    char m_pad04[0x54 - 0x04]; // +0x04..+0x53  the 0x50-byte DPSESSIONDESC2 copy
                               //               (lpszSessionName@+0x34, lpszPassword@+0x38)
};

// The source DPSESSIONDESC2 Init deep-copies: only the two name pointers it
// duplicates are pinned (offsets within the 0x50-byte struct).
struct CNetSessionDesc {
    char m_pad00[0x30];
    char* m_lpszName;     // +0x30  lpszSessionName
    char* m_lpszPassword; // +0x34  lpszPassword
};

// ---------------------------------------------------------------------------
// CNetSessionNode - two CString members + two raw heap buffers.
// ---------------------------------------------------------------------------
class CNetSessionNode : public CNetNodeBase {
public:
    ~CNetSessionNode();

    i32 m_04;     // +0x04  cleared on teardown
    CString m_08; // +0x08  name CString
    CString m_0c; // +0x0c  name CString
    i32 m_10;     // +0x10
    char* m_14;   // +0x14  owned buffer (freed second)
    char* m_18;   // +0x18  owned buffer (freed first)
    i32 m_1c;     // +0x1c
    i32 m_20;     // +0x20  cleared on teardown
};

// ===========================================================================
// CNetPlayerListNode::~CNetPlayerListNode  @0x1793b0
// Stamp the most-derived vtable (0x5f0760), defer the two name frees + m_type
// clear to CWapNodeB::FreeStrings, then the base subobject restamps 0x5e8cb4.
// /GX frame from the destructible base subobject.
// ===========================================================================
// @early-stop
// EH-state-machine order wall (~93.3%, eh-dtor-vptr-stamp-vs-trylevel-order):
// every instruction matches except the /GX trylevel write ([esp+0x10]=0) and the
// own-vptr stamp are emitted in the opposite order from retail (retail stamps the
// vptr then writes the state; the recompile writes the state first). Not steerable
// from C. Same plateau class as ~CSeverusEntryList (0x1557a0). Logic complete.
RVA(0x001793b0, 0x46)
CNetPlayerListNode::~CNetPlayerListNode() {
    m_vptr = &g_netPlayerNodeVtbl;
    ((CWapNodeB*)this)->FreeStrings();
}

// ===========================================================================
// CNetSessionNode::~CNetSessionNode  @0x179420
// Stamp the most-derived vtable (0x5f0778), clear m_04/m_20, free the two raw
// buffers (m_18 then m_14), then the CString members + base subobject fold in.
// ===========================================================================
// @early-stop
// EH-state-machine order wall (eh-dtor-vptr-stamp-vs-trylevel-order): the
// teardown logic is byte-faithful but the /GX state-machine writes interleave
// with the field clears in an order MSVC fixes from the member layout, not
// steerable from C (same plateau class as ~CSeverusEntryList 0x1557a0).
RVA(0x00179420, 0x8a)
CNetSessionNode::~CNetSessionNode() {
    m_vptr = &g_netSessionNodeVtbl;
    m_04 = 0;
    m_20 = 0;
    if (m_18) {
        RezFree(m_18);
    }
    m_18 = 0;
    if (m_14) {
        RezFree(m_14);
    }
    m_14 = 0;
    // m_0c, m_08 CString members + base subobject (stamp 0x5e8cb4) fold here.
}

// ===========================================================================
// CNetPlayerListNode::Init  @0x1795a0
// Deep-copy the 0x50-byte DPSESSIONDESC2 into +0x04, force dwSize=0x50, then
// strdup the session name + password into +0x34/+0x38 (overwriting the copied
// pointers). Returns 0 on a null desc, 1 otherwise. __thiscall, ret 4.
// ===========================================================================
RVA(0x001795a0, 0xdb)
i32 CNetPlayerListNode::Init(void* desc) {
    CNetSessionDesc* src = (CNetSessionDesc*)desc;
    if (!src) {
        return 0;
    }
    i32* dst = (i32*)m_pad04; // this+0x04
    memcpy(dst, src, 0x50);
    *dst = 0x50; // dwSize
    char** name = (char**)(m_pad04 + (0x34 - 0x04));
    char** pass = (char**)(m_pad04 + (0x38 - 0x04));
    *name = 0;
    *pass = 0;
    if (src->m_lpszName && strlen(src->m_lpszName)) {
        *name = (char*)RezAlloc(strlen(src->m_lpszName) + 8);
        strcpy(*name, src->m_lpszName);
    }
    if (src->m_lpszPassword && strlen(src->m_lpszPassword)) {
        *pass = (char*)RezAlloc(strlen(src->m_lpszPassword) + 8);
        strcpy(*pass, src->m_lpszPassword);
    }
    return 1;
}
