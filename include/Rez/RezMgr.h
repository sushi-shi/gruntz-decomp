// RezMgr.h - the Monolith "RezMgr" archive container classes: CRezItmBase (the
// shared resource-tree node base), CRezItm (a leaf resource / file node) and
// CRezDir (a subdirectory node). These build the in-memory directory tree that
// the engine's RezSync/CRezDir loader walks over a Gruntz.REZ / GRUNTZ.VRZ
// archive. The container is the Monolith "RezMgr Version 1" format.
//
// Field names are placeholders (m_<hexoffset>); ONLY the OFFSETS + code bytes
// are load-bearing (campaign doctrine). The layouts below are CONFIRMED from the
// two ctors, the shared base ctor, the
// `operator new` sizes (0x24 leaf / 0x38 dir), and the field stores in
// FindEntry/Load/OpenSub.
//
// ---------------------------------------------------------------------------
// CRezItmBase (16 bytes) - the shared base of every directory-tree node. Both
// ctors call the same base ctor (stores base vtbl @+0 and
// the parent pointer @+0xc).
//   +0x00  vptr     : vtable pointer (base; the derived ctor
//                     overwrites it - two-phase construction).
//   +0x04  m_next   : intrusive sibling link (written by CRezList::AddHead)
//   +0x08  m_prev   : intrusive sibling link (written by CRezList::AddHead)
//   +0x0c  m_parent : owning/parent pointer (the one base-ctor arg).
//
// ---------------------------------------------------------------------------
// CRezItm : CRezItmBase (0x24 = 36 bytes) - a leaf resource (file) node.
// (derived vtbl)
//   +0x10  m_fp  : 0      +0x14  m_readBuf : 0      +0x20  m_pos : -1
//   (+0x18/+0x1c set by the virtual load, not the ctor)
//
// ---------------------------------------------------------------------------
// CRezDir : CRezItmBase (exactly 0x38 = 56 bytes) - a subdirectory node
// (derived vtbl). The ctor inits an embedded child collection sub-object (two
// vtables at +0x10 and +0x1c) and bookkeeping. (The +0x38..+0x64 fields an
// earlier model listed here belong to CRezDirNode, a distinct class the loader
// walks - see the RezMgr.cpp note - not to this 0x38 ctor's object.)
//   +0x10  listA   : (embedded child collection #1: vptr,head,tail)
//   +0x14  ..head  : 0        (collection head)
//   +0x18  ..tail  : 0        (collection tail)
//   +0x1c  listB   : (embedded child collection #2: vptr,head,tail)
//   +0x28  m_28    : 0
//   +0x2c  m_maxOpen : ctor arg2 (the open-handle cap; the dir IS the handle cache)
//   +0x30  m_30    : 1        ("valid"/initialized flag)
//   +0x34  m_34    : 0
#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// <Mfc.h> brings <windows.h> KERNEL32 (GetCurrentDirectoryA, used by MakeRezPath)
// plus CString / CObject; the engine helpers below stay minimal externs.
#include <Mfc.h>

// The canonical hash-bucket collection CHashBase + its intrusive node CHashElement
// (wave5-F1: the former RezColl/RezNode views folded onto these). CRezDirNode embeds
// a CHashBase (m_kids) and walks it with First/Next.
#include <Bute/Hash.h>

// ---------------------------------------------------------------------------
// External engine helpers, modeled with NO body so their `call rel32`
// displacements are reloc-masked in objdiff (the "external no-body callee"
// idiom). Calling-convention/arg-shape pinned from the disasm.
// ---------------------------------------------------------------------------

// Raw heap alloc/free the container links in (alloc(size) returns a
// pointer; free(ptr)). __cdecl, args on the stack.
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// The directory-entry "stat" reader: fills a 0x24-byte WIN32-find-style record
// for `name`, returns 0 on success (FindFirstFileA + GetDriveTypeA + file-time
// conversions live here). The attribute dword lives at byte +6 of the
// record; bit 0x4000 marks a directory entry.
struct RezFindRec {
    char raw[0x24];
};
extern "C" i32 RezStatEntry(const char* name, RezFindRec* rec);

