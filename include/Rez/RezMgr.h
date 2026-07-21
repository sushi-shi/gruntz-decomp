#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Mfc.h>

#include <Bute/Hash.h>

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

struct RezFindRec {
    char raw[0x24];
};
extern "C" i32 RezStatEntry(const char* name, RezFindRec* rec);

class CRezDir;

extern "C" void RezAssertFail(const char* msg);

extern "C" char* RezStrrchr(const char* s, i32 ch);
extern "C" i32 RezStricmp(const char* a, const char* b);

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
    i32 m_readonly;        // +0x18  readonly flag (Open stores its readonly arg here)
    i32 m_1c;        // +0x1c  (set by the virtual load, not this TU; role unproven)
    i32 m_pos;       // +0x20  position cursor (= -1)
};

extern "C" i32 RezFClose(void* fp);                             // 0x11f780
extern "C" u32 RezFRead(void* buf, u32 size, u32 n, void* fp);  // 0x18c220
extern "C" i32 RezFSeek(void* fp, i32 off, i32 origin);         // 0x18c3a0
extern "C" u32 RezFWrite(void* buf, u32 size, u32 n, void* fp); // 0x18cb40

#include <Rez/RezList.h>

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

class CRezDirNode; // fwd (a CHashElement holds a CRezDirNode* sub-dir at +0x14)

struct RezSrc {
    char m_pad0[0x08];
    i32 m_8; // +0x08  (must be nonzero)
    char m_padc[0x1c - 0x0c];
    i32 m_1c;            // +0x1c  (must be <= 1)
    CRezItmBase* m_stream; // +0x20  the polymorphic read stream (family item)
};

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

#include <Gruntz/String.h>

extern "C" void RezFormat(CString* dst, const char* fmt, ...);

extern "C" i32 RezFileExists(const char* szPath);

#endif // SRC_REZ_REZMGR_H
