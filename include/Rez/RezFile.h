// RezFile.h - CRezFile, the WAP32 "RezMgr" managed archive FILE*: one open-file
// node whose lifetime is managed by its owning CRezDir (the directory node IS the
// file-handle cache - see <Rez/RezMgr.h>): the dir bounds the number of
// concurrently-open handles with an LRU eviction list, and CRezFile lazily
// (re)opens its handle on demand.
//
// CRezFile : CRezItmBase (0x1c bytes; own 8-slot vtable ??_7CRezFile @0x1ef7d0:
// [0] 0x13cef0 empty, [1] ??_G 0x13cb60 / ??1 0x13cb80, [2] Read 0x13cc00,
// [3] Write 0x13cca0, [4] Open-stub 0x13cd40, [5] Close-stub 0x13cd50,
// [6] Flush 0x13cd60, [7] Check-stub 0x13cdb0). The meaty handle open/close are
// the NON-virtual OpenFile (0x13cdc0) / CloseFile (0x13ce70); the slot-4/5/7
// overrides are trivial stubs (retail: return 0). The ctor (0x13cac0) heap-copies
// the filename and enrolls the node into the dir's closed list; the dtor
// (0x13cb80) closes the handle, frees the name and unlinks from the closed list.
// (This class was previously split across TWO fake identities: "CRezParseNode"
// (RezMgr.h; the ctor) and "CRezDir13cb80" (the dtor placeholder) - both stamp
// the SAME own vtable 0x1ef7d0, i.e. one class.)
//
//   CRezFile (this):
//     +0x10  m_name   : char*  - the filename passed to fopen (heap copy).
//     +0x14  m_handle : FILE*  - the open stdio handle (0 = closed).
//     +0x18  m_dir    : CRezDir* - the owning directory / handle cache.
#ifndef SRC_REZ_REZFILE_H
#define SRC_REZ_REZFILE_H

#include <rva.h>

// CRezItmBase (the 8-slot stream-node interface) + CRezDir (the owning handle
// cache: gate @m_parent, open/closed lists, count/cap/mode flags).
#include <Rez/RezMgr.h>

// The shared intrusive list ops: CObjList::Remove (0x1852e0) / CRezList::AddHead
// (0x1851e0) unlink/enroll a node by its +4/+8 links. The dir's embedded lists
// are CRezDirList (RezMgr.h); the ops are reached through the CRezList view (the
// list-family name unification is a flagged follow-up - the helpers only touch
// the {head,tail}/{next,prev} words both shapes share).
#include <Rez/RezList.h>

// The buffered-FILE stdio + heap helpers (statically-linked CRT in retail; external
// no-body so each `call rel32` is reloc-masked). __cdecl, args on the stack.
extern "C" void* Eng_fopen(const char* path, const char* mode); // 0x11f870
extern "C" i32 RezFClose(void* fp);                             // 0x11f780
extern "C" u32 RezFRead(void* buf, u32 size, u32 n, void* fp);  // 0x18c220
extern "C" i32 RezFSeek(void* fp, i32 off, i32 origin);         // 0x18c3a0
extern "C" u32 RezFWrite(void* buf, u32 size, u32 n, void* fp); // 0x18cb40
extern "C" i32 Eng_fflush(void* fp);                            // 0x125b50
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// The three fopen mode strings the lazy-open selects by the dir's flags:
//   s_rb     "rb"  (read-only:  m_write==0 && m_readonly!=0)  - shared w/ SoundDevice
//   s_rPlusB "r+b" (read/write: m_write==0 && m_readonly==0)
//   s_wPlusB "w+b" (write:      m_write!=0 && m_readonly==0)
// Bound via @data-symbol (not DATA): clang mangles the const-char[] extern with a
// `Q` storage class while cl 5.0 emits `P` (?s_X@@3PBDB), so a DATA() label's clang
// mangledName misses the base obj's undefined external ([[data-binding-mangling-gotchas]]).
// @data-symbol is scanned per-.cpp, so the P-mangled bindings live in RezFile.cpp
// (s_rb is also bound by DirectSoundMgr.cpp, which shares the datum).
extern const char s_rb[];
extern const char s_rPlusB[];
extern const char s_wPlusB[];

// ---------------------------------------------------------------------------
// CRezFile - one managed archive FILE* (a CRezDir child node).
// ---------------------------------------------------------------------------
VTBL(CRezFile, 0x001ef7d0);
class CRezFile : public CRezItmBase {
public:
    // 0x13cac0: base-ctor CRezItmBase(parent), record the owning dir, heap-copy
    // the filename, enroll into the dir's closed list (CRezList::AddHead).
    CRezFile(void* parent, char* nameSrc, CRezDir* dir);
    // 0x13cb80: close the handle if open (CloseFile), free the name copy, unlink
    // from the dir's closed list (CObjList::Remove), then ~CRezItmBase folds.
    virtual ~CRezFile() OVERRIDE; // [1]; ??_G 0x13cb60

    virtual void Noop() OVERRIDE; // [0] own empty copy at 0x13cef0

    // Read `count` bytes at `pos` into buf, ensuring the handle is open and
    // recovering through the dir's gate on seek/short-read failure (0x13cc00).
    virtual i32 Read(i32 a, i32 pos, u32 count, void* buf) OVERRIDE; // [2]

    // Write `count` bytes from buf at `pos`, same gating (0x13cca0); the write
    // counterpart of Read (RezFWrite instead of RezFRead).
    virtual i32 Write(i32 a, i32 pos, u32 count, void* buf) OVERRIDE; // [3]

    virtual i32 Open(char* name, i32 readonly, i32 write) OVERRIDE; // [4] 0x13cd40 -> 0
    virtual i32 Close() OVERRIDE;                                   // [5] 0x13cd50 -> 0

    // Flush the handle (0x13cd60, slot 6): fflush retrying through the dir's gate.
    // Returns 1 (no handle / flushed) or 0 (the gate gave up).
    virtual i32 Flush() OVERRIDE; // [6]

    virtual i32 Check() OVERRIDE; // [7] 0x13cdb0 -> 0

    // Lazily (re)open the handle (0x13cdc0, non-virtual): evict the LRU if over
    // the cap, fopen with the flag-selected mode, retrying through the gate; on
    // success move from the closed list to the open list and bump the open count.
    // Returns 1 (already open / opened) or 0 (gave up / invalid mode combo).
    i32 OpenFile();

    // Close the handle (0x13ce70, non-virtual): fclose retrying through the gate,
    // then drop the open count, move back to the closed list, and null the handle.
    // Returns 1 (no handle / closed) or 0 (the gate gave up).
    i32 CloseFile();

    char* m_name;   // +0x10  filename passed to fopen (heap copy; freed by the dtor)
    FILE* m_handle; // +0x14  CRT FILE* (0 = closed); passed to fseek/fread/... by value
    CRezDir* m_dir; // +0x18  owning directory / handle cache
};
SIZE(CRezFile, 0x1c); // verified: CSymParser::ParseRecords `push 0x1c; new; ctor 0x13cac0`

// The "*.*" wildcard literal owned by RezFile.cpp (DATA()-bound there; extern "C" avoids
// the P/Q const-array mangling split). Declared here (C linkage) so the definition can
// drop `extern "C"` while keeping the exact symbol + binding. Emits no code.
extern "C" const char g_wildcard[]; // 0x61a0a0  "*.*"

#endif // SRC_REZ_REZFILE_H
