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
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

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
#include <Rez/RezItmOwner.h>

class CRezItmBase {
public:
    CRezItmBase(void* parent);

    // The 8-slot stream-node interface (ground truth = the retail ??_7CRezItmBase
    // @0x1ef768: [0] 0x13c530 concrete empty, [1] the ??_G/??1 dtor pair
    // 0x13c500/0x13c520, [2..7] __purecall). CRezItm / CRezDir / CRezFile each
    // override all six stream slots; every retail call into the family dispatches
    // (no direct .text callers of any slot body - xref-verified).
    virtual void Slot00_13c530(); // [0] 0x13c530 (empty body; original role unrecovered)
    virtual ~CRezItmBase(); // [1] ??1 0x13c520 (clears m_parent)
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
    CRezItmOwner* m_parent; // +0x0c  owning object CRezItm polls via Retry()
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
    void* m_readBuf; // +0x14  raw heap read buffer (= 0); operator new'd / operator delete'd
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

// A polymorphic stream object: the recursive read does
//   ecx = src->m_stream (the object @ src+0x20)
//   edx = *(void**)ecx   (its vtable)
//   call [edx+8]         (the 3rd virtual slot) with this=ecx, args (off,0,size,buf)
// So model it as a class with the read method at vtable slot index 2.
class RezStream {
public:
    virtual void v0() = 0;
    virtual void v1() = 0;
    virtual i32 ReadAt(i32 off, i32 zero, u32 size, void* buf) = 0; // slot +0x08
};

// The archive source object that the dir node points to at +0x18. Load checks
// m_8 (nonzero) and m_1c (<=1) and reads through the stream at +0x20.
struct RezSrc {
    char m_pad0[0x08];
    i32 m_8; // +0x08  (must be nonzero)
    char m_padc[0x1c - 0x0c];
    i32 m_1c;            // +0x1c  (must be <= 1)
    RezStream* m_stream; // +0x20  (the polymorphic read stream)
};

// The child collection embedded at CRezDirNode+0x38 is the canonical CHashBase
// (First/Next iterated) - defined in <Bute/Hash.h>. Its 8-byte engine size
// {count, buckets} is fixed by CSymTab's dual embedding (Bute/SymTab.h).
class CRezDirNode {
public:
    i32 Load(i32 childFlag);

    void* m_0;      // +0x00 (vtable / base, not touched by Load)
    void* m_4;      // +0x04
    void* m_8;      // +0x08
    i32 m_off;      // +0x0c  (payload offset)
    u32 m_size;     // +0x10  (payload size)
    void* m_subdir; // +0x14  (unused by Load on `this`)
    RezSrc* m_src;  // +0x18  (archive source object)
    char m_pad1c[0x38 - 0x1c];
    CHashBase m_kids; // +0x38..+0x3f  (8-byte engine child collection)
    char m_pad40[8];  // +0x40..+0x47
    void* m_buf;      // +0x48  (payload buffer / loaded gate)
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

// A runtime "low-detail / front-end-class" selector global:
// when nonzero the FEC archive lookup uses the GruntzLo.FEC variant path.
extern i32 g_rezLowDetail;

// ---------------------------------------------------------------------------
// CGameMode - the active game-state/mode object the manager drives once per
// frame (RezMgr+0x2c). It is polymorphic; the per-frame tick calls TWO of its
// virtuals: slot +0x10 (index 4) = Update() -> a frame-delta/status int, and
// slot +0x14 (index 5) = Render()/PostUpdate() (void). The placeholder slots
// 0..3 keep the slot indices correct; modeled external/no-body so the
// `mov eax,[ecx]; call [eax+0x10]` / `call [edx+0x14]` indirect-virtual call
// shapes fall out, reloc-masked.
// ---------------------------------------------------------------------------
class CGameMode {
public:
    virtual void v0();     // +0x00
    virtual void v1();     // +0x04
    virtual void v2();     // +0x08
    virtual void v3();     // +0x0c
    virtual i32 Update();  // +0x10  (slot 4) - per-frame state step
    virtual void Render(); // +0x14  (slot 5) - per-frame post-step
};

// ---------------------------------------------------------------------------
// The per-frame global frame-clock state the tick + the clock-update helper
// (RezMgr::UpdateClock) maintain. File-scope ints (reloc-masked):
//   g_now        : last timeGetTime() sample
//   g_frameDelta : ms elapsed since the previous frame
// and the per-second frame-timing accumulators the tick advances:
//   g_lastNow    g_lastDelta (clamped <= 0x64)
//   g_accumMs    : running accumulated frame-time
//   g_frameTicks : per-frame tick counter
//   five interval countdown timers (seeds
//                           0x32/0x64/0xc8/0x190/0x1f4 ms), each decremented
//                           by the clamped delta and reseeded when it expires.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// RezMgr - the engine game manager (CGameApp::m_8, the WAP32 CGameMgr; engine
// label CGruntzMgr - names are placeholders, only offsets + code bytes are
// load-bearing). Allocated 0xa30 bytes and constructed via the ctor;
// the per-frame idle (CGameApp slot +0x20) tail-calls this object's vtable slot
// +0x10 = PerFrameTick() every frame (BYTE-EXACT, the heart of the game
// loop). Its vftable has slot 0 dtor / +0x10 PerFrameTick /
// +0x14 .. / +0x38 UpdateClock. The ctor (new 0xa30 in
// InitializeGameManager) installs that vftable + the +0xec/+0xf0 path
// CStrings + the +0x150 sub-object - see docs/match-learnings.md for its full map
// (deferred; store-order entropy). Pinned members:
//   +0x2c  m_mode      : the active CGameMode* the tick drives (null => no-op)
//   +0xb0  m_renderGate: when nonzero the tick SKIPS the post-step Render()
//   +0xec  m_pathA     : a CString scratch buffer (assembled archive path #1)
//   +0xf0  m_pathB     : a CString scratch buffer (assembled archive path #2)
//   +0xf4  m_inGameDir : 1 if the CD drive letter == the current dir's drive
//   +0xf8  m_haveRez   : the Gruntz.REZ / DATA-path archive was found
//   +0xfc  m_haveMoviez: the MOVIEZ-path archive was found
// It is polymorphic (vptr @+0). MakeImageKey dispatches a resource load by file
// extension to the BMP/PCX/PID loaders; MakeRezPath assembles candidate archive
// paths and probes them with FileExists.
// ---------------------------------------------------------------------------
// The owning window holder reached at RezMgr+0x04; its +0x04 is the HWND that
// HandleDebugPosition posts the WM_COMMAND to.
struct RezMgrOwner {
    char p0[4];
    HWND m_hWnd;
}; // +0x04 -> +0x04 = HWND

class RezMgr {
public:
    // The per-frame game tick (vtable slot +0x10 / index 4).
    virtual i32 PerFrameTick();