class CRezDir;

// The list/collection iteration helpers the directory tree uses (engine fns,
// external no-body, __thiscall - the collection/node arrives in ecx, no stack
// args). Modeled as member functions (First on the collection, Next on a node)
// so the `lea ecx,[..]; call` / `mov ecx,..; call` shapes fall out, reloc-masked.
//   CHashBase::First()   -> first child node   (shared def in <Bute/Hash.h>)
//   CHashElement::Next() -> next sibling node

// The engine assert/trace sink: prints/logs the message string.
extern "C" void RezAssertFail(const char* msg);

// CRT-ish string helpers used by the RezMgr path/key builders (external,
// no-body so their call displacements are reloc-masked).
//   strrchr(s, ch)   - find LAST occurrence of ch (used to find the
//                      file extension dot).
//   stricmp(a, b)    - case-insensitive compare, 0 on match.
extern "C" char* RezStrrchr(const char* s, i32 ch);
extern "C" i32 RezStricmp(const char* a, const char* b);

// ---------------------------------------------------------------------------
// CRezItmBase - the shared node base (parent ptr @+0xc).
// Polymorphic so the vptr lands at +0x00 and the two-phase vtable stores fall
// out; the ctor takes the parent pointer (stored @+0xc).
// ---------------------------------------------------------------------------
// The owning node reached at CRezItmBase+0xc: CRezItm's stream methods poll its
// slot-2 Retry gate on a short read / seek failure.
class CSymParser; // the real owner/Retry-gate (ex the CRezItmOwner interface view)

class CRezItmBase {
public:
    CRezItmBase(void* parent);

    // The 8-slot stream-node interface (ground truth = the retail ??_7CRezItmBase
    // @0x1ef768: [0] 0x13c530 concrete empty, [1] the ??_G/??1 dtor pair
    // 0x13c500/0x13c520, [2..7] __purecall). CRezItm / CRezDir / CRezFile each
    // override all six stream slots; every retail call into the family dispatches
    // (no direct .text callers of any slot body - xref-verified).
    virtual void Noop(); // [0] 0x13c530 (empty body; original role unrecovered)
    virtual ~CRezItmBase();       // [1] ??1 0x13c520 (clears m_parent)
    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) = 0;  // [2]
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) = 0; // [3]
    virtual i32 Open(char* name, i32 readonly, i32 write) = 0;      // [4]
    virtual i32 Close() = 0;                                        // [5]
    virtual i32 Flush() = 0;                                        // [6]
    virtual i32 Check() = 0;                                        // [7]

    // +0x04/+0x08 are the node's intrusive sibling links, written by
    // CRezList::AddHead (0x1851e0) when the node is enrolled in an owner's child
    // list (see CRezFile in <Rez/RezFile.h>). The ctors here never touch them;
    // typed as the node base rather than left as raw void*.
    CRezItmBase* m_next;    // +0x04
    CRezItmBase* m_prev;    // +0x08
    CSymParser* m_parent; // +0x0c  the owning parser CRezItm polls via Retry() (slot 2)
};

// ---------------------------------------------------------------------------
// CRezItm (0x24 = 36 bytes) - a leaf resource/file node. By its dtor/Read/Close
// bytes it is a buffered FILE* reader: +0x10 the stdio FILE*, +0x14 a heap read
// buffer, +0x20 the current file position cursor (-1 = unset). The owner at
// +0xc is polled to recover from short reads / seek failures.
// ---------------------------------------------------------------------------
class CRezItm : public CRezItmBase {
public:
    CRezItm(void* parent);
    virtual ~CRezItm() OVERRIDE; // [1] 0x13c590

