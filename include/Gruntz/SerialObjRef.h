// SerialObjRef.h - a serialized reference to a registry object BY NAME. This is
// the small record embedded at +0x34 of every serializable CUserLogic tile-logic
// object; its Chain (0x8c00, __thiscall ret 0x10) is run by each leaf's Serialize
// override (CSecretTeleporterTrigger, CFortressFlag, RollingBall, CCursorSnapSprite,
// the eyecandy/candy leaves, ...) to persist WHICH registry object this logic points
// at, keyed by that object's registry NAME.
//
// Chain(arc, mode, unused, obj): mode 7 = READ (read an 0x80-byte key name + a
// 0x10-byte blob from the archive, then resolve that name through obj's class
// registry into the referenced value m_value); mode 4 = WRITE (re-derive the
// value's name from the registry, write the name back + the 0x10-byte blob). The
// CString temp on the write path is inner-scoped + RVO so /GX elides the EH frame
// (the function is frameless in retail).
//
// The registry is the canonical CDDrawSubMgrLeaf reached via obj->m_7c->m_0c->m_leaf: its name
// map (a ::CMapStringToPtr at +0x10) resolves the key (m_10.Lookup), and its
// KeyOfValue_152d30 turns a value pointer back into its key CString. Both are the
// real symbols in CDDrawSubMgrLeaf.cpp; modeled here so the reloc-masked calls pair
// by mangled name.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTZ_CSERIALOBJREF_H
#define GRUNTZ_GRUNTZ_CSERIALOBJREF_H

#include <rva.h>

#include <Mfc.h> // CObject / CMapStringToOb / CString + <windows.h>

// The serialize/archive object (param1): the shared WAP32 stream interface (Read @
// slot +0x2c / Write @ slot +0x30), now a real declared-only virtual class.
#include <Gruntz/SerialArchive.h>

// The class-name registry is the CANONICAL CDDrawSubMgrLeaf (the ODR-duplicate
// local def here is folded, 2026-07-16): the name->value map is its m_10
// (::CMapStringToPtr, Lookup 0x1b8438 - the mfc_class-proven band) and
// KeyOfValue_152d30 turns a value back into its key CString.
#include <DDrawMgr/DDrawSubMgrLeaf.h>

// (The former CSerialRegHolder / CSerialNameHolder / CSerialObj shell chain is
// its +0x7c the AnimWorkerObj, the worker's m_0c the CDDrawSurfaceMgr, and the
// +0x2c registry its m_leaf - obj->m_7c->m_0c->m_leaf, all typed.)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/UserLogic.h>

// ---------------------------------------------------------------------------
// The serialized-object-reference record itself (embedded at +0x34).
class CSerialObjRef {
public:
    // Serialize the referenced object by its registry key name (read/write per mode).
    i32 Chain(CSerialArchive* arc, i32 mode, i32 unused, CGameObject* obj); // 0x8c00

    CGameObject* m_00;   // +0x00  the referenced object
    CGameObject* m_04;   // +0x04  (== m_00)
    AnimWorkerObj* m_08; // +0x08  obj->m_7c
    CObject* m_value;    // +0x0c  resolved value (registry Lookup result)
    char m_blob[0x10];   // +0x10  the 0x10-byte serialized blob
};
SIZE_UNKNOWN(CSerialObjRef);

#endif // GRUNTZ_GRUNTZ_CSERIALOBJREF_H
