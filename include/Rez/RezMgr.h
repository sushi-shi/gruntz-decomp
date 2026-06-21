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
//   +0x00  m_vtbl   : vtable pointer (base; the derived ctor
//                     overwrites it - two-phase construction).
//   +0x04  m_4      : (not written by these ctors)
//   +0x08  m_8      : (not written by these ctors; OpenSub zeroes its owner's +8)
//   +0x0c  m_parent : owning/parent pointer (the one base-ctor arg).
//
// ---------------------------------------------------------------------------
// CRezItm : CRezItmBase (0x24 = 36 bytes) - a leaf resource (file) node.
// (derived vtbl)
//   +0x10  m_10  : 0      +0x14  m_14 : 0      +0x20  m_20 : -1
//   (+0x18/+0x1c set by the virtual load, not the ctor)
//
// ---------------------------------------------------------------------------
// CRezDir : CRezItmBase (>= 0x38; ctor builds 0x38 = 56 bytes) - a subdirectory
// node (derived vtbl). The ctor inits an embedded child collection
// sub-object (two vtables at +0x10 and +0x1c) and bookkeeping; the
// higher fields (+0x40..+0x64) are written by Load/OpenSub at runtime.
//   +0x10  m_vtblA : (collection vtable #1)
//   +0x14  m_14    : 0        (collection head)
//   +0x18  m_18    : 0        (collection tail)
//   +0x1c  m_vtblB : (collection vtable #2)
//   +0x20  m_20 :0  +0x24 m_24:0  +0x28 m_28:0  +0x34 m_34:0
//   +0x2c  m_2c    : ctor arg2 (the owning RezMgr back-pointer)
//   +0x30  m_30    : 1        ("valid"/initialized flag)
// Runtime fields (NOT ctor-initialized; pinned from Load/OpenSub):
//   +0x38  m_coll2 : a second/child collection base (Load iterates &this+0x38)
//   +0x40  m_loaded: OpenSub's load gate (return 0 if zero)
//   +0x44  m_44    : passed to the recursive walker
//   +0x48  m_loaded2: Load's already-loaded gate (return 1 if nonzero)
//   +0x54  m_w     +0x58 m_h  +0x5c m_x  +0x60 m_y : running max dims (OpenSub)
//   +0x64  m_name  : cached lookup-name buffer (operator new'd / freed)
#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H
#include <rva.h>  // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// <Mfc.h> brings <windows.h> KERNEL32 (GetCurrentDirectoryA, used by MakeRezPath)
// plus CString / CObject; the engine helpers below stay minimal externs.
#include <Mfc.h>

// ---------------------------------------------------------------------------
// External engine helpers, modeled with NO body so their `call rel32`
// displacements are reloc-masked in objdiff (the "external no-body callee"
// idiom). Calling-convention/arg-shape pinned from the disasm.
// ---------------------------------------------------------------------------

// Raw heap alloc/free the container links in (alloc(size) returns a
// pointer; free(ptr)). __cdecl, args on the stack.
extern "C" void *RezAlloc(unsigned int size);
extern "C" void  RezFree(void *p);

// The directory-entry "stat" reader: fills a 0x24-byte WIN32-find-style record
// for `name`, returns 0 on success (FindFirstFileA + GetDriveTypeA + file-time
// conversions live here). The attribute dword lives at byte +6 of the
// record; bit 0x4000 marks a directory entry.
struct RezFindRec { char raw[0x24]; };
extern "C" int RezStatEntry(const char *name, RezFindRec *rec);

class CRezDir;

// The list/collection iteration helpers the directory tree uses (engine fns,
// external no-body, __thiscall - the collection/node arrives in ecx, no stack
// args). Modeled as member functions (First on the collection, Next on a node)
// so the `lea ecx,[..]; call` / `mov ecx,..; call` shapes fall out, reloc-masked.
//   RezColl::First()  -> first child node
//   RezNode::Next()   -> next sibling node
struct RezNode;

// The engine assert/trace sink: prints/logs the message string.
extern "C" void RezAssertFail(const char *msg);

