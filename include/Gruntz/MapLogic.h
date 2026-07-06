// MapLogic.h - the CUserLogic-derived 2D terrain/influence grid game-logic object
// (placeholder name CMapLogic; distinct from the 0x60-byte CMapMgr container in
// MapMgr.h). The trace grouped these __thiscall methods under "CBrickz", but the
// matched CBrickz container is the self-contained node graph in Brickz.cpp; this
// class is the *CUserLogic leaf* whose ctor (0x113c0) stores the CUserLogic vftable
// (0x5e705c) and constructs the +0x18 link sub-object (the /GX EH frame), then owns
// a terrain-flag cell grid, an MFC CArray at +0x7c and a parallel pointer array at
// +0x80/+0x84.
//
// RTTI: the most-derived vftable the ctor stores is CUserLogic's own (0x5e705c) -
// this leaf adds no virtuals, so there is no distinct RTTI class for it; the name
// is a structural placeholder. ONLY offsets + code bytes are load-bearing.
//
// The grid cells are 0x1c bytes stride; each cell's +0x00 holds packed terrain
// flags (the 0x939 passability mask, also seen in CGruntPuddle), +0x04 holds a
// per-cell id. The serializer methods (0xec230 / 0x82430 / 0x9f7f0) drive an
// archive object through its vtable: slot +0x2c = read-in (mode 7), slot +0x30 =
// write-out (mode 4); 0x9f7f0 dispatches slots +0x08 / +0x0c.
#ifndef GRUNTZ_MAPLOGIC_H
#define GRUNTZ_MAPLOGIC_H

#include <rva.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/UserLogic.h>

// The CArchive-like serializer object the Serialize methods drive is the shared WAP32
// CSerialArchive (Read @ vtable +0x2c / Write @ +0x30), now the one modeled class in
// <Gruntz/SerialArchive.h> - the former local `CMapArchive` view is folded away.

// The serializable float curve (a monotone lookup ramp in .data at VA 0x64cfb0;
// 0xec230 reads/writes a 12-float slice). Modeled extern + DATA() so the
// `push offset` operands reloc-mask against the retail .data symbol.
DATA(0x0024cfb0)
extern float g_mapCurve[12]; // 0x64cfb0..0x64cfddf slice the serializer touches

// The intrusive free-list node allocator the +0x84 pointer-array serializer pulls
// nodes from (shared with Projectile.cpp / BattlezMapConfig.cpp). The node body
// pointer is recovered as (slot - g_freeListNodeBias).
extern void* g_freeList;       // ?g_freeList@@3PAXA            (VA 0x645544)
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA      (VA 0x64554c)

// ---------------------------------------------------------------------------
// CObArray-like MFC pointer array embedded at CMapLogic+0x7c. The CObject-derived
// MFC array layout: {vtable@+0, m_pData@+4, m_nSize@+8, m_nMaxSize@+0xc,
// m_nGrowBy@+0x10} = 0x14 bytes. SetSize @0x1b4f75 (__thiscall, ret 8) is called
// on it with ecx = &m_arr. Modeled with a no-body SetSize so the call reloc-masks.
SIZE_UNKNOWN(CMapPtrArray);
struct CMapPtrArray {
    void SetSize(i32 n, i32 growBy); // 0x1b4f75 (__thiscall)
    char _vft0[4];  // +0x00 foreign/base object vptr (reduced view; not owned/dispatched)
    void** m_pData; // +0x04  the pointer array body
    i32 m_nSize;    // +0x08  element count
    i32 m_nMaxSize; // +0x0c
    i32 m_nGrowBy;  // +0x10
};

// ---------------------------------------------------------------------------
// CMapLogic : CUserLogic - the terrain/influence grid leaf. Field names are
// placeholders; offsets are recovered from the field stores in the methods below.
// The CUserLogic base occupies +0x00..+0x3f; this leaf's grid/array members start
// past it.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CMapLogic);
class CMapLogic : public CTileLogic {
public:
    CMapLogic();           // no-arg shape (only the teardown is here)
    ~CMapLogic() OVERRIDE; // 0x0113c0 (bare CUserLogic teardown, /GX frame)

    // The +0x7c pointer-array serializer (0x082430) + its tear-down helper
    // (0x085480). __thiscall; both free the array's nodes back to g_freeList,
    // resize the CObArray via SetSize, and dispatch the archive read/write slots.
    i32 SerializeNodes(CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // 0x082430
    void FreeNodes();                                                 // 0x085480

    // The grid-reset cleanup the tear-down chains (0x9ec30, __thiscall, no args).
    // External/no-body so the `mov ecx,this; call rel32` reloc-masks (it shares the
    // mapmgr unit's Reset address; redefining it would clash). NOT a virtual here.
    void Reset(); // 0x09ec30

    // grid/array members (offsets from the field stores; the CUserLogic base owns
    // +0x00..+0x3f). Only the touched offsets are named.
    char m_pad40[0x7c - 0x40];
    CMapPtrArray m_arr; // +0x7c  CObArray (m_pData@+0x80, m_nSize@+0x84)
    i32 m_90;           // +0x90  scratch dword the node serializer streams
};

// 0xec230: the float-curve serializer. __cdecl free function (caller-cleanup,
// `ret`); drives the archive's read/write slots over the g_mapCurve slice keyed by
// `mode` (7=read via +0x2c, 4=write via +0x30). Declared at namespace scope.
i32 MapSerializeCurve(CSerialArchive* ar, i32 mode); // 0x0ec230

// 0x9f7f0: a tiny polymorphic Visit - reads [ecx] as a vtable and calls slot +0x08
// (mode 4) or +0x0c (mode 7) with the buffer arg, returning 1 unless the slot
// returned non-zero. __thiscall (ecx=this) with 4 stack args (ret 0x10), only the
// first two used. The visited object's slots +0x08 / +0x0c are modeled polymorphic
// (decls only) so `mov edx,[ecx]; push buf; call [edx+8]` falls out.
SIZE_UNKNOWN(CMapVisitTarget);
struct CMapVisitTarget {
    virtual i32 Slot00();
    virtual i32 Slot04();
    virtual i32 Slot08(void* buf);                  // +0x08 (mode 4)
    virtual i32 Slot0C(void* buf);                  // +0x0c (mode 7)
    i32 Visit(void* buf, i32 mode, i32 a2, i32 a3); // 0x09f7f0 (__thiscall)
};

#endif // GRUNTZ_MAPLOGIC_H
