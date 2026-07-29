#ifndef GRUNTZ_CDDRAWWORKER_H
#define GRUNTZ_CDDRAWWORKER_H
#include <rva.h>

#include <Ints.h>
#include <DDrawMgr/ShadeTableCache.h> // CShadeTable - the per-frame shade table
#include <Gruntz/Loadable.h>          // canonical CLoadable : CWapObj : CObject (9-slot base)
#include <Image/CImage.h>             // CImage COMPLETE - GetAt downcasts the CObArray band element
                                      // (CImage.h pulls only rva/Ints/WapObj, so there is no cycle
                                      //  and no weight: WapObj is already in via Loadable.h)

struct PidHeader; // the descriptor the CreateFrame slots take
class CImage;     // <Image/CImage.h>

class CSymTab;          // Bute/SymTab.h - the name->record table slots 10/15 iterate
struct CParseSource;    // Gruntz/ParseSource.h - the leaf parse record slot 16 reloads
class CDDrawSurfaceMgr; // the +0x0c owning world manager handed to each frame
                        // (== CImage::m_parent; the CImageParent pad-view is dissolved)

class CDDrawWorker : public CLoadable {
public:
    // INLINE (owner, id) ctor - BYTE-PROVEN by CDDrawWorkerRegistry::DispatchKeyed38
    // @0x154ae0 (and its three siblings 34/30/2C), which expand the whole
    // `new CDDrawWorker` in place: `push 0x6c; call ??2@YAPAXI@Z`, then the CLoadable
    // (owner, id) base inline (??_7CLoadable stamp + m_id/m_flags/m_ownerCtx), then the
    // +0x10 CObArray member ctor, then the ??_7CDDrawWorker stamp, then this body's two
    // sentinel stores (0x1869f = 99999 / 0). The arg ORDER is observable: cl5
    // materializes actual arguments right-to-left even when inlining, and retail loads
    // the registry's map COUNT (`mov eax,[edi+0x1c]`) BEFORE the owner
    // (`mov edi,[edi+0xc]`) - so `id` is the RIGHTMOST parameter.
    CDDrawWorker(CDDrawSurfaceMgr* owner, i32 id) : CLoadable(owner, id) {
        m_minIndex = 99999;
        m_maxIndex = 0;
    }
    virtual ~CDDrawWorker() OVERRIDE; // slot 1 (scalar-deleting dtor)
    // slots 5/7/8: CDDrawWorker's own overrides of the CLoadable defaults (ground
    // truth = the retail 0x1efbe8 vtable): IsLoaded @0x155750, Unload @0x151eb0 =
    // the frame teardown (defined in WwdGameObject.cpp; was ALSO declared here as a
    // duplicate non-virtual "DeleteAll" - one body, one name now), GetClassId
    // @0x155770 -> CLASSID_WORKER. Slot 6 IsReady stays inherited (0x001c08).
    virtual i32 IsLoaded() OVERRIDE; // [5] @0x155750  m_0c && m_04 != -1
    // slot 6 IsReady (0x001c08) is INHERITED from CLoadable (same body RVA;
    // audit: redeclare-nothing).
    // [7] @0x151eb0: delete every owned frame, RemoveAll, seed [min,max] sentinels.
    virtual void Unload() OVERRIDE;
    virtual i32 GetClassId() OVERRIDE; // [8] @0x155770  -> CLASSID_WORKER
    // slots 9-16: the 8 new virtuals CDDrawWorker adds over CLoadable's 9-slot base.
    // Declared-only => cl emits the full 17-slot ??_7CDDrawWorker @0x1efbe8 (this
    // realizes the vtable per the all-vtables mandate; was a bare VTBL() manual ref).
    // Bodies live at their retail RVAs (reloc-masked). Slots 10 (0x1521f0, byte-exact)
    // and 15 (0x1522b0, @early-stop regalloc wall) are HOMED in DDrawWorker.cpp: they
    // walk a CSymTab scope and self-dispatch InsertFrame (slot 14) / ReloadFrame
    // (slot 16). m_0c is the owning CDDrawSurfaceMgr (single-frame flag m_flags&0x100).
    virtual i32 SetKey(const char* key);             // slot 9  @0x155810 (key copy)
    virtual i32 BuildFramesFromSymTab(CSymTab* tab); // slot 10 @0x1521f0
    // The three frame-create slots each seed a CImage and run ONE of its loader
    // virtuals; their leading args are just that loader's args, forwarded verbatim
    // (retail 0x152166 / 0x1520b6 / 0x152006 push them straight through). So their
    // types are the loader's - see the proof block over CImage::Create24 /
    // LoadDispatch / Create in <Image/CImage.h> (SETTLED 2026-07-27):
    //   slot 11 -> CImage::Create24    (a1/a2 = width/height)
    //   slot 12 -> CImage::LoadDispatch (a1 = blob desc, a2 = mode, a4 = blob LENGTH)
    //   slot 13 -> CImage::Create      (a1 = file PATH)
    virtual CImage* CreateFrame24(i32 width, i32 height, i32 index, i32 keyed); // slot 11 @0x152110
    virtual CImage*
    CreateFrame28(PidHeader* desc, i32 mode, i32 index, u32 size);   // slot 12 @0x152060
    virtual CImage* CreateFrame30(char* path, i32 index, i32 keyed); // slot 13 @0x151fb0
    // FOLD (stage 4, DONE for CDDrawWorker): the ex `CDDrawWorker` (<Gruntz/Sprite.h>) IS this
    // class - it is now a typedef of it. Slot 14's body @0x151f00 was declared as
    // ?InsertFrame@CDDrawWorker@@ while BEING this vtable's slot-14 body: its own code reads
    // the frame CObArray at +0x10 (m_pData@+0x14 / m_nSize@+0x18) and the owner at
    // +0x0c - i.e. m_items/m_owner, offset for offset - and it already cast its own
    // array to the real ::CObArray to call SetAtGrow. Declaring it here as the real
    // virtual retires that WIRING row. The return type is CImage* (the body's own
    // mangled name says PAVCImage, not the `i32` this slot used to be declared with).
    //
    // FOLD (stage 5, DONE for CDDrawWorker): that third view of this same 0x6c object is
    // dissolved too - it is now a typedef, and slots [11]/[12]/[13] (0x152110/0x152060/
    // 0x151fb0), which it had declared as CDDrawWorker::CreateFrame24/28/30, are this
    // class's own virtuals. Their return type is CImage* (the bodies' mangled names say
    // PAVCImage), not the `i32` the slots used to be declared with.
    virtual CImage* InsertFrame(void* rec, i32 n, i32 flag); // slot 14 @0x151f00
    virtual i32 ValidateFramesFromSymTab(CSymTab* tab);      // slot 15 @0x1522b0
    // slot 16 @0x1523b0. `rec` IS a CParseSource*: the only caller is slot 15
    // (ValidateFramesFromSymTab @0x1522b0) walking a CSymTab scope, whose payload it
    // already reads as CParseSource (GetEntryTag / m_name), and the body hands it
    // straight to CImage::Reload(CParseSource*, i32). Was declared i32, which forced a
    // ptr->int cast at the call and an int->ptr cast in the body.
    virtual i32 ReloadFrame(CParseSource* rec, i32 n, i32 flag);
    // ---- the ex-CDDrawWorker non-virtual methods (stage 5 of the fold; bodies in
    // wwdgameobject at their retail RVAs). They were declared on a THIRD view of this
    // same 0x6c object; CreateFrame24/28/30 above are this vtable's own slots 11/12/13.
    i32 SetAllTypes(i32 type);             // 0x152480  walk [min,max], set each frame's draw type
    i32 SetAllFormats(CShadeTable* shade); // 0x152520
    i32 SetAllField18(i32 value);          // 0x1524d0  walk [min,max], set each owned light level
    i32 GetFirstFrameState();              // 0x152570  lowest frame's owned draw type
    i32 GetMemoryUsage(i32 raw);           // 0x1523f0  sum decoded byte size over [min,max]
    i32 FindFrame(CImage* frame, char* outName, i32* outIndex); // linear scan + name copy
    // The bounds-checked accessor SetAllTypes/SetAllFormats inline: a frame index outside
    // [m_minIndex, m_maxIndex] yields a null frame.
    CImage* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return 0;
        }
        // CImage : CWapObj : CObject, so the CObArray band element is a plain downcast.
        return static_cast<CImage*>(m_items.GetAt(index));
    }

    // Bounds-read a frame pointer against [m_minIndex, m_maxIndex] (0x15cc30, the ex
    // CDDrawWorker::GetFrame; out-of-line in the spriteresource unit).
    CImage* GetFrame(i32 n); // 0x15cc30
    // The +0x0c owning parent context. It is the CDDrawSurfaceMgr: MakeWorker
    // (DDrawWorkerRegistry.cpp) copies it straight out of the registry's own
    // m_ownerCtx, and the same word is handed to every frame as CImage::m_parent
    // (`new CImage(index, Owner())`) where the three fields it reaches - +0x04
    // m_drawTarget, +0x1c m_ptrColl, +0x24 m_level - are the manager's. That is
    // exactly what OwnerMgr() already returns, so this is the base accessor, retyped.
    CDDrawSurfaceMgr* Owner() const {
        return OwnerMgr();
    }
    // (SetOwner deleted with the CImageParent fold - it had zero callers and existed
    // only to hold a reinterpret back into the i32 slot.)
    void AddFrameAt(void* elem, i32 index); // 0x1521c0  SetAtGrow + widen [m_64,m_68]

    CObArray m_items;  // +0x10  owned-pointer array (0x14: m_pData@+0x14, m_nSize@+0x18)
    char m_name[0x40]; // +0x24  registry key buffer (SetKey strncpy's it,
                       //        NUL @+0x63; CDDrawWorkerRegistry removes by it)
    i32 m_minIndex;    // +0x64  cached-index sentinel (Unload seeds 99999)
    i32 m_maxIndex;    // +0x68
};
SIZE(0x6c);

#endif // GRUNTZ_CDDRAWWORKER_H