    // Read `count` bytes at file position (off+base) into buf, recovering through
    // the owner's Retry() gate on seek/short-read failure. Returns bytes read
    // (== count) or 0; updates the +0x20 position cursor. (0x13c600, slot 2)
    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) OVERRIDE;

    // Write `count` bytes from buf at file position (base+off), recovering through
    // the owner's Retry() gate on seek/short-write failure (0x13c6c0, slot 3). The
    // position cursor is invalidated (-1); the write counterpart of Read.
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) OVERRIDE;

    // (Re)open the FILE* with an fopen mode picked from the readonly/write flags
    // (write+readonly is rejected), recovering through the owner's Retry() gate;
    // stashes the readonly flag in m_18 and a RezAlloc'd copy of the filename in
    // m_readBuf, then resets the cursor. (0x13c760, slot 4)
    virtual i32 Open(char* filename, i32 readonly, i32 write) OVERRIDE;

    // Close the FILE*, free the read buffer, reset the position cursor. Retries
    // fclose through the owner's gate. Returns 1 on success, 0 if no FILE*/gave up.
    // (0x13c830, slot 5)
    virtual i32 Close() OVERRIDE;

    // fflush the FILE*, retrying through the owner's Retry() gate (0x13c8a0,
    // slot 6). Resets the position cursor first; returns 1 once flushed, 0 if no
    // FILE* or the gate gave up. (Was named Scan; the slot's semantic is flush.)
    virtual i32 Flush() OVERRIDE;

    // Re-acquire check (0x13c8f0, slot 7): reset the cursor, look the FILE* up in
    // the open-file registry (RezDirLookup); if absent, re-Open from the stored
    // filename (m_readBuf) + readonly flag (m_18) - dispatched through slot 4
    // exactly as retail. Returns 1 if open/reopened, else 0.
    virtual i32 Check() OVERRIDE;

    // <Mfc.h> (included above) already pulls <stdio.h>, so FILE is in scope here; the
    // handle is the real CRT FILE* (a pointer-type change is matching-neutral - void*
    // and FILE* are both 4 bytes and m_fp is only touched in RezFile.cpp):
    FILE* m_fp;      // +0x10  CRT FILE* (= 0); passed to fseek/fread/... by value
    char* m_readBuf; // +0x14  owned filename copy (= 0); operator new(strlen+1)'d, strcpy'd, re-Open'd
    i32 m_18;        // +0x18  readonly flag (Open stores its readonly arg here)
    i32 m_1c;        // +0x1c  (set by the virtual load, not this TU; role unproven)
    i32 m_pos;       // +0x20  position cursor (= -1)
};

// The buffered-FILE stdio helpers CRezItm's stream methods call (statically linked
// CRT in retail; external no-body so their `call rel32` displacements are
// reloc-masked). __cdecl, args on the stack.
extern "C" i32 RezFClose(void* fp);                             // 0x11f780
extern "C" u32 RezFRead(void* buf, u32 size, u32 n, void* fp);  // 0x18c220
extern "C" i32 RezFSeek(void* fp, i32 off, i32 origin);         // 0x18c3a0
extern "C" u32 RezFWrite(void* buf, u32 size, u32 n, void* fp); // 0x18cb40

// ---------------------------------------------------------------------------
// CRezDir (ctor builds 0x38 = 56 bytes; runtime fields extend to +0x68) - a
// subdirectory node + the directory the loader walks (Load/OpenSub/FindEntry).
// Own (derived) vtable @0x1ef7a8: cl auto-emits ??_7CRezDir (real polymorphic
// derived of CRezItmBase, whose ctor 0x13c940 stamps it). Retrofit the retail
// datum name (was the anonymous Vtbl_1ef7a8 in UnknownVTables.h); reloc-masked,
// matching-neutral.
// ---------------------------------------------------------------------------
// The embedded child collection sub-object CRezDir carries twice (an intrusive
// {vptr,head,tail} list at +0x10 open, +0x1c closed) is the ONE concrete CRezList
// (<Rez/RezList.h>, own vtable ??_7CRezList @0x1ef7c8 - the dir ctor stamps it
// into both members; the dtor chain dead-store-eliminates down to the single
// ??_7CObjListBase base stamp retail shows). The former "CRezDirList" name was
// this same class.
#include <Rez/RezList.h>

