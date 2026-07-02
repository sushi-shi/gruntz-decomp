// CSerialSub34.h - the shared "+0x34 serializable sub-object" whose Chain
// (0x8c00, __thiscall ret 0x10) is invoked by every CUserLogic-derived Serialize
// override (CSecretTeleporterTrigger, CFortressFlag, RollingBall,
// CCursorSnapSprite, ...). It is a small record holding a referenced game-object
// and the looked-up value for a key serialized through the archive.
//
// Chain(arc, mode, unused, obj): mode 7 = READ (read an 0x80-byte key name + a
// 0x10-byte blob from the archive, then resolve the key through obj's class
// registry into m_c); mode 4 = WRITE (re-derive the value's name from the registry,
// write it back + the 0x10-byte blob). The CString temp on the write path is inner-
// scoped + RVO so /GX elides the EH frame (the function is frameless in retail).
//
// The registry is a CDDrawSubMgrLeaf reached via obj->m_7c->m_0c->m_2c: its name
// map (CMapStringToOb at +0x10) resolves the key (m_10.Lookup), and its
// KeyOfValue_152d30 turns a value pointer back into its key CString. Both are the
// real symbols in CDDrawSubMgrLeaf.cpp; modeled here with a matching local view so
// the reloc-masked calls pair by mangled name.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTZ_CSERIALSUB34_H
#define GRUNTZ_GRUNTZ_CSERIALSUB34_H

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
// The +0x34 sub-object record itself.
class CSerialSub34 {
public:
    i32 Chain(CSerialArchive* arc, i32 mode, i32 unused, CSerialObj* obj); // 0x8c00

    CSerialObj* m_00;        // +0x00  the referenced object
    CSerialObj* m_04;        // +0x04  (== m_00)
    CSerialNameHolder* m_08; // +0x08  obj->m_7c
    CObject* m_0c;           // +0x0c  resolved value (Lookup result)
    char m_10[0x10];         // +0x10  the 0x10-byte serialized blob
};

#endif // GRUNTZ_GRUNTZ_CSERIALSUB34_H
