// CSerialObjRef.h - a serialized reference to a registry object BY NAME. This is
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
// The registry is a CDDrawSubMgrLeaf reached via obj->m_7c->m_0c->m_2c: its name
// map (CMapStringToOb at +0x10) resolves the key (m_10.Lookup), and its
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

// The class name registry (matches CDDrawSubMgrLeaf.cpp): the name->value map sits
// at +0x10 (CMapStringToOb::Lookup is the 0x1b8438 call); KeyOfValue_152d30 turns a
// value back into its key CString (FUN_00552d30, RVO return). Local view; only the
// class name + signatures matter for the reloc-masked symbols.
class CDDrawSubMgrLeaf {
public:
    CString KeyOfValue_152d30(CObject* target); // 0x152d30

    char _00[0x10];
    CMapStringToOb m_10; // +0x10  the name map (Lookup at 0x1b8438)
};

// The name-holder reached through obj->m_7c->m_0c: its +0x2c is the registry leaf.
class CSerialRegHolder {
public:
    char _00[0x2c];
    CDDrawSubMgrLeaf* m_2c; // +0x2c  registry leaf
};
class CSerialNameHolder {
public:
    char _00[0x0c];
    CSerialRegHolder* m_0c; // +0x0c
};

// The referenced game object (param4): its m_7c is the name-holder above.
class CSerialObj {
public:
    char _00[0x7c];
    CSerialNameHolder* m_7c; // +0x7c
};

// ---------------------------------------------------------------------------
// The serialized-object-reference record itself (embedded at +0x34).
class CSerialObjRef {
public:
    // Serialize the referenced object by its registry key name (read/write per mode).
    i32 Chain(CSerialArchive* arc, i32 mode, i32 unused, CSerialObj* obj); // 0x8c00

    CSerialObj* m_00;        // +0x00  the referenced object
    CSerialObj* m_04;        // +0x04  (== m_00)
    CSerialNameHolder* m_08; // +0x08  obj->m_7c
    CObject* m_value;        // +0x0c  resolved value (registry Lookup result)
    char m_blob[0x10];       // +0x10  the 0x10-byte serialized blob
};
SIZE_UNKNOWN(CSerialObjRef);

#endif // GRUNTZ_GRUNTZ_CSERIALOBJREF_H