// ---------------------------------------------------------------------------
// CRezDir : CRezItmBase (exactly 0x38 bytes; ctor 0x13c940) - a directory node
// that IS ALSO the open-file-handle cache managing its CRezFile children (the
// former separate "CRezFileMgr" model was THIS class under a second name - the
// layouts are identical field-for-field: gate @+0x0c == CRezItmBase::m_parent,
// open/closed lists @+0x10/+0x1c, count/cap/mode flags @+0x28..+0x34).
// Its slot 4 Open (0x13ca60) latches the readonly/write mode flags; slot 5 Close
// (0x13ca80) drains the open list by CloseFile()-ing each child; the dtor
// (0x13c9b0) deletes every child in both lists.
// ---------------------------------------------------------------------------
VTBL(CRezDir, 0x001ef7a8);
class CRezDir : public CRezItmBase {
public:
    CRezDir(void* parent, i32 maxOpen);
    virtual ~CRezDir() OVERRIDE; // [1] 0x13c9b0 (delete all children; RezFile.cpp)

    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) OVERRIDE;  // [2] 0x13ca40 -> 0
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) OVERRIDE; // [3] 0x13ca50 -> 0
    virtual i32 Open(char* name, i32 readonly, i32 write) OVERRIDE;      // [4] 0x13ca60 latch flags
    virtual i32 Close() OVERRIDE; // [5] 0x13ca80 close all open children
    virtual i32 Flush() OVERRIDE; // [6] 0x13caa0 -> 1
    virtual i32 Check() OVERRIDE; // [7] 0x13cab0 -> 1

    i32 FindEntry(char* name);
    // OpenSub is NOT matched in this TU - see RezMgr.cpp note.

    // Exactly 0x38 bytes (verified: CSymParser::ParseBuffer does `push 0x38; new;
    // call 0x13c940`). The +0x38..+0x64 "runtime" fields that an earlier model kept
    // here actually belong to CRezDirNode (a distinct class walked by Load/OpenSub -
    // see the RezMgr.cpp note); this TU only touches the ctor-set members below.
    // --- ctor-initialized embedded child collections + handle-cache state ---
    CRezList m_openList;   // +0x10  open CRezFile children (LRU; tail evicted)
    CRezList m_closedList; // +0x1c  idle/closed children (ctor enrolls here)
    i32 m_openCount;       // +0x28  currently-open handles (= 0)
    i32 m_maxOpen;         // +0x2c  open-handle cap (= ctor arg2)
    i32 m_readonly;        // +0x30  mode flag (= 1; slot-4 Open re-latches)
    i32 m_write;           // +0x34  mode flag (= 0)
};

// (The former "CRezParseNode" class here was CRezFile under a fake identity: its
// ctor 0x13cac0 / dtor 0x13cb80 stamp CRezFile's own vtable 0x1ef7d0. Folded onto
// the canonical CRezFile in <Rez/RezFile.h>.)

// ---------------------------------------------------------------------------
// CRezDirNode - the directory-tree node that Load is a method of.
// This is the recursively-walked dir node; its field set DIFFERS from the 0x38
// CRezDir built by the OpenSub allocator (the +0x10/+0x18/+0x1c slots that the
// 0x38 ctor uses for the embedded collection's vtables are, on THIS node, a
// data-size @+0x10 and an archive-source pointer @+0x18). Load pins:
//   +0x0c  m_off    : payload offset (passed to the source's virtual read)
//   +0x10  m_size   : payload byte size (allocates a buffer of this size)
//   +0x14  m_subdir : child sub-dir CRezDirNode* (the recursion target)
//   +0x18  m_src    : the archive-source object (its vtable is at m_src+0x20;
//                     slot +8 = ReadAt(off, 0, size, buf); +0x08 a sort/state
//                     field; +0x1c a version/flag that must be <= 1)
//   +0x38  m_kids   : the child collection (First/Next iterated here)
//   +0x48  m_buf    : the loaded payload buffer (also the "already loaded" gate)
// ---------------------------------------------------------------------------
class CRezDirNode; // fwd (a CHashElement holds a CRezDirNode* sub-dir at +0x14)

// The polymorphic stream object at src+0x20 IS a CRezItmBase-family item: the
// dispatched slot-2 (call [edx+8], args (off,0,size,buf)) is exactly CRezItmBase's
// [2] Read(off, base, count, buf) - the RezStream dispatch view is dissolved.