// CRT-ish string helpers used by the RezMgr path/key builders (external,
// no-body so their call displacements are reloc-masked).
//   strrchr(s, ch)   - find LAST occurrence of ch (used to find the
//                      file extension dot).
//   stricmp(a, b)    - case-insensitive compare, 0 on match.
extern "C" char *RezStrrchr(const char *s, int ch);
extern "C" int   RezStricmp(const char *a, const char *b);


// ---------------------------------------------------------------------------
// CRezItmBase - the shared node base (parent ptr @+0xc).
// Polymorphic so the vptr lands at +0x00 and the two-phase vtable stores fall
// out; the ctor takes the parent pointer (stored @+0xc).
// ---------------------------------------------------------------------------
class CRezItmBase {
public:
    CRezItmBase(void *parent);
    virtual ~CRezItmBase() {}

    void  *m_4;        // +0x04
    void  *m_8;        // +0x08
    void  *m_parent;   // +0x0c
};

// ---------------------------------------------------------------------------
// CRezItm (0x24 = 36 bytes) - a leaf resource/file node.
// ---------------------------------------------------------------------------
class CRezItm : public CRezItmBase {
public:
    CRezItm(void *parent);
    virtual ~CRezItm() OVERRIDE {}

    int   m_10;   // +0x10  (= 0)
    int   m_14;   // +0x14  (= 0)
    int   m_18;   // +0x18  (set by load)
    int   m_1c;   // +0x1c  (set by load)
    int   m_20;   // +0x20  (= -1)
};

// ---------------------------------------------------------------------------
// CRezDir (ctor builds 0x38 = 56 bytes; runtime fields extend to +0x68) - a
// subdirectory node + the directory the loader walks (Load/OpenSub/FindEntry).
// ---------------------------------------------------------------------------
class CRezDir : public CRezItmBase {
public:
    CRezDir(void *parent, void *rezMgr);
    virtual ~CRezDir() OVERRIDE {}

    int   FindEntry(char *name);
    // OpenSub is NOT matched in this TU - see RezMgr.cpp note.

    // --- ctor-initialized embedded child collection (+0x10..+0x34) ---
    void *m_vtblA;   // +0x10  (collection vtable)
    int   m_14;      // +0x14  (= 0, collection head)
    int   m_18;      // +0x18  (= 0, collection tail)
    void *m_vtblB;   // +0x1c  (collection vtable)
    int   m_20;      // +0x20  (= 0)
    int   m_24;      // +0x24  (= 0)
    int   m_28;      // +0x28  (= 0)
    void *m_2c;      // +0x2c  (= ctor arg2)
    int   m_30;      // +0x30  (= 1)
    int   m_34;      // +0x34  (= 0)
    // --- runtime-only fields (NOT set by the ctor) ---
    int   m_38;      // +0x38  (second collection base; Load walks &this+0x38)
    int   m_3c;      // +0x3c
    int   m_loaded;  // +0x40  (OpenSub gate)
    int   m_44;      // +0x44
    int   m_loaded2; // +0x48  (Load gate)
    int   m_4c;      // +0x4c
    int   m_50;      // +0x50
    int   m_54;      // +0x54  (max width)
    int   m_58;      // +0x58  (max height)
    int   m_5c;      // +0x5c  (max x)
    int   m_60;      // +0x60  (max y)
    void *m_name;    // +0x64  (cached lookup-name buffer)

    // Engine-label backlog stubs.
    void Stub_13b0c0();
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
class CRezDirNode;  // fwd (RezNode holds a CRezDirNode* sub-dir at +0x14)

// A polymorphic stream object: the recursive read does
//   ecx = src->m_stream (the object @ src+0x20)
//   edx = *(void**)ecx   (its vtable)
//   call [edx+8]         (the 3rd virtual slot) with this=ecx, args (off,0,size,buf)
// So model it as a class with the read method at vtable slot index 2.
class RezStream {
public:
    virtual void v0() = 0;
    virtual void v1() = 0;
    virtual int ReadAt(int off, int zero, unsigned size, void *buf) = 0;  // slot +0x08
};

// The archive source object that the dir node points to at +0x18. Load checks
// m_8 (nonzero) and m_1c (<=1) and reads through the stream at +0x20.
struct RezSrc {
    char       m_pad0[0x08];
    int        m_8;        // +0x08  (must be nonzero)
    char       m_padc[0x1c - 0x0c];
    int        m_1c;       // +0x1c  (must be <= 1)
    RezStream *m_stream;   // +0x20  (the polymorphic read stream)
};

// The child collection embedded at CRezDirNode+0x38 (First/Next iterated).
// First() is __thiscall: returns the first child node or 0.
struct RezColl {
    RezNode *First();
    char m_pad[0x10];
};

class CRezDirNode {
public:
    int Load(int childFlag);

