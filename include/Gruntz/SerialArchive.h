// SerialArchive.h - the shared WAP32 serialization/archive stream interface driven
// by every CUserLogic-derived Serialize override (CMenuSparkle, CMovingLogic,
// CMgrSettings, CSerialSub34, ...). A polymorphic CArchive-like stream whose vtable
// holds the general read slot @ +0x2c (mode 7) and write slot @ +0x30 (mode 4).
//
// Modeled declared-only real-virtual (11 padder slots then Read/Write, never defined
// here -> no ??_7 emitted) so `arc->Read(buf,n)` lowers to the exact
// `mov eax,[arc]; push n; push buf; mov ecx,arc; call [eax+0x2c]` __thiscall
// dispatch. This is the single shared model that replaces the former per-TU typed-PMF
// views (CMsSerialArchiveVtbl / CMgrArchiveVtbl / CMlSerialArchiveVtbl /
// CSerialArchiveVtbl). Same shape as CMapArchive (MapLogic.h). The archive object is
// created elsewhere (passed in as a param), so this is a dispatch interface, not an
// own-class vtable stamp. Field/method names are placeholders; only offsets + emitted
// bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_SERIALARCHIVE_H
#define GRUNTZ_SERIALARCHIVE_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CSerialArchive);
struct CSerialArchive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Read(void* buf, i32 n);  // +0x2c  (mode 7)
    virtual void Write(void* buf, i32 n); // +0x30  (mode 4)
};

#endif // GRUNTZ_SERIALARCHIVE_H
