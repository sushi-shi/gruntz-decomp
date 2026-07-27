#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H
#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // real MFC CObject (the primary-facet base)

class CDDrawSubMgrLeafScan; // the token-map ctx (ex CAniMapOwner - its +0x10 Ptr map is m_10)
class CDDrawSurfaceMgr;
// struct, NOT class - CDDPalette is defined as a struct; the class-key of the FIRST
// decl a TU sees is what MSVC mangles into its references (see AniRecordBase2.h).
struct CDDPalette; // the +0x10 work palette

struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE; // [1] 0x1657a0 real primary-facet teardown dtor

    i32 Parse(void* ctx, const i16* src);                              // 0x168c60
    i32 GetSize();                                                     // 0x168e50
    void ResolveIndices(CDDrawSubMgrLeafScan* owner, const char* str); // 0x168d00
    // (the Alloc*/FreeBuf/PushPalette pool leaves are NOT this class's - they are
    // CAniRecordBase2's own vtable-slot bodies; see <DDrawMgr/AniRecordBase2.h>.)

    inline CAniRecordView() {
        m_count = 0;
        m_indices = 0;
        m_owner = 0xffff;
    }

    // vptr implicit at +0x00
    u16 m_flags;               // +0x04  status word (bit 1 scaled, bit 2 has-name)
    u16 m_06;                  // +0x06
    i32 m_08;                  // +0x08
    // +0x0c/+0x10 are SERIALIZED HANDLES, not pointers: Parse @0x168c60 reads them
    // straight out of the i16 record stream and the dtor stamps +0x0c with the 0xffff
    // sentinel. Nothing in the tree ever dereferences either - they were typed
    // CDDrawSurfaceMgr*/CDDPalette* (copied from CAniRecordBase2's same-offset pair),
    // which cost four casts to write a 16-bit stream value into a pointer slot.
    i32 m_owner;   // +0x0c  serialized owner handle (0xffff = unbound)
    i32 m_palette; // +0x10  serialized palette handle
    i32 m_seedFrame;           // +0x14  parsed seed/start frame (SetAnimEx reads record[0]'s)
    i32 m_frameCount;          // +0x18  frame count (GetSize)
    i32 m_1c;                  // +0x1c
    i32 m_20;                  // +0x20
    i32 m_24;                  // +0x24
    u16 m_28;                  // +0x28
    u16 m_2a;                  // +0x2a
    i32 m_count;               // +0x2c  resolved-index array length
    i32* m_indices;            // +0x30  resolved-index array
};
SIZE_UNKNOWN();

// --- vtable catalog ---
// The primary 5-slot CObject-derived facet vtable. [vtbl-4]=NULL (verified) -> no RTTI
// COL, so the retail class name is UNRECOVERABLE: neither "CAniRecordView" nor the guessed
// "CAniRecordPrimary" is provable, so the reconstruction name CAniRecordView stands (not
// renamed - unprovable). @identity-TODO: real retail class name unrecoverable (no RTTI).

#endif // GRUNTZ_CANIRECORDVIEW_H
