#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H
#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // real MFC CObject (the primary-facet base)

class CDDrawSubMgrLeafScan; // the token-map ctx (ex CAniMapOwner - its +0x10 Ptr map is m_10)
class CDDrawSurfaceMgr;     // the record owner (ex CAniRecordOwner - m_ptrColl/m_drawTarget)

struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE; // [1] 0x1657a0 real primary-facet teardown dtor

    i32 Parse(void* ctx, const i16* src);                      // 0x168c60
    i32 GetSize();                                             // 0x168e50
    void ResolveIndices(CDDrawSubMgrLeafScan* owner, const char* str); // 0x168d00
    void* AllocBufMakeB(i32 size, i32 flag);                   // 0x168ee0  (pool MakeB)
    void* AllocBufCreate(i32 handle, i32 flag);                // 0x168f20  (pool Create, slot 9)
    void* AllocBufMakeB2(i32 size, i32 flag);                  // 0x168ea0  (pool MakeB2)
    void* AllocBufMakeB3(i32 a, i32 size, i32 flag);           // 0x168f60  (pool MakeB3)
    void FreeBuf();                                            // 0x168fb0
    // 0x168fd0 (vtable slot 13): when the owner image is 8bpp, push the record's
    // palette buffer (m_buf) onto the owner's surface; else return 1.
    i32 PushPalette();

    inline CAniRecordView() {
        m_count = 0;
        m_indices = 0;
        m_owner = reinterpret_cast<CDDrawSurfaceMgr*>(0xffff);
    }

    // vptr implicit at +0x00
    u16 m_flags;              // +0x04  status word (bit 1 scaled, bit 2 has-name)
    u16 m_06;                 // +0x06
    i32 m_08;                 // +0x08
    CDDrawSurfaceMgr* m_owner; // +0x0c  the owning surface mgr (seeded 0xffff sentinel)
    i32 m_buf;                // +0x10  pool work buffer
    i32 m_seedFrame;          // +0x14  parsed seed/start frame (SetAnimEx reads record[0]'s)
    i32 m_frameCount;         // +0x18  frame count (GetSize)
    i32 m_1c;                 // +0x1c
    i32 m_20;                 // +0x20
    i32 m_24;                 // +0x24
    u16 m_28;                 // +0x28
    u16 m_2a;                 // +0x2a
    i32 m_count;              // +0x2c  resolved-index array length
    i32* m_indices;           // +0x30  resolved-index array
};

// --- vtable catalog ---
// The primary 5-slot CObject-derived facet vtable. [vtbl-4]=NULL (verified) -> no RTTI
// COL, so the retail class name is UNRECOVERABLE: neither "CAniRecordView" nor the guessed
// "CAniRecordPrimary" is provable, so the reconstruction name CAniRecordView stands (not
// renamed - unprovable). @identity-TODO: real retail class name unrecoverable (no RTTI).
VTBL(CAniRecordView, 0x001f02c0);

#endif // GRUNTZ_CANIRECORDVIEW_H
