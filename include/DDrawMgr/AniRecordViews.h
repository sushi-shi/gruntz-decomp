// AniRecordViews.h - honest by-offset owner/surface models the 'ANI' frame-record
// (CAniRecordView, AniRecord.cpp) reaches by this-relative offset. NONE of these
// carry an RTTI vtable, so there is no recoverable class name - they are minimal
// by-offset models (no fabricated identity), NOT canonical classes. Scaffolding
// header (per the homing ratchet): kept out of the .cpp so it stops counting as a
// .cpp-local view, but each is an @identity-TODO to resolve once its owning
// subsystem is RTTI-pinned.
#ifndef GRUNTZ_DDRAWMGR_ANIRECORDVIEWS_H
#define GRUNTZ_DDRAWMGR_ANIRECORDVIEWS_H

#include <Ints.h>
#include <Mfc.h>                          // CMapStringToPtr (the map owner's +0x10 member)
#include <DDrawMgr/DDSurface.h>           // CDDSurface (the surface descriptor's +0x2c target)
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections (the owner's +0x1c pool)

// @identity-TODO (full xref chase, dead-ends at an RTTI-less owner of the map-worker):
// the Alloc* leaves + Slot13 that read record->m_owner have NO direct rel32 caller -
// sema xref proves they are referenced ONLY as CAniRecordBase2 vtable slots (0x1f0304 =
// ??_7CAniRecordBase2+0x2c slot 11, 0x1f030c = +0x34 slot 13). So m_owner is the owner
// of the CAniRecordBase2 "map worker" object, set by the CDDrawWorkerMapSmall factory
// that news it; the owner carries no RTTI vtable, so no class name is recoverable. Its
// +0x08 is a flags word the buffer virtuals OR a bit into; its +0x1c is the real
// CDDrawPtrCollections pool allocator the Alloc* leaves route through.
class CAniRecordOwner {
public:
    i32 m_00, m_04; // +0x00..+0x07
    i32 m_flags;    // +0x08  flags
    char _pad0c[0x1c - 0x0c];
    CDDrawPtrCollections* m_pool; // +0x1c  the pool allocator (real CDDrawPtrCollections)
};

// @identity-TODO (full xref chase, dead-ends at a void* context): the token-map owner
// (ResolveIndices arg1 / Parse ctx). sema xref 0x168c60 proves Parse's only caller is
// CAniElement::Build_165460 (mangled ?...@@QAEHPAXPAU...@@H@Z -> ctx is `void*`, no class
// name), which forwards its own `ctx` arg (supplied by the CDDrawSubMgrLeaf ANI factory) straight through.
// The context carries no RTTI vtable. Its +0x10 is the real MFC CMapStringToPtr the index
// resolver looks each token up in.
struct CAniMapOwner {
    char _pad00[0x10];
    CMapStringToPtr m_map; // +0x10
};

// @identity-TODO (full xref chase, dead-ends at RTTI-less descriptors): the owner-image /
// surface-descriptor chain PushPalette (0x168fd0) walks. sema xref 0x168fd0 proves it has no
// direct caller - it is CAniRecordBase2 vtable slot 13 (0x1f030c) only, dispatched on the
// map-worker. m_owner->m_04 is the owner image (AniImageHost), whose +0x10 is a surface
// descriptor (AniSurfDesc) carrying the source bitdepth (+0x18; 8 = paletted) and the
// target CDDSurface (+0x2c). Neither carries an RTTI vtable; only the touched offsets are
// modeled (CImage.h is deliberately avoided - its EH include tangle regresses
// AniRecord.cpp's /GX destructors).
struct AniSurfDesc {
    char pad_00[0x18];
    i32 m_18; // +0x18  source bitdepth (8 = 8bpp / paletted)
    char pad_1c[0x2c - 0x1c];
    CDDSurface* m_2c; // +0x2c  target surface
};
struct AniImageHost {
    char pad_00[0x10];
    AniSurfDesc* m_10; // +0x10  surface descriptor
};

// (CAniStrArray is GONE - 0x168e70 IS the real ?GetAt@CStringArray COMDAT; MSVC5
// emits it out-of-line naturally (by-value CString return), so the "cannot re-emit
// the real MFC method" wall was false. Direct tokens.GetAt(i) + @rva-symbol pin.)

#endif // GRUNTZ_DDRAWMGR_ANIRECORDVIEWS_H
