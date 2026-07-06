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
//   +0x2c  m_rezMgr : ctor arg2 (the owning-manager back-pointer)
//   +0x30  m_30    : 1        ("valid"/initialized flag)
//   +0x34  m_34    : 0
#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// <Mfc.h> brings <windows.h> KERNEL32 (GetCurrentDirectoryA, used by MakeRezPath)
// plus CString / CObject; the engine helpers below stay minimal externs.
#include <Mfc.h>

// The shared RezColl (hash-bucket child collection) + RezNode (chain node)
// definition. CRezDirNode embeds a RezColl (m_kids) and walks it with First/Next.
#include <Rez/RezColl.h>

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
//   RezColl::First()  -> first child node   (shared def in <Rez/RezColl.h>)
//   RezNode::Next()   -> next sibling node

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
    virtual ~CRezItmBase();

    // +0x04/+0x08 are the node's intrusive sibling links, written by
    // CRezList::AddHead (0x1851e0) when the node is enrolled in an owner's child
    // list (see CRezParseNode below). The ctors here never touch them; typed as
    // the node base rather than left as raw void*.
    CRezItmBase* m_next;        // +0x04
    CRezItmBase* m_prev;        // +0x08
    CRezItmOwner* m_parent;     // +0x0c  owning object CRezItm polls via Retry()
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill6(); // vtable-slot filler (real slot; declared-only)
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
    virtual ~CRezItm() OVERRIDE;

    // Read `count` bytes at file position (off+base) into buf, recovering through
    // the owner's Retry() gate on seek/short-read failure. Returns bytes read
    // (== count) or 0; updates the +0x20 position cursor. (vtable slot 2)
    i32 Read(i32 off, i32 base, u32 count, void* buf);

    // Write `count` bytes from buf at file position (base+off), recovering through
    // the owner's Retry() gate on seek/short-write failure (0x13c6c0). The position
    // cursor is invalidated (-1); the write counterpart of Read.
    i32 Write(i32 base, i32 off, u32 count, void* buf);

    // Close the FILE*, free the read buffer, reset the position cursor. Retries
    // fclose through the owner's gate. Returns 1 on success, 0 if no FILE*/gave up.
    // (vtable slot 5)
    i32 Close();

    // Opaque handles kept as void* (RezMgr.h is pulled into /O2-sensitive TUs like
    // Image.cpp, so <stdio.h>/FILE cannot be injected without rescheduling them):
    void* m_fp;                 // +0x10  opaque CRT FILE* (= 0); passed to RezF* by value
    void* m_readBuf;            // +0x14  raw heap read buffer (= 0); RezAlloc'd / RezFree'd
    i32 m_18;                   // +0x18  (set by the virtual load, not this TU; role unproven)
    i32 m_1c;                   // +0x1c  (set by the virtual load, not this TU; role unproven)
    i32 m_pos;                  // +0x20  position cursor (= -1)
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5(); // vtable-slot filler (real slot; declared-only)
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
// {vptr,head,tail} list at +0x10 and +0x1c). REAL POLYMORPHIC (ALL-VTABLES): each
// is a polymorphic list whose implicit vptr the CRezDir ctor auto-stamps (both
// resolve to the same child-collection vtable 0x1ef7c8). The default ctor zeroes
// the head/tail links.
VTBL(CRezDirList, 0x001ef7c8);
struct CRezDirList {
    CRezDirList() {
        m_head = 0;
        m_tail = 0;
    }
    virtual ~CRezDirList(); // +0x00  vptr (external no-body dtor)
    CRezItmBase* m_head;    // +0x04  child chain head (CRezItmBase-derived nodes)
    CRezItmBase* m_tail;    // +0x08  child chain tail
};

VTBL(CRezDir, 0x001ef7a8);
class CRezDir : public CRezItmBase {
public:
    CRezDir(void* parent, void* rezMgr);
    virtual ~CRezDir() OVERRIDE {}

