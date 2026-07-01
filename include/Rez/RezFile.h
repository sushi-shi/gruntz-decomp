// RezFile.h - CRezFile, the WAP32 "RezMgr" open-file wrapper: a single archive
// FILE* whose lifetime is managed by a file-handle cache (CRezFileMgr) that bounds
// the number of concurrently-open handles with an LRU eviction list. Distinct from
// CRezItm (RezMgr.h): CRezItm is the buffered leaf reader (FILE* @+0x10, owner via
// CRezItmBase::m_parent @+0xc); CRezFile keeps the filename @+0x10, the FILE*
// @+0x14, and its manager @+0x18, and lazily (re)opens the handle on demand.
//
// Recovered from the four methods at 0x13cc00 (Read), 0x13cca0 (Write), 0x13cdc0
// (Open/lazy-open) and 0x13ce70 (Close). Only OFFSETS + code bytes are load-bearing
// (campaign doctrine); unproven roles keep m_<hex>.
//
//   CRezFile (this):
//     +0x10  m_name   : char*  - the filename passed to fopen.
//     +0x14  m_handle : FILE*  - the open stdio handle (0 = closed).
//     +0x18  m_mgr    : CRezFileMgr* - the owning handle cache.
//
//   CRezFileMgr (m_mgr):
//     +0x0c  m_gate     : CRezItmOwner* - the recover/keep-going gate polled on an
//                         I/O failure (slot-2 Retry, like CRezItm's owner).
//     +0x10  m_openList : CObjList {vtbl,head,tail} - the open-handle LRU; its tail
//                         (+0x18) is the eviction candidate.
//     +0x1c  m_closedList: CObjList {vtbl,head,tail} - the idle/closed list.
//     +0x28  m_openCount: i32 - currently-open handles.
//     +0x2c  m_maxOpen  : i32 - cap; when openCount > maxOpen, Open evicts the LRU.
//     +0x30  m_readonly : i32 - mode flag (read).
//     +0x34  m_write    : i32 - mode flag (write).
#ifndef SRC_REZ_REZFILE_H
#define SRC_REZ_REZFILE_H

#include <rva.h>

// The buffered-FILE stdio + heap helpers (statically-linked CRT in retail; external
// no-body so each `call rel32` is reloc-masked). __cdecl, args on the stack.
extern "C" void* Eng_fopen(const char* path, const char* mode); // 0x11f870
extern "C" i32 RezFClose(void* fp);                             // 0x11f780
extern "C" u32 RezFRead(void* buf, u32 size, u32 n, void* fp);  // 0x18c220
extern "C" i32 RezFSeek(void* fp, i32 off, i32 origin);         // 0x18c3a0
extern "C" u32 RezFWrite(void* buf, u32 size, u32 n, void* fp); // 0x18cb40
extern "C" i32 Eng_fflush(void* fp);                            // 0x125b50
extern "C" void RezFree(void* p);

// The three fopen mode strings the lazy-open selects by the manager's flags:
//   s_rb     "rb"  (read-only:  m_write==0 && m_readonly!=0)  - shared w/ SoundDevice
//   s_rPlusB "r+b" (read/write: m_write==0 && m_readonly==0)
//   s_wPlusB "w+b" (write:      m_write!=0 && m_readonly==0)
DATA(0x0020b668)
extern const char s_rb[];
DATA(0x0021a0a4)
extern const char s_rPlusB[];
DATA(0x0021a0a8)
extern const char s_wPlusB[];

// The recover/keep-going gate (CRezItmBase's owner shape): slot-2 Retry returns
// nonzero to retry an I/O op, zero to give up.
#include <Rez/RezItmOwner.h>

class CRezFile;

// The intrusive open/closed LRU list embedded at CRezFileMgr+0x10 / +0x1c. Remove
// (0x1852e0, the engine CObjList::Remove) unlinks a node; Append (0x1851e0) splices
// one in. Both __thiscall on the {vtbl,head,tail} head; external no-body so the
// `lea/add ecx,&list; push node; call` shapes fall out, reloc-masked.
struct CObjList {
    void* m_vtbl;     // +0x00
    CRezFile* m_head; // +0x04
    CRezFile* m_tail; // +0x08  (the LRU eviction candidate when read off +0x18)

    void Remove(void* node); // 0x1852e0
    void Append(void* node); // 0x1851e0
};

// The file-handle cache that owns + bounds the open CRezFile handles.
struct CRezFileMgr {
    char m_pad0[0x0c];
    CRezItmOwner* m_gate;  // +0x0c
    CObjList m_openList;   // +0x10  (0x10..0x1b; tail @+0x18)
    CObjList m_closedList; // +0x1c  (0x1c..0x27)
    i32 m_openCount;       // +0x28
    i32 m_maxOpen;         // +0x2c
    i32 m_readonly;        // +0x30
    i32 m_write;           // +0x34

    // Close every currently-open handle (0x13ca80): walk the open list (its head
    // @+0x14), Close()-ing each until the list drains. Returns 1.
    i32 CloseAllOpen();
};
SIZE_UNKNOWN(CRezFileMgr);

// ---------------------------------------------------------------------------
// CRezFile - one managed archive FILE*.
// ---------------------------------------------------------------------------
class CRezFile {
public:
    // Read `count` bytes at `pos` into buf, ensuring the handle is open and
    // recovering through the manager's gate on seek/short-read failure (0x13cc00).
    i32 Read(i32 a, i32 pos, u32 count, void* buf);

    // Write `count` bytes from buf at `pos`, same gating (0x13cca0); the write
    // counterpart of Read (RezFWrite instead of RezFRead).
    i32 Write(i32 a, i32 pos, u32 count, void* buf);

    // Flush the handle (0x13cd60): fflush retrying through the manager's gate.
    // Returns 1 (no handle / flushed) or 0 (the gate gave up).
    i32 Flush();

    // Lazily (re)open the handle (0x13cdc0): evict the LRU if over the cap, fopen
    // with the flag-selected mode, retrying through the gate; on success move from
    // the closed list to the open list and bump the open count. Returns 1 (already
    // open / opened) or 0 (gave up / invalid mode combo).
    i32 Open();

    // Close the handle (0x13ce70): fclose retrying through the gate, then drop the
    // open count, move back to the closed list, and null the handle. Returns 1
    // (no handle / closed) or 0 (the gate gave up).
    i32 Close();

    char m_pad0[0x10];
    char* m_name;       // +0x10
    void* m_handle;     // +0x14  FILE*
    CRezFileMgr* m_mgr; // +0x18
};
SIZE_UNKNOWN(CRezFile);

#endif // SRC_REZ_REZFILE_H