// The archive source object that the dir node points to at +0x18. Load checks
// m_8 (nonzero) and m_1c (<=1) and reads through the stream at +0x20.
struct RezSrc {
    char m_pad0[0x08];
    i32 m_8; // +0x08  (must be nonzero)
    char m_padc[0x1c - 0x0c];
    i32 m_1c;            // +0x1c  (must be <= 1)
    CRezItmBase* m_stream; // +0x20  the polymorphic read stream (family item)
};

// The child collection embedded at CRezDirNode+0x38 is the canonical CHashBase
// (First/Next iterated) - defined in <Bute/Hash.h>. Its 8-byte engine size
// {count, buckets} is fixed by CSymTab's dual embedding (Bute/SymTab.h).
class CRezDirNode {
public:
    i32 Load(i32 childFlag);

    char _vft0[4];  // +0x00 engine vptr (reduced view; not dispatched by Load)
    char m_pad04[0x0c - 0x04]; // +0x04..+0x0b (untouched by Load; roles unrecovered)
    i32 m_off;      // +0x0c  (payload offset)
    u32 m_size;     // +0x10  (payload size)
    CRezDirNode* m_subdir; // +0x14  child sub-dir (the recursion target; unused on `this`)
    RezSrc* m_src;  // +0x18  (archive source object)
    char m_pad1c[0x38 - 0x1c];
    CHashBase m_kids; // +0x38..+0x3f  (8-byte engine child collection)
    char m_pad40[8];  // +0x40..+0x47
    u8* m_buf;        // +0x48  payload buffer / loaded gate
};

// The child chain node walked by Load carries the recursion target (the sub-dir
// CRezDirNode*) in the canonical CHashElement's payload slot CHashElement::m_record
// (void*; cast to CRezDirNode* at the Load call site).

// ---------------------------------------------------------------------------
// CString - the minimal MFC CString model (a single char* m_pchData @+0; an
// `operator const char*` returning it). Ctor/copy/dtor/Format are external
// engine NAFXCW helpers (reloc-masked):
//   CString::CString(const char*)
//   CString::CString(const CString&)
//   CString::~CString()
//   Format(CString*, const char* fmt, ...) - the engine's CString::Format
//             wrapper that defers to the v-formatter.
// Modeled so the path builder's CString churn is reloc-masked.
// ---------------------------------------------------------------------------
#include <Gruntz/String.h>

// Format(dst, fmt, ...) - the file-scope cdecl CString-format wrapper
// (takes the destination CString by address as its first stack arg).
extern "C" void RezFormat(CString* dst, const char* fmt, ...);

// FileExists(szPath) - the Win32 OF_EXIST probe (Utils::WinAPI). Used
// to test each candidate archive path. Returns nonzero if the file exists.
extern "C" i32 RezFileExists(const char* szPath);

// (The former `RezMgr` manager class here - with its `CGameMode`/`RezMgrOwner`
// satellite views and the `CheckDbgVal`/`g_rezLowDetail` phantoms - is DISSOLVED
// (2026-07-16). RezMgr WAS CGruntzMgr (<Gruntz/GruntzMgr.h>), byte-verified:
// retail ??_7CGruntzMgr @0x5e9b64 slot 4 = thunk 0x1c7b -> 0x8b740 PerFrameTick,
// and the base ??_7CGameMgr @0x5e9b8c slot 4 = 0x13ddc0, the base
// CGameMgr::PerFrameTick (ex "UpdateClock"; frame-pacing family declared in
// <Wap32/Wap32.h>, bodies in src/Wap32/GameApp.cpp). CGameMode was CState,
// RezMgrOwner the base's m_gameWnd (CGameWnd), CheckDbgVal == RunModalDialog
// (thunk 0x2bb7 -> 0x90260), g_rezLowDetail a divergent duplicate of
// g_disableHqMovie @0x2455d4. Methods 0x8b740/0x8e470/0x91670 live on in
// src/Rez/RezMgr.cpp as CGruntzMgr's own.)

#endif // SRC_REZ_REZMGR_H
