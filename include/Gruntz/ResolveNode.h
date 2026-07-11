#ifndef GRUNTZ_GRUNTZ_RESOLVENODE_H
#define GRUNTZ_GRUNTZ_RESOLVENODE_H

// ResolveNode.h - CResolveNode, the 0x68-byte CLoadable leaf node of the DDraw
// surface/page-manager family (own primary vftable @0x1efbc0 = ??_7CResolveNode;
// the shared WwdBResolve grand-base of the CWwdGameObject factory objects).
// Hoisted from ResolveNode.cpp (wave4-L) so the split method set can live in its
// retail objs: default ctor/dtor pocket (D, ResolveNode.cpp), the 3-arg ctor
// (I obj, WwdFactoryObject.cpp), Init (T obj, DDrawSurfacePair.cpp).
//
// Layout recovered from the ctor/dtor/Init stores; only OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).

#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : CObject (9-slot base)
#include <Ints.h>
#include <rva.h>

class CResolveNode : public CLoadable {
public:
    // Re-based onto the canonical 9-slot CLoadable: the m_04/m_08/m_0c header +
    // slots 5..8 come from CLoadable. This node OVERRIDES slot 5 (IsLoaded
    // @0x154a10) and slot 7 (Unload/reset @0x154a80), INHERITS slot 6 (IsReady
    // @0x001c08) and slot 8 (GetClassId @0x154a00). Resolve (slot 9) is the
    // node's own new virtual.
    i32 IsLoaded() OVERRIDE;       // [5] 0x154a10  (checks m_04!=-1 && m_0c)
    i32 Unload() OVERRIDE;         // [7] 0x154a80  reset/reload hook
    virtual i32 Resolve(i32, i32); // [9] 0x164790  CResolveNode's own new virtual

    CResolveNode();                                    // 0x1549d0 (D pocket)
    CResolveNode(i32 owner, i32 field04, i32 field08); // 0x15b2c0 (I obj)
    virtual ~CResolveNode() OVERRIDE;                  // 0x154a50 (D pocket)
    i32 Init(i32 owner, i32 field04, i32 resolveX, i32 resolveY, i32 field40, i32 field08);
    // ^ 0x1647e0 (T obj)

    // vptr @+0x00 + m_04/m_08/m_0c inherited from CLoadable; own scratch from +0x10.
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
SIZE_UNKNOWN(CResolveNode);

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H
