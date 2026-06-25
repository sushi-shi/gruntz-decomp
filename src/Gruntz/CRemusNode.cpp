#include <rva.h>
// CRemusNode.cpp - the ctor (0x15b2c0) of a leaf node in the CDirectDrawMgr
// surface/page-manager Lucius/Remus family (the "remus base dtor vtable" lineage,
// g_remusBaseDtorVtbl @0x5e8cb4). This node's own primary vftable is at RVA
// 0x1efbc0; its base ctor/dtor live at 0x1549d0 / 0x154a50 (currently stubbed as
// HelperHost_*), and its scalar-deleting destructor at 0x154a30. Three factory
// methods in the 0x559... block call this ctor.
//
// The ctor takes three args (a root/parent handle + two scalars), seeds the Lucius
// base fields (m_04/m_08/m_0c) and the resolution/scaling sentinels
// (m_20/m_5c/m_64 = 0x80000000, m_38 = -1, m_3c/m_40 = 0), and stamps the node
// vftable. The node's virtuals are not yet matched, so the vftable is referenced
// as a reloc-masked DIR32 datum (g_remusNodeVtbl) and stamped manually rather than
// modeled polymorphically (an incomplete polymorphic class would emit a divergent
// ??_7 vtable). Only the OFFSETS + emitted bytes are load-bearing; field names are
// placeholders. Plain /O2 /MT leaf: NO EH frame; __thiscall, ret 0xc.

// The node's primary vftable (foreign engine datum; RVA = VA-0x400000).
DATA(0x001efbc0)
extern void* g_remusNodeVtbl; // 0x5efbc0

// The 0x68-byte node. Layout recovered from the ctor stores; the gaps are unread
// scratch (the family's resolution-ladder block).
class CRemusNode {
public:
    CRemusNode(i32 root, i32 a2, i32 a3);

    void* m_vptr;             // +0x00
    i32 m_04;                 // +0x04  = a2
    i32 m_08;                 // +0x08  = a3
    i32 m_0c;                 // +0x0c  = root handle
    char _pad10[0x20 - 0x10]; // +0x10..+0x1f
    i32 m_20;                 // +0x20  = 0x80000000
    char _pad24[0x38 - 0x24]; // +0x24..+0x37
    i32 m_38;                 // +0x38  = -1
    i32 m_3c;                 // +0x3c  = 0
    i32 m_40;                 // +0x40  = 0
    char _pad44[0x5c - 0x44]; // +0x44..+0x5b
    i32 m_5c;                 // +0x5c  = 0x80000000
    char _pad60[0x64 - 0x60]; // +0x60..+0x63
    i32 m_64;                 // +0x64  = 0x80000000
};

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
