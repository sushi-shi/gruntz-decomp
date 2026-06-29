// FileMem.h - CFileMem, the engine's CFile-derived save/pack file STREAM. It
// wraps a CFileIO (the KERNEL32 file at +0x10) behind a CFile-style virtual
// interface, presenting a named, position-tracked stream. Used by the "Gruntz
// Save Game" loader (FUN_00556020) and friends.
//
// Two-level hierarchy recovered from the lifecycle (two manual vtables):
//   CFileMemBase  - vtable 0x005efe68 (base sub-object ctor 0x00157850):
//       +0x00  m_vtbl : vtable pointer (manual-stamped; CObject-style two-phase)
//       +0x04  m_4    : i32
//       +0x08  m_8    : i32
//       +0x0c  m_name : the stream name (MFC CString)
//   CFileMem : CFileMemBase - vtable 0x005efe30 (derived dtor 0x00157980):
//       +0x10  m_file : underlying CFileIO (the physical KERNEL32 file)
//       +0x20  m_length : current length / position
//       +0x24  m_offset : bytes consumed / current offset
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). The CString member (a class with a dtor)
// makes ctor/dtor carry the MSVC5 C++ EH frame -> the TU is built /GX (mfc
// profile, /O1: MFC-derived CFile-clone code).
#ifndef SRC_IO_FILEMEM_H
#define SRC_IO_FILEMEM_H
#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CString + <windows.h>

#include <Io/FileStream.h> // CFileIO (the embedded physical file)

// ---------------------------------------------------------------------------
// CFileMemBase - the base sub-object (vtable 0x005efe68): a named stream with
// two scalar fields. The manual vtable stamp keeps the ctor/dtor byte-exact
// while the full vtable (slots into still-unmatched methods) can't be emitted.
// ---------------------------------------------------------------------------
class CFileMemBase {
public:
    CFileMemBase();

    void* m_vtbl;   // +0x00
    i32 m_4;        // +0x04
    i32 m_8;        // +0x08
    CString m_name; // +0x0c
};

// ---------------------------------------------------------------------------
// CFileMem - the derived stream (vtable 0x005efe30): embeds a CFileIO at +0x10
// and tracks a position pair at +0x20/+0x24.
// ---------------------------------------------------------------------------
class CFileMem : public CFileMemBase {
public:
    ~CFileMem();
    void Reset();                                // 0x00157a50 - zero fields + Empty name (derived)
    i32 SetName(const char* name, i32 a, i32 b); // 0x00165e30
    i32 Open();                                  // 0x00165e60
    i32 Ready();                                 // 0x00165ef0 (returns 1; pokes inner +0x54)
    i32 Read(void* buf, i32 n);                  // 0x00165f00
    i32 Write(const void* buf, i32 n);           // 0x00165f50

    CFileIO m_file; // +0x10
    i32 m_length;   // +0x20
    i32 m_offset;   // +0x24

    // The base sub-object's own Close/Reset (vtable 0x005efe68 slot +0xc).
    void ResetBase(); // 0x00157a40
};

#endif // SRC_IO_FILEMEM_H