    i32 FindEntry(char* name);
    // OpenSub is NOT matched in this TU - see RezMgr.cpp note.

    // Exactly 0x38 bytes (verified: CSymParser::ParseBuffer does `push 0x38; new;
    // call 0x13c940`). The +0x38..+0x64 "runtime" fields that an earlier model kept
    // here actually belong to CRezDirNode (a distinct class walked by Load/OpenSub -
    // see the RezMgr.cpp note); this TU only touches the ctor-set members below.
    // --- ctor-initialized embedded child collection (+0x10..+0x27) ---
    CRezDirList m_listA;        // +0x10  {vptr,head,tail}
    CRezDirList m_listB;        // +0x1c  {vptr,head,tail}
    i32 m_28;                   // +0x28  (= 0; role unproven)
    void* m_rezMgr;             // +0x2c  (= ctor arg2; owning-manager back-ptr, stored only)
    i32 m_30;                   // +0x30  (= 1; "valid"/initialized flag)
    i32 m_34;                   // +0x34  (= 0; role unproven)
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5(); // vtable-slot filler (real slot; declared-only)
};

// ---------------------------------------------------------------------------
// CRezParseNode : CRezItmBase (0x1c bytes; ctor 0x13cac0) - a .rez parse-tree
// node, the third CRezItmBase-derived class alongside CRezItm/CRezDir. Built by
// CSymParser::ParseRecords (`push 0x1c; new; ctor`); base-ctors CRezItmBase(parent),
// heap-copies its name into +0x10, records the owner @+0x18 and links itself into
// the owner's child list (a CRezList embedded at owner+0x1c) via AddHead. Real
// polymorphic (its own derived vtable 0x1ef7d0, whose datum FinalVtables.cpp
// binds as CVtEmit_1ef7d0; the reloc-masked vptr store here matches whichever symbol
// names that address, so this class carries no VTBL of its own).
class CRezParseNode : public CRezItmBase {
public:
    CRezParseNode(void* parent, char* nameSrc, void* owner);
    virtual void v0(); // new virtual - forces the distinct derived vtable (+0x1ef7d0)

    char* m_10; // +0x10  heap-copied name
    i32 m_14;   // +0x14  (= 0; role unproven)
    void* m_18; // +0x18  owner (its child list is the CRezList at owner+0x1c)
};

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
class CRezDirNode; // fwd (RezNode holds a CRezDirNode* sub-dir at +0x14)

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

// The child collection embedded at CRezDirNode+0x38 is the shared RezColl
// (First/Next iterated) - defined in <Rez/RezColl.h>. Its 8-byte engine size
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
    RezColl m_kids;  // +0x38..+0x3f  (8-byte engine child collection)
    char m_pad40[8]; // +0x40..+0x47
    void* m_buf;     // +0x48  (payload buffer / loaded gate)
};

// The child chain node walked by Load carries the recursion target (the sub-dir
// CRezDirNode*) in the shared RezNode's payload slot RezNode::m_14 (void*; cast
// to CRezDirNode* at the Load call site).

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

    i32 MakeImageKey(void* arg1, char* name, void* arg3);
    i32 MakeRezPath();

    // The frame-clock advance helper (a non-virtual member; also
    // installed at vtable slot +0x38). Reconstructed in RezMgr.cpp (0x13ddc0);
    // returns int (the retail symbol is ?UpdateClock@RezMgr@@QAEHXZ).
    i32 UpdateClock();
    // Its two external/no-body __thiscall callees (reloc-masked): the busy-wait
    // pacing limiter (0x13dec0) and the window-start time sampler (0x13de70).
    void SpinWaitUntil(i32 target); // 0x13dec0
    void InitTimeFields(i32 reset); // 0x13de70

    // The extension-dispatch image loaders (external, reloc-masked).
    i32 LoadBmp(void* a, void* b);
    i32 LoadPcx(void* a, void* b);
    i32 LoadPid(void* a, void* b, void* c);

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
