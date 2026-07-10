// FecCrypt.h - CFecFile, the "FEC File" encrypted-resource archive reader/writer
// (0x17b5f0 references the "Opened FEC File %s" diagnostics) AND the CMoviePlayer decode
// store (it is CMoviePlayer's embedded m_540 sub-object). Formerly modeled under three
// names - CFecFile (FecCrypt.cpp), CPageStore17b510 (BoundaryUpper2Views.h) and
// CMovieDecodeStore (MoviePlayer.h) - all the SAME 0x200-byte object (same +0x124
// polymorphic stream, +0x138 index). Unified here as the single-source shape.
//
// Field names are placeholders; the offsets, the vtable SLOT offsets and the code bytes
// are the load-bearing facts (campaign doctrine).
#ifndef CRYPTO_FECCRYPT_H
#define CRYPTO_FECCRYPT_H

#include <Ints.h>

// The stream subobject at CFecFile+0x124: a polymorphic file/byte source. Its virtuals
// are DECLARED only (foreign vtable, reloc-masked dispatch). Slots: Open @+0x28,
// Seek @+0x30, Read @+0x3c, Close @+0x50, Discard @+0x54 (all __thiscall by default).
class FecStream {
public:
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
    virtual i32 Open(const char* name, i32 a2, i32 a3); // +0x28
    virtual void Slot2C();                              // +0x2c
    virtual i32 Seek(i32 off, i32 origin);              // +0x30
    virtual void Slot34();
    virtual void Slot38();
    virtual i32 Read(void* buf, i32 size);        // +0x3c
    virtual i32 Write(const void* buf, i32 size); // +0x40
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Close();   // +0x50
    virtual void Discard(); // +0x54  (OnFail drops the active stream through this)
};

// The helper at CFecFile+0x138 (its Op at 0x1b4d7c is a reloc-masked rel32 call). Also
// carries a RemoveAt (0x1b4bad, the MFC-array remove used by Init/Close), so it is an
// array-backed container reached __thiscall.
struct FecIndex {
    void Op(i32 a1, i32 a2);            // 0x1b4d7c  __thiscall
    void RemoveAt(i32 idx, i32 count); // 0x1b4bad  __thiscall (Init/Close reset)
    void* m_00;
    i32 m_04;
    i32 m_08; // +0x08  read as the first Op arg in the loop
};

class CFecFile {
public:
    // Init/Close/Lookup are the CMoviePlayer-driven decode-store lifecycle (this same
    // object is CMoviePlayer's m_540 "decode store"): Init resets + arms the store,
    // Close tears it down, Lookup resolves an entry offset. OnFail (0x17b5a0) is the
    // read-fail teardown ReadArchive/Close both call.
    i32 Init();                          // 0x17b510  reset + arm (open-gate -> 1)
    void Close();                        // 0x17b570  teardown (OnFail + reset + gate -> 0)
    i32 Lookup(u32 idx);                 // 0x17b840  resolve entry idx (1-based) -> m_128
    i32 CreateArchive(const char* name); // 0x17b8a0
    i32 ReadArchive(const char* name);   // 0x17b5f0
    i32 OnFail();                        // 0x17b5a0

    i32 m_00;           // +0x00  open-gate (must be nonzero)
    i32 m_04;           // +0x04  already-open flag
    i32 m_08;           // +0x08
    i32 m_0c;           // +0x0c  version major (12-byte header word 0)
    i32 m_10;           // +0x10  version minor
    i32 m_14;           // +0x14  entry count
    char m_18[0x10c];   // +0x18  per-entry record (m_11e WORD + m_120 stride inside)
    FecStream m_stream; // +0x124
    i32 m_128;          // +0x128  Lookup's success result
    i32 _pad12c[(0x134 - 0x12c) / 4];
    i32 m_134;        // +0x134  cleared by Init/OnFail
    FecIndex m_index; // +0x138
    i32* m_13c;       // +0x13c  per-entry offset table (dwords)
    i32 m_140;        // +0x140
};

#endif // CRYPTO_FECCRYPT_H
