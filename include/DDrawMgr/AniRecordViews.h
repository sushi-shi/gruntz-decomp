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
// (CAniRecordOwner / CAniMapOwner / AniSurfDesc / AniImageHost are GONE - the whole
// chain was canon: the record owner IS CDDrawSurfaceMgr (m_pool@+0x1c==m_ptrColl,
// m_04==m_drawTarget), the "owner image" IS CDDrawSubMgrPages (+0x10==m_frontPair),
// the "surface descriptor" IS CDDrawSurfacePair (+0x18==m_bpp, +0x2c==m_surface),
// and the token-map ctx IS CDDrawSubMgrLeafScan (+0x10 CMapStringToPtr == m_10 -
// the SAME Ptr-band map, supplied by its own ANI factory).)

// (CAniStrArray is GONE - 0x168e70 IS the real ?GetAt@CStringArray COMDAT; MSVC5
// emits it out-of-line naturally (by-value CString return), so the "cannot re-emit
// the real MFC method" wall was false. Direct tokens.GetAt(i) + RVA_COMPGEN pin.)

#endif // GRUNTZ_DDRAWMGR_ANIRECORDVIEWS_H
