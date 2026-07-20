#ifndef GRUNTZ_CDDRAWWORKER_H
#define GRUNTZ_CDDRAWWORKER_H
#include <rva.h>

#include <Ints.h>
#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : CObject (9-slot base)

class CImage; // <Image/CImage.h>

class CSymTab;      // Bute/SymTab.h - the name->record table slots 10/15 iterate
class CImageParent; // the +0x0c owning parent handed to each frame (== CImage::m_parent)

class CDDrawWorker : public CLoadable {
public:
    virtual ~CDDrawWorker() OVERRIDE; // slot 1 (scalar-deleting dtor)
    // slots 5/7/8: CDDrawWorker's own overrides of the CLoadable defaults (ground
    // truth = the retail 0x1efbe8 vtable): IsLoaded @0x155750, Unload -> the frame
    // teardown (retail slot fn 0x151eb0 == the non-virtual DeleteAll below, bound
    // in wwdgameobject; declared-only here so the slot reloc masks it), GetClassId
    // @0x155770 -> CLASSID_WORKER. Slot 6 IsReady stays inherited (0x001c08).
    virtual i32 IsLoaded() OVERRIDE; // [5] @0x155750  m_0c && m_04 != -1
    // slot 6 IsReady (0x001c08) is INHERITED from CLoadable (same body RVA;
    // audit: redeclare-nothing).
    virtual i32 Unload() OVERRIDE;     // [7] @0x151eb0  == DeleteAll (declared-only)
    virtual i32 GetClassId() OVERRIDE; // [8] @0x155770  -> CLASSID_WORKER
    // slots 9-16: the 8 new virtuals CDDrawWorker adds over CLoadable's 9-slot base.
    // Declared-only => cl emits the full 17-slot ??_7CDDrawWorker @0x1efbe8 (this
    // realizes the vtable per the all-vtables mandate; was a bare VTBL() manual ref).
    // Bodies live at their retail RVAs (reloc-masked). Slots 10 (0x1521f0, byte-exact)
    // and 15 (0x1522b0, @early-stop regalloc wall) are HOMED in DDrawWorker.cpp: they
    // walk a CSymTab scope and self-dispatch InsertFrame (slot 14) / ReloadFrame
    // (slot 16). m_0c is the owning CDDrawSurfaceMgr (single-frame flag m_flags&0x100).
    virtual i32 SetKey_155810(const char* key);            // slot 9  @0x155810 (key copy)
    virtual i32 BuildFramesFromSymTab(CSymTab* tab);       // slot 10 @0x1521f0
    virtual CImage* CreateFrame24(i32 a0, i32 a1, i32 index, i32 a3); // slot 11 @0x152110
    virtual CImage* CreateFrame28(i32 a0, i32 a1, i32 index, i32 a3); // slot 12 @0x152060
    virtual CImage* CreateFrame30(i32 a0, i32 index, i32 a2);         // slot 13 @0x151fb0
    // FOLD (stage 4, DONE for CSprite): the ex `CSprite` (<Gruntz/Sprite.h>) IS this
    // class - it is now a typedef of it. Slot 14's body @0x151f00 was declared as
    // ?InsertFrame@CSprite@@ while BEING this vtable's slot-14 body: its own code reads
    // the frame CObArray at +0x10 (m_pData@+0x14 / m_nSize@+0x18) and the owner at
    // +0x0c - i.e. m_items/m_owner, offset for offset - and it already cast its own
    // array to the real ::CObArray to call SetAtGrow. Declaring it here as the real
    // virtual retires that WIRING row. The return type is CImage* (the body's own
    // mangled name says PAVCImage, not the `i32` this slot used to be declared with).
    //
    // FOLD (stage 5, DONE for CImageSet): that third view of this same 0x6c object is
    // dissolved too - it is now a typedef, and slots [11]/[12]/[13] (0x152110/0x152060/
    // 0x151fb0), which it had declared as CImageSet::CreateFrame24/28/30, are this
    // class's own virtuals. Their return type is CImage* (the bodies' mangled names say
    // PAVCImage), not the `i32` the slots used to be declared with.
    virtual CImage* InsertFrame(void* rec, i32 n, i32 flag); // slot 14 @0x151f00
    virtual i32 ValidateFramesFromSymTab(CSymTab* tab);  // slot 15 @0x1522b0
    virtual i32 ReloadFrame(i32 rec, i32 n, i32 flag); // slot 16 @0x1523b0
    // ---- the ex-CImageSet non-virtual methods (stage 5 of the fold; bodies in
    // wwdgameobject at their retail RVAs). They were declared on a THIRD view of this
    // same 0x6c object; CreateFrame24/28/30 above are this vtable's own slots 11/12/13.
    i32 SetAllTypes(i32 type);        // 0x152480  walk [min,max], set each frame's draw type
    i32 SetAllFormats(i32 format);    // 0x152520  @fake-param: really a ShadeDescr*
    i32 SetAllField18(i32 value);     // 0x1524d0  walk [min,max], set each owned light level
    i32 GetFirstFrameState();         // 0x152570  lowest frame's owned draw type
    i32 GetMemoryUsage(i32 raw);      // 0x1523f0  sum decoded byte size over [min,max]
    i32 FindFrame(CImage* frame, char* outName, i32* outIndex); // linear scan + name copy
    // The bounds-checked accessor SetAllTypes/SetAllFormats inline: a frame index outside
    // [m_minIndex, m_maxIndex] yields a null frame.
    CImage* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return 0;
        }
        return reinterpret_cast<CImage*>(m_items.GetAt(index));
    }

    // Bounds-read a frame pointer against [m_minIndex, m_maxIndex] (0x15cc30, the ex
    // CSprite::GetFrame; out-of-line in the spriteresource unit).
    i32 GetFrame(i32 n); // 0x15cc30
    // The +0x0c owning parent context, typed. CLoadable models m_0c as a plain i32
    // because each of its derivatives owns a DIFFERENT parent type - retyping the
    // shared base would be wrong for the others - so the concrete type lives here, in
    // ONE accessor, instead of a cast at every use site. Both ex-views (CSprite::m_c /
    // CImageSet::m_owner) proved the type: it is handed to each created frame as
    // CImage::m_parent (`new CImage(index, m_owner)`).
    CImageParent* Owner() const {
        return reinterpret_cast<CImageParent*>(m_0c);
    }
    void SetOwner(CImageParent* p) {
        m_0c = reinterpret_cast<i32>(p);
    }
    void DeleteAll(); // 0x151eb0  delete every owned element, RemoveAll, seed sentinels
    void AddFrameAt_1521c0(void* elem, i32 index); // 0x1521c0  SetAtGrow + widen [m_64,m_68]

    ::CObArray m_items; // +0x10  owned-pointer array (0x14: m_pData@+0x14, m_nSize@+0x18)
    char m_name[0x40];   // +0x24  registry key buffer (SetKey_155810 strncpy's it,
                        //        NUL @+0x63; CDDrawWorkerRegistry removes by it)
    i32 m_minIndex;           // +0x64  cached-index sentinel (DeleteAll seeds 99999)
    i32 m_maxIndex;           // +0x68
};
VTBL(CDDrawWorker, 0x001efbe8); // ??_7CDDrawWorker@@6B@ (17-slot CLoadable-derived vtable)

#endif // GRUNTZ_CDDRAWWORKER_H
