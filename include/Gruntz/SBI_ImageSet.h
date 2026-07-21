#ifndef GRUNTZ_SBI_IMAGESET_H
#define GRUNTZ_SBI_IMAGESET_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_Image.h> // canonical frameless CSBI_Image base (real RTTI base)

class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and

#include <Gruntz/SerialArchive.h>
typedef CSerialArchive CImageSetStream;

class CSBI_ImageSet : public CSBI_Image {
public:
    // tag 4 + the resolved-record slot cleared (the `new CSBI_ImageSet` ctor leg the tab
    // builders fold at each new-site, over the out-of-line CSBI_RectOnly base ctor).
    CSBI_ImageSet() {
        m_frame = 0;
        m_kind = 4;
        m_34 = 0;
    }
    virtual ~CSBI_ImageSet() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1eac4c thunk 0x3ca1 -> 0xe74f0): chains CSBI_Image::SerializeFields.
    virtual i32 SerializeFields(CSerialArchive* ar, i32 mode, i32 a3, i32 a4) OVERRIDE; // 0xe74f0
    virtual void SbiSlot3() OVERRIDE;                                                   // slot 3
    virtual void SbiSlot4() OVERRIDE;  // slot 4
    virtual void SbiSlot5() OVERRIDE;  // slot 5
    virtual i32
    SetupImage(CStatusBarMgr*, CDDrawSurfaceMgr*, i32, i32, SbRect, const char*, i32, i32)
        OVERRIDE; // slot 11
    // slot 12 (new), body 0x0e74c0 (a Ghidra recovery gap - not yet reconstructed). It takes
    // ONE arg: the game-menu builder calls it as `Activate(7)` on the DESTRUCT item.
    // slot 12 (+0x30; shared default body 0xe74c0 - the level that grows the vtable
    // to 13): the (on) notify hook the status-bar walkers drive on the notify fields.
    virtual void Notify(i32 on);
    // (0xe74f0 was declared here as a non-virtual `Serialize` - it IS the slot-1
    //  SerializeFields override above. The 0xe6e40 base leg is CSBI_Image's - SBI_Image.h.)
    // slot-3 body (vtbl 0x1eac4c slot [3], thunk 0x2a09): reset the resolved record +
    // latched value. Re-attributed from the SBI_RectOnly host TU (dossier #16).
    // This IS the member teardown the CHAIN-DTOR device runs from every ~CSBI_X below
    // CSBI_ImageSet: retail's ~CSBI_ImageSet / ~CSBI_ImageSetAni / ~CSBI_StatzTabArrow /
    // ~CSBI_WarlordHead each `call 0xe7400` at their own level AND again at every folded
    // base level - the ONE inherited non-virtual helper, not a per-class dtor leg. (The
    // four declared-only aliases DtorImageSet/DtorImageSetAni/DtorStatzTabArrow/DtorReset
    // were fake views of this exact function - unbound symbols that would not link.)
    void ResetCounters(); // 0xe7400
    // slot-5 body (vtbl 0x1eac4c slot [5], thunk 0x2e78): one play step re-resolving the
    // frame from the record table. Ex CAniPlayer view (dossier #16).
    i32 TickRenderFrame(); // 0xe7440

    CImageSet* m_34; // +0x34  resolved config record (the real image-registry CImageSet)
    i32 m_38;      // +0x38  serialized config id (4 bytes)
};
SIZE(CSBI_ImageSet, 0x3c);

VTBL(CSBI_ImageSet, 0x001eac4c);

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGESET_DTOR)
inline CSBI_ImageSet::~CSBI_ImageSet() {
    ResetCounters();
}
#endif

#endif // GRUNTZ_SBI_IMAGESET_H
