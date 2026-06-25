// InterfaceObject.cpp - the DirectPlay group-list node (C:\Proj\NetMgr), the
// 0x10-byte node AddGroupNode (0x178360) operator-new's onto CNetMgr's +0x1c list
// (final vtbl 0x5f0748, base CObject dtor-vtbl 0x5e8cb4, a CString name at +0x8).
// The trace placeholder is "InterfaceObject". Two methods are reconstructed here:
//   ~InterfaceObject (0x179340) - the /GX node destructor.
//   GetName         (0x179300) - return the +0x8 name CString by value.
// Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// The minimal MFC CString model (a single char* @+0). Its copy-ctor
// (0x1b9ba3) and dtor (0x1b9cde) are MFC/CRT (NAFXCW) - declared no-body so the
// member operations' rel32 calls reloc-mask.
class CString {
public:
    CString();
    CString(const CString& s);
    ~CString();
    char* m_data; // +0x00
};

// The two vtables stamped as the node's hierarchy is torn down (transitional
// manual stamps - the node's virtuals aren't modeled, so cl can't emit a matching
// vtable; reference the retail tables by address as reloc-masked DATA externs).
DATA(0x001f0748)
extern void* g_groupNodeVtbl; // 0x5f0748  final (most-derived) vptr
// g_remusBaseDtorVtbl (0x5e8cb4) is the CObject base dtor-vptr, already DATA-pinned
// in the Remus/DDraw TUs; declared extern only here so the base stamp reloc-masks.
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4

// The CObject base subobject: its dtor restamps the base vptr (0x5e8cb4). Modeled
// as a value base so the trailing stamp lands AFTER the CString member teardown
// (eh-dtor-subobject-vptr-restore-member.md).
struct InterfaceObjectBase {
    void* m_vptr; // +0x00
    ~InterfaceObjectBase() {
        m_vptr = &g_remusBaseDtorVtbl;
    }
};

class InterfaceObject : public InterfaceObjectBase {
public:
    i32 m_4;     // +0x04
    CString m_8; // +0x08  name
    i32 m_c;     // +0x0c
    ~InterfaceObject();
    CString GetName();
};

// InterfaceObject::GetName (0x179300) - copy-construct the +0x8 name CString into
// the caller's return slot and hand it back.
RVA(0x00179300, 0x20)
CString InterfaceObject::GetName() {
    return m_8;
}

// InterfaceObject::~InterfaceObject (0x179340) - stamp the most-derived vptr
// (0x5f0748), zero m_4/m_c, then (member teardown) destruct the +0x8 CString and
// (base teardown) restamp the CObject base vptr (0x5e8cb4). The CString member's
// non-trivial dtor forces the /GX EH frame.
// @early-stop
// 89.4% /GX eh-dtor trylevel-sequencing wall (docs/patterns/
// eh-dtor-vptr-stamp-vs-trylevel-order.md family): the two vptr stamps, the
// m_4/m_c zeroing, the inline ~CString member teardown and the base vptr restamp
// are all byte-faithful, but the compiler-generated [esp+N] trylevel writes are
// scheduled in a slightly different order relative to the user stamps - the /GX
// EH-state machine's choice, not source-steerable. Logic 100% correct; deferred
// to the final sweep.
RVA(0x00179340, 0x48)
InterfaceObject::~InterfaceObject() {
    *(void**)this = &g_groupNodeVtbl;
    m_4 = 0;
    m_c = 0;
}
