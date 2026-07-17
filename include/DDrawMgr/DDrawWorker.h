// DDrawWorker.h - an owned-collection node of the DDrawMgr "DDraw worker"
// family (placeholder name; engine "tomalla-35"). Non-RTTI engine class; its
// own primary vtable is at RVA 0x1efbe8 (g_ddrawWorkerVtbl). It derives from a
// CLoadable-shaped base subobject (m_04/m_08/m_0c reset on teardown, then the
// CObject-like grand-base dtor vtable g_wapObjectDtorVtbl @0x5e8cb4 restored).
//
// Layout recovered from the dtor (0x1557a0) + DeleteAll helper (0x151eb0):
//   +0x00 vptr (CLoadable)
//   +0x04 m_04  (reset to -1 on teardown)
//   +0x08 m_08  (reset to 0)
//   +0x0c m_0c  (reset to 0)
//   +0x10 m_items  CObArray of owned CObject* (vtbl 0x5ed494; m_pData@+0x14, m_nSize@+0x18)
//   +0x64 m_64  cached-index sentinel (DeleteAll seeds it to 99999 = 0x1869f)
//   +0x68 m_68  (reset to 0 by DeleteAll)
// Only the offsets + emitted bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CDDRAWWORKER_H
#define GRUNTZ_CDDRAWWORKER_H
#include <rva.h>

#include <Ints.h>
#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : CObject (9-slot base)

// DISSOLVED (Fable A2, 2026-07-14): the former "CWorkerElement" 14-slot shell WAS
// the canonical CImage (<Image/CImage.h>, ??_7 @0x5eaa2c, 18 slots): the elements
// of m_items are built by CSprite::InsertFrame as `new CImage(n, m_c)`, walked as
// CImage* by CImageSet::GetMemoryUsage, and the shell's two live dispatches map
// slot-for-slot - "Delete(1)" = the slot-1 scalar-deleting dtor (`delete el`),
// "Query34(rec, flag)" (+0x34, slot 13) = CImage::Reload(CParseSource*, i32)
// @0x153380. The consumers (WwdGameObject.cpp) now use CImage directly.
class CImage; // <Image/CImage.h>

// (CWorkerObArray is GONE: the +0x10 owned-pointer array IS MFC ::CObArray.  PROVEN from
//  the binary - its ctor 0x1b55e9 stamps vtable 0x1ed494, whose MFC CRuntimeClass names it
//  "CObArray".  The old view reached SetSize 0x1b5653 / SetAtGrow 0x1b5822 by casting to
//  CDWordArray*, which bound those relocs to the WRONG library symbol - CDWordArray lives
//  at [0x1b4b43, 0x1b4f0b), while both of those addresses are in the CObArray band.)

// The grand-base (CObject-like) dtor vtable, restamped manually by CWwdSpatialMgr's
// teardown (CDDrawWorkerHost.h). Same datum as ??_7CObject @0x5e8cb4.

// The "DDraw worker" base subobject is the canonical CLoadable (m_04/m_08/m_0c +
// the field-reset dtor + the grand-base 0x5e8cb4 re-stamp folded via ~CWapObj ->
// ~CObject). Its ctor/dtor fold into the leaf's, giving retail's two-phase
// vptr schedule + the destructible-base /GX frame.
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
    // walk a CSymTab scope and self-dispatch InsertFrame (slot 14) / Slot40_1523b0
    // (slot 16). m_0c is the owning CDDrawSurfaceMgr (single-frame flag m_flags&0x100).
    virtual i32 SetKey_155810(const char* key);            // slot 9  @0x155810 (key copy)
    virtual i32 BuildFramesFromSymTab(CSymTab* tab);       // slot 10 @0x1521f0
    virtual i32 CreateFrame24(i32 a, i32 b, i32 c, i32 d); // slot 11 @0x152110
    virtual i32 CreateFrame28(i32 a, i32 b, i32 c, i32 d); // slot 12 @0x152060
    virtual i32 CreateFrame30(i32 a, i32 b, i32 c);        // slot 13 @0x151fb0
    // FOLD (stage 4, DONE for CSprite): the ex `CSprite` (<Gruntz/Sprite.h>) IS this
    // class - it is now a typedef of it. Slot 14's body @0x151f00 was declared as
    // ?InsertFrame@CSprite@@ while BEING this vtable's slot-14 body: its own code reads
    // the frame CObArray at +0x10 (m_pData@+0x14 / m_nSize@+0x18) and the owner at
    // +0x0c - i.e. m_items/m_owner, offset for offset - and it already cast its own
    // array to the real ::CObArray to call SetAtGrow. Declaring it here as the real
    // virtual retires that WIRING row. The return type is CImage* (the body's own
    // mangled name says PAVCImage, not the `i32` this slot used to be declared with).
    //
    // STILL A VIEW: CImageSet (<Image/ImageSet.h>) is the same 0x6c object again -
    // slots [11]/[12]/[13] (0x152110/0x152060/0x151fb0) are declared as
    // CImageSet::CreateFrame24/28/30 and are this vtable's own slots. Stage 5.
    virtual CImage* InsertFrame(void* rec, i32 n, i32 flag); // slot 14 @0x151f00
    virtual i32 ValidateFramesFromSymTab(CSymTab* tab);  // slot 15 @0x1522b0
    virtual i32 Slot40_1523b0(i32 rec, i32 n, i32 flag); // slot 16 @0x1523b0
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
        return (CImageParent*)m_0c;
    }
    void SetOwner(CImageParent* p) {
        m_0c = (i32)p;
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

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_CDDRAWWORKER_H
