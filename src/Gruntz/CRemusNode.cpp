#include <rva.h>
// CRemusNode.cpp - a leaf node in the CDirectDrawMgr surface/page-manager
// Lucius/Remus family (the "remus base dtor vtable" lineage; the base subobject
// vftable g_remusBaseDtorVtbl @0x5e8cb4 is restamped at ~CRemusNode exit). The
// node's own primary vftable is at RVA 0x1efbc0 (g_remusNodeVtbl). The node's
// virtuals are not yet matched, so the vftable is referenced as a reloc-masked
// DIR32 datum and stamped manually rather than modeled polymorphically (an
// incomplete polymorphic class would emit a divergent ??_7 vtable). Only the
// OFFSETS + emitted bytes are load-bearing; field names are placeholders.
//
// Methods (all plain /O2 /MT leaves, NO EH frame, __thiscall):
//   0x1549d0  default ctor (seeds sentinels, m_0c = 0)
//   0x154a30  ??_G scalar-deleting destructor (in Discovered.cpp slot 1 of vtbl)
//   0x154a50  ~CRemusNode (resets fields, restamps base vptr)
//   0x15b2c0  parameterized ctor (root + two scalars)
//   0x1647e0  Init(a1..a6): seeds fields, dispatches virtual slot 9, returns 1

// The node's primary vftable (foreign engine datum; RVA = VA-0x400000).
DATA(0x001efbc0)
extern void* g_remusNodeVtbl; // 0x5efbc0

// The base-subobject (CObject-like, "remus base") dtor vftable restamped at
// ~CRemusNode exit; reloc-masked DATA() while the class stays non-polymorphic.
DATA(0x005e8cb4)
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4

struct CRemusNodeVtbl; // primary vftable view; PMF slot defined after the class

// The 0x68-byte node. Layout recovered from the ctor/dtor/Init stores; the gaps
// are unread scratch (the family's resolution-ladder block).
class CRemusNode {
public:
    CRemusNode();
    CRemusNode(i32 root, i32 a2, i32 a3);
    ~CRemusNode();
    i32 Init(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6);

    CRemusNodeVtbl* m_vptr;   // +0x00
    i32 m_04;                 // +0x04
    i32 m_08;                 // +0x08
    i32 m_0c;                 // +0x0c
    char _pad10[0x20 - 0x10]; // +0x10..+0x1f
    i32 m_20;                 // +0x20  = 0x80000000
    char _pad24[0x38 - 0x24]; // +0x24..+0x37
    i32 m_38;                 // +0x38  = -1
    i32 m_3c;                 // +0x3c  = 0
    i32 m_40;                 // +0x40
    char _pad44[0x4c - 0x44]; // +0x44..+0x4b
    i32 m_4c;                 // +0x4c
    i32 m_50;                 // +0x50
    char _pad54[0x58 - 0x54]; // +0x54..+0x57
    i32 m_58;                 // +0x58
    i32 m_5c;                 // +0x5c  = 0x80000000
    char _pad60[0x64 - 0x60]; // +0x60..+0x63
    i32 m_64;                 // +0x64  = 0x80000000
};

// Typed view of the node's primary vftable (g_remusNodeVtbl @0x5efbc0). Slot 9
// (byte offset 0x24) is dispatched by Init: a __thiscall method of the node
// taking two scalars (vtbl[9] = 0x164790). Modeled as a pointer-to-member of the
// now-complete CRemusNode so MSVC sizes it as a 4-byte code pointer and lowers
// the call to `mov ecx,this; call [vptr+0x24]`. The leading slots are opaque
// pointers so the dispatched slot lands at the right byte offset.
struct CRemusNodeVtbl {
    void* m_slot0to8[9];                    // +0x00..+0x20
    i32 (CRemusNode::*m_resolve)(i32, i32); // +0x24
};

// Default ctor: seeds the resolution/scaling sentinels and zeroes the base
// fields, then stamps the node vftable. No arg-store to float, unlike the
// parameterized ctor below.
RVA(0x001549d0, 0x29)
CRemusNode::CRemusNode() {
    m_0c = 0;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    *(void**)this = &g_remusNodeVtbl;
    m_5c = (i32)0x80000000;
    m_64 = (i32)0x80000000;
    m_3c = 0;
    m_40 = 0;
}

// ~CRemusNode: resets the sentinels/base fields and restamps the base-subobject
// (CObject-like) dtor vftable. Trivial base -> no member teardown, no EH frame.
// @early-stop
// sentinel-seed store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// the body is byte-identical EXCEPT the single immediate vptr restamp store
// (`mov [ecx],&g_remusBaseDtorVtbl`). Retail defers it to just after the eax
// sentinel chain (`xor eax,eax` then the store, before m_08/m_0c); MSVC5 here
// eagerly hoists the data-independent immediate store to position 2 (right after
// `mov eax,0x80000000`). No source order steers it (tried vptr-first/mid/last; all
// ~78.8%) - the immediate store carries no dependency. Logic complete.
RVA(0x00154a50, 0x23)
CRemusNode::~CRemusNode() {
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_04 = -1;
    m_08 = 0;
    *(void**)this = &g_remusBaseDtorVtbl;
    m_0c = 0;
}

// @early-stop
// sentinel-seed ctor store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// identical instruction multiset, but cl floats the m_08 (edx=arg3) store and the
// m_38 (-1) store to different positions than retail; 3 field-order spellings all
// ~60%. Source steers which arg lands in edx, not the store schedule. Logic complete.
RVA(0x0015b2c0, 0x3d)
CRemusNode::CRemusNode(i32 root, i32 a2, i32 a3) {
    m_04 = a2;
    m_08 = a3;
    m_0c = root;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_5c = (i32)0x80000000;
    m_64 = (i32)0x80000000;
    *(void**)this = &g_remusNodeVtbl;
    m_3c = 0;
    m_40 = 0;
}

// Init: seeds m_0c/m_04/m_08 from args, clears m_4c/m_58, sets m_50 = 1, then
// dispatches the node's virtual slot 9 (g_remusNodeVtbl[9] = 0x164790) with two
// of the args, finally stores a5 into m_40 and returns TRUE.
RVA(0x001647e0, 0x48)
i32 CRemusNode::Init(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    m_0c = a1;
    m_04 = a2;
    m_08 = a6;
    m_4c = 0;
    m_58 = 0;
    m_50 = 1;
    (this->*(m_vptr->m_resolve))(a3, a4);
    m_40 = a5;
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CRemusNode);
SIZE_UNKNOWN(CRemusNodeVtbl);
