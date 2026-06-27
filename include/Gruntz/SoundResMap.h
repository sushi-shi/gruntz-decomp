// SoundResMap.h - a string-keyed resource registry that supports remove-by-value
// (C:\Proj\Gruntz; the 0x157bxx region, reached from LoadObjectSoundResources at
// 0x09a910). The owner embeds an MFC CMapStringToPtr at +0x10 (keyed by a name,
// value = a polymorphic CObject-derived resource with a virtual destructor) and
// exposes RemoveByValue(p): walk the map for the entry whose value == p, RemoveKey
// it, then `delete p` (the virtual scalar-deleting destructor at vtbl slot 1).
//
// Recovered shape: m_map (CMapStringToPtr) sits at +0x10 so m_nCount lands at +0x1c
// (the inlined GetStartPosition reads it). Only OFFSETS + emitted bytes are
// load-bearing; field/class names are placeholders (campaign doctrine).
#ifndef GRUNTZ_SOUNDRESMAP_H
#define GRUNTZ_SOUNDRESMAP_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CMapStringToPtr / CString / POSITION + <windows.h>

// The polymorphic map value: a resource whose virtual destructor is the SECOND
// virtual (vtbl slot 1), so `delete p` dispatches `call [vptr+4]` with the scalar-
// deleting flag. Modeled with no bodies (reloc-masked indirect call).
class CSoundRes {
public:
    virtual void vf0(); // slot 0 (unused here, fixes the dtor at slot 1)
    virtual ~CSoundRes();
};

// The registry. m_map @+0x10 (CMapStringToPtr, 0x1c bytes -> +0x10..+0x2b).
class CSoundResMap {
public:
    void RemoveByValue(CSoundRes* p); // 0x157b00

    char m_00[0x10];       // +0x00 (preceding state, not touched here)
    CMapStringToPtr m_map; // +0x10
};

#endif // GRUNTZ_SOUNDRESMAP_H