    void   *m_0;        // +0x00 (vtable / base, not touched by Load)
    void   *m_4;        // +0x04
    void   *m_8;        // +0x08
    int     m_off;      // +0x0c  (payload offset)
    unsigned m_size;    // +0x10  (payload size)
    void   *m_subdir;   // +0x14  (unused by Load on `this`)
    RezSrc *m_src;      // +0x18  (archive source object)
    char    m_pad1c[0x38 - 0x1c];
    RezColl m_kids;     // +0x38..+0x47  (child collection, 0x10 bytes)
    void   *m_buf;      // +0x48  (payload buffer / loaded gate)
};

// A child entry node in a CRezDirNode's collection: holds the sub-dir node ptr
// at +0x14 that Load recurses into. Next() is __thiscall: returns the
// next sibling node or 0.
struct RezNode {
    RezNode *Next();
    char         m_pad0[0x14];
    CRezDirNode *m_14;  // +0x14  (sub-dir node; Load recurses on it)
};

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
#include <Gruntz/CString.h>

// Format(dst, fmt, ...) - the file-scope cdecl CString-format wrapper
// (takes the destination CString by address as its first stack arg).
extern "C" void RezFormat(CString *dst, const char *fmt, ...);

// FileExists(szPath) - the Win32 OF_EXIST probe (Utils::WinAPI). Used
// to test each candidate archive path. Returns nonzero if the file exists.
extern "C" int RezFileExists(const char *szPath);

// A runtime "low-detail / front-end-class" selector global:
// when nonzero the FEC archive lookup uses the GruntzLo.FEC variant path.
extern int g_rezLowDetail;

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
    virtual void v0();                 // +0x00
    virtual void v1();                 // +0x04
    virtual void v2();                 // +0x08
    virtual void v3();                 // +0x0c
    virtual int  Update();             // +0x10  (slot 4) - per-frame state step
    virtual void Render();             // +0x14  (slot 5) - per-frame post-step
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
class RezMgr {
public:
    // The per-frame game tick (vtable slot +0x10 / index 4).
    virtual int  PerFrameTick();

    int  MakeImageKey(void *arg1, char *name, void *arg3);
    int  MakeRezPath();

    // The frame-clock advance helper (a non-virtual member; also
    // installed at vtable slot +0x38). External/no-body so its direct call is
    // reloc-masked.
    void UpdateClock();

    // The extension-dispatch image loaders (external, reloc-masked).
    int  LoadBmp(void *a, void *b);
    int  LoadPcx(void *a, void *b);
    int  LoadPid(void *a, void *b, void *c);

    // CD/install helpers on the manager (external, reloc-masked).
    char GetGruntzDriveLetter();
    void ReportError(int msgId, int code);

    // The debug-position keyboard probe: when the active mode reports state 3,
    // looks up the "DEBUG_POSITION" debug value and (if set) posts a WM_COMMAND
    // to the owning window. CheckDbgVal is the external (reloc-masked) debug-value
    // lookup helper its call site dispatches to.
    int  HandleDebugPosition();
    int  CheckDbgVal(const char *key, int defVal, int flag);

    // --- layout (vptr occupies +0x00) ---------------------------------------
    char       m_pad4[0x2c - 0x04];   // +0x04..+0x2b
    CGameMode *m_mode;                // +0x2c  (active game-mode driven per frame)
    char       m_pad30[0xb0 - 0x30];  // +0x30..+0xaf
    int        m_renderGate;          // +0xb0  (nonzero => skip the post-step)
    char       m_padb4[0xec - 0xb4];  // +0xb4..+0xeb
    CString  m_pathA;        // +0xec  (CString)
    CString  m_pathB;        // +0xf0  (CString)
    int        m_inGameDir;    // +0xf4
    int        m_haveRez;      // +0xf8
    int        m_haveMoviez;   // +0xfc
};

#endif // SRC_REZ_REZMGR_H