    i32 MakeRezPath();

    // The frame-clock advance helper (a non-virtual member; also
    // installed at vtable slot +0x38). Reconstructed in RezMgr.cpp (0x13ddc0);
    // returns int (the retail symbol is ?UpdateClock@RezMgr@@QAEHXZ).
    i32 UpdateClock();
    // The busy-wait pacing limiter (0x13dec0), reconstructed in src/Wap32/GameApp.cpp.
    void SpinWaitUntil(i32 target); // 0x13dec0
    // (`InitTimeFields` used to be declared here too. It never existed as a RezMgr method:
    // 0x13de70 is WAP32::CGameMgr::InitTimeFields, and this duplicate declaration emitted a
    // ?InitTimeFields@RezMgr@@QAEXH@Z reference that no definition could ever satisfy.
    // UpdateClock now calls the real base method. See the @identity-TODO in GameApp.cpp:
    // RezMgr IS CGruntzMgr : WAP32::CGameMgr, a fold owned by the GruntzMgr lane.)

    // Frame-rate config: SetFrameRate stores `fps` in the pacing gate (+0x1c) and
    // derives the ms-per-frame budget (+0x28 = 1000/fps); TrySetFrameRate installs it
    // only when pacing is not already active (else clears it and fails).
    void SetFrameRate(i32 fps);   // 0x13dee0
    i32 TrySetFrameRate(i32 fps); // 0x13df00

    // CD/install helpers on the manager (external, reloc-masked).
    char GetGruntzDriveLetter();
    void ReportError(i32 msgId, i32 code);

    // The debug-position keyboard probe: when the active mode reports state 3,
    // looks up the "DEBUG_POSITION" debug value and (if set) posts a WM_COMMAND
    // to the owning window. CheckDbgVal is the external (reloc-masked) debug-value
    // lookup helper its call site dispatches to.
    i32 HandleDebugPosition();
    i32 CheckDbgVal(const char* key, i32 defVal, i32 flag);

    // --- layout (vptr occupies +0x00) ---------------------------------------
    RezMgrOwner* m_4;         // +0x04  owning window holder (m_4->m_hWnd = HWND)
    char m_pad8[0x18 - 0x08]; // +0x08..+0x17
    i32 m_smoothedFrameCount; // +0x18  smoothed frame count (UpdateClock: m_frameCounter>>1 window)
    i32 m_pacingGate;         // +0x1c  active gate (>0 enables per-frame pacing)
    i32 m_frameCounter;       // +0x20  frame counter (incremented each tick)
    i32 m_windowStartTick;    // +0x24  window-start tick
    i32 m_frameBudgetMs;      // +0x28  target ms-per-frame (pacing budget)
    CGameMode* m_mode;        // +0x2c  (active game-mode driven per frame)
    char m_pad30[0xb0 - 0x30]; // +0x30..+0xaf
    i32 m_renderGate;          // +0xb0  (nonzero => skip the post-step)
    char m_padb4[0xec - 0xb4]; // +0xb4..+0xeb
    CString m_pathA;           // +0xec  (CString)
    CString m_pathB;           // +0xf0  (CString)
    i32 m_inGameDir;           // +0xf4
    i32 m_haveRez;             // +0xf8
    i32 m_haveMoviez;          // +0xfc
};

// --- vtable catalog ---

#endif // SRC_REZ_REZMGR_H
