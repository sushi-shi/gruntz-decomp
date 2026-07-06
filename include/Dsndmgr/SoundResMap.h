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

#include <Mfc.h>          // CMapStringToPtr / CString / POSITION + <windows.h>
#include <Wap32/Object.h> // Wap::CObject - the MFC-free WAP grand-base (namespace-qualified)

// The polymorphic map value: a WAP CObject-derived resource. RemoveByValue does
// `delete p`, dispatching the scalar-deleting destructor at the shared Wap::CObject
// vtable slot 1 (`call [vptr+4]`). It derives from the real engine grand-base
// (RTTI "CObject", vtable @0x5e8cb4) - slots 0/2/3/4 are the inherited CObject
// interface (declared-only, reloc-masked); CSoundRes overrides only the dtor slot 1.
SIZE_UNKNOWN(CSoundRes);
class CSoundRes : public Wap::CObject {
public:
    ~CSoundRes(); // slot 1 (scalar-deleting dtor)
};

// The registry. m_map @+0x10 (CMapStringToPtr, 0x1c bytes -> +0x10..+0x2b).
SIZE_UNKNOWN(CSoundResMap);
class CSoundResMap {
public:
    void RemoveByValue(CSoundRes* p); // 0x157b00

    char m_00[0x10];       // +0x00 (preceding state, not touched here)
    CMapStringToPtr m_map; // +0x10
};

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_SOUNDRESMAP_H
