// MgrSettings.h - a small persisted settings record (0x3c bytes) whose Serialize
// (0x109e00) streams three ints + five doubles through a CArchive-like stream,
// then round-trips a named-object reference (a 0x80 name + an index) through the
// game registry: READ resolves name->record via CMapStringToOb::Lookup and indexes
// the record's bounded element array into m_38; WRITE re-derives the name+index
// from m_38 via CDDrawWorkerRegistry::AnyValueMatches and writes them back.
//
// Same registry round-trip shape as CSerialObjRef/the g_gameReg singleton; field
// names are placeholders (offsets + code bytes load-bearing).
#ifndef GRUNTZ_CMGRSETTINGS_H
#define GRUNTZ_CMGRSETTINGS_H

#include <Mfc.h> // real MFC CMapStringToOb / CObject (Lookup 0x1b8008, reloc-masked)
class CImage; // the resolved m_38 frame element (<Image/CImage.h>)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <Ints.h>
#include <rva.h>
// The *0x24556c singleton is NOT declared here (see the same note in <Gruntz/Play.h>):
// a header-level decl pins one view's type on every includer and blocks the
// CGameRegistry == CGruntzMgr fold. Each includer declares it with the type it needs.

// The registry leaf reached as g_gameReg->m_world->m_imageRegistry IS the canonical
// CDDrawWorkerRegistry (<DDrawMgr/DDrawWorkerRegistry.h>): same object, same
// CMapStringToOb at +0x10, and this view's AnyValueMatches_155630 is that class's
// ReadField at the SAME rva 0x155630. The 4th duplicate definition of this class is
// dissolved (it was an ODR divergence the two never-meeting TUs hid).
#include <DDrawMgr/DDrawWorkerRegistry.h>

// (The ex CMgrLookupRec/CMgrActiveHolder views are DISSOLVED: the record Lookup
// yields is the real frame-data CSprite (<Gruntz/Sprite.h>: elements @+0x14, bounds
// @+0x64/+0x68), and the +0x30 "active holder" is the real g_gameReg->m_world
// CDDrawSurfaceMgr whose +0x10 IS m_imageRegistry.)

// Per-serialize round counter the archive bumps - declared in <Gruntz/SerialCounter.h>.
#include <Gruntz/SerialCounter.h>

// The settings record itself.
SIZE_UNKNOWN(CMgrSettings);
class CMgrSettings {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x109e00

    i32 m_00, m_04, m_08, m_0c;          // +0x00..+0x0c (m_0c unstreamed)
    double m_10, m_18, m_20, m_28, m_30; // +0x10..+0x30
    CImage* m_38; // +0x38 resolved frame reference (a CImageSet frame; ex void*)
};

#endif // GRUNTZ_CMGRSETTINGS_H
