// RezFile.cpp - the Rez archive-container file-system family, carved out of
// src/Rez/RezMgr.cpp (holding-TU drain, 2026-07-11). ONE contiguous retail .text obj
// spanning 0x13c4e0-0x13ceec: the CRezItmBase/CRezItm/CRezDir/CRezParseNode directory-
// tree nodes AND the CRezFile/CRezFileMgr LRU-managed open-file wrapper (ex RezFile
// unit). Proven ONE file by the shared private fopen-mode literals 0x21a0a4 ("r+b") /
// 0x21a0a8 ("w+b"), referenced ONLY by Open@CRezItm + Open@CRezFile, and the text
// A-B-A weave (CloseAllOpen@CRezFileMgr @0x13ca80 inside the CRezItm run).
//
// Both node ctors share the base ctor CRezItmBase::CRezItmBase (stores the base vtable
// and the parent pointer @+0xc), then overwrite the vtable with the derived one
// (two-phase construction; all vtable stores reloc-masked). `operator new` sizes 0x24
// (leaf) / 0x38 (dir) confirm the layouts.
//
// OpenSub is NOT matched here: it runs on a THIRD, distinct node layout (uses +0x1c as
// a child COUNT and +0x10 as a list-append target, conflicting with both the 0x38
// CRezDir ctor's vtable stores and CRezDirNode's +0x10 size / +0x18 source - so the
// three "CRezDir"-labeled functions are actually three different classes).
#include <Rez/RezMgr.h>
#include <Rez/RezFile.h> // CRezFile/CRezFileMgr (this TU's own classes; shared decls)
#include <Rez/RezList.h> // CRezList AddHead (0x1851e0) - owner child-list enroll
#include <rva.h>

// ---------------------------------------------------------------------------
// CRezItmBase::CRezItmBase(parent)
//   mov [this] = base vtbl; mov [this+0xc] = parent. Out-of-line so
//   the derived ctors emit a `call` to it.
RVA(0x0013c4e0, 0x12)
CRezItmBase::CRezItmBase(void* parent) {
    // Language-forced cast: the ctor's parameter is `void*` in the retail ABI
    // (mangled ??0CRezItmBase@@QAE@PAX@Z = PAX), while the stored member is the
    // typed Retry-gate CRezItmOwner*. Storing the void* param into the typed
    // member requires the reinterpret.
    m_parent = (CRezItmOwner*)parent;
}

// ---------------------------------------------------------------------------
// CRezItmBase::~CRezItmBase()
// The base destructor: restore the base vtbl (auto, since polymorphic) and clear
// the parent pointer. Out-of-line so the derived dtor emits a `call` to it.
RVA(0x0013c520, 0xe)
CRezItmBase::~CRezItmBase() {
    m_parent = 0;
}

// ---------------------------------------------------------------------------
// CRezItm::CRezItm(parent)
// Base ctor (vtbl + parent), then derived vtbl, m_fp = 0,
// m_readBuf = 0, m_pos = -1. m_18/m_1c untouched.
RVA(0x0013c540, 0x28)
CRezItm::CRezItm(void* parent) : CRezItmBase(parent) {
    m_fp = 0;
    m_readBuf = 0;
    m_pos = -1;
}

// ---------------------------------------------------------------------------
// CRezItm::~CRezItm()
// Derived dtor: if a FILE* is open, Close() it; free the read buffer; chain to
// the base dtor. A destructible state -> the /GX EH frame (push -1 / handler).
RVA(0x0013c590, 0x66)
CRezItm::~CRezItm() {
    if (m_fp != 0) {
        Close();
    }
    if (m_readBuf != 0) {
        RezFree(m_readBuf);
    }
}

// ---------------------------------------------------------------------------
// CRezItm::Read(off, base, count, buf)  (vtable slot 2)
// Seek to absolute position (off+base) if the cursor isn't already there,
// recovering through the owner's Retry() gate on a seek failure; then fread
// `count` bytes into buf, retrying through the same gate on a short read. On
// success advances the +0x20 cursor and returns the bytes read; 0 / cursor=-1
// on a zero count or a gate that gives up.
//
// 99.77% (reloc-masked plateau): EVERY code byte matches the disasm. The entry
// guard now spells `if(count <= 0)` (unsigned `count`, so `<= 0` == `== 0`) which
// lowers to retail's `test edi,edi; ja` - the equality `== 0` form gave `jne`, and
// wrapping the body in `if(count>0)` cascaded (99.3%->83.4%); the `<= 0`
// early-return flips the one opcode with no cascade (docs/patterns/
// unsigned-zero-guard-le-not-eq.md). Only residual: the four buffered-IO calls,
// which the delinker names bare `fseek`/`fread` (Ghidra FLIRT) vs our
// `_RezFSeek`/`_RezFRead` - identical `call rel32`, a delinker bare-name artifact.
RVA(0x0013c600, 0xbd)
i32 CRezItm::Read(i32 off, i32 base, u32 count, void* buf) {
    if (count <= 0) {
        return 0;
    }

    i32 pos = base + off;

    if (m_pos != pos) {
        while (RezFSeek(m_fp, pos, 0) != 0) {
            if (m_parent->Retry() == 0) {
                m_pos = -1;
                return 0;
            }
        }
    }

    u32 got = RezFRead(buf, 1, count, m_fp);
    while (got != count) {
        if (m_parent->Retry() == 0) {
            m_pos = -1;
            return 0;
        }
        got = RezFRead(buf, 1, count, m_fp);
    }

    m_pos = got + pos;
    return got;
}

// ---------------------------------------------------------------------------
// CRezItm::Write(base, off, count, buf)
// The write counterpart of Read: invalidate the cursor (m_pos = -1), seek to the
// absolute position (base+off) recovering through the owner's Retry() gate on a
// seek failure, then fwrite `count` bytes from buf, retrying the write through
// the same gate on a short write. Returns 0 on a zero count or a gate that gives
// up; the write count otherwise. Unlike Read, the cursor is left invalid.
// ---------------------------------------------------------------------------
// @early-stop
// 99.69% (reloc-masked plateau) - every code byte matches retail. The count guard
// now spells `if(count <= 0)` (unsigned), lowering to retail's `test;jbe` (the
// `== 0` form gave `je`; see Read's note + docs/patterns/
// unsigned-zero-guard-le-not-eq.md). Only residual: the buffered-IO calls the
// delinker names bare `fseek`/`fread` (Ghidra FLIRT) vs our `_RezFSeek`/`_RezFWrite`
// - identical `call rel32`, a delinker bare-name artifact (not steerable from src).
RVA(0x0013c6c0, 0x97)
i32 CRezItm::Write(i32 base, i32 off, u32 count, void* buf) {
    m_pos = -1;
    if (count <= 0) {
        return 0;
    }

    i32 pos = off + base;

    while (RezFSeek(m_fp, pos, 0) != 0) {
        if (m_parent->Retry() == 0) {
            return 0;
        }
    }

    u32 put = RezFWrite(buf, 1, count, m_fp);
    while (put != count) {
        if (m_parent->Retry() == 0) {
            return 0;
        }
        put = RezFWrite(buf, 1, count, m_fp);
    }
    return put;
}

// ---------------------------------------------------------------------------
// CRezItm::Open(filename, readonly, write)
// Pick the fopen mode from the readonly/write flags (write+readonly is invalid),
// (re)open the FILE* recovering through the owner's Retry() gate, then stash the
// readonly flag (m_18), keep a RezAlloc'd copy of the filename (m_readBuf) and
// reset the position cursor. Same mode ladder as CRezFile::Open.
RVA(0x0013c760, 0xc1)
i32 CRezItm::Open(char* filename, i32 readonly, i32 write) {
    for (;;) {
        if (write) {
            if (readonly) {
                return 0;
            }
            m_fp = Eng_fopen(filename, s_wPlusB);
        } else if (readonly) {
            m_fp = Eng_fopen(filename, s_rb);
        } else {
            m_fp = Eng_fopen(filename, s_rPlusB);
        }
        if (m_fp != 0) {
            break;
        }
        if (m_parent->Retry() == 0) {
            return 0;
        }
        if (m_fp != 0) {
            break;
        }
    }

    m_18 = readonly;
    if (m_readBuf != 0) {
        RezFree(m_readBuf);
    }
    m_readBuf = RezAlloc(strlen(filename) + 1);
    if (m_readBuf != 0) {
        strcpy((char*)m_readBuf, filename);
    }
    m_pos = -1;
    return 1;
}

// ---------------------------------------------------------------------------
// CRezItm::Close()  (vtable slot 5)
// fclose the FILE*, retrying through the owner's Retry() gate; then free the
// read buffer and reset the cursor. Returns 1 on success, 0 if there was no open
// FILE* or the gate gave up.
// @early-stop
// regalloc wall (zero-register-pinning): structure is byte-exact but retail pins
// this->esi / the loop-flag ok->edi, while my cl swaps them (this->edi, ok->esi)
// -- the prologue interleave `push esi; mov ecx,esi; push edi` vs mine
// `push esi; push edi; mov ecx,edi`, cascading the esi<->edi names through the
// whole body. `while(ok==0)`+`ok=0` init beats `do-while` (81.3% vs 56.5%); the
// swap is the documented MSVC5 callee-save coin-flip, not source-steerable.
RVA(0x0013c830, 0x63)
i32 CRezItm::Close() {
    if (m_fp == 0) {
        return 0;
    }

    i32 ok = 0;
    while (ok == 0) {
        if (RezFClose(m_fp) == 0) {
            ok = 1;
        } else {
            ok = 0;
            if (m_parent->Retry() == 0) {
                return 0;
            }
        }
    }

    m_fp = 0;
    if (m_readBuf != 0) {
        RezFree(m_readBuf);
    }
    m_readBuf = 0;
    m_pos = -1;
    return ok;
}

// The FILE*-readable probe (0x125b50): returns 0 when a byte is available (the loop
// keeps retrying while it reports "not ready"). Statically-linked CRT helper; external
// no-body so the `call rel32` displacement is reloc-masked. __cdecl, arg on the stack.
extern "C" i32 RezItmProbe(void* fp); // 0x125b50

// ---------------------------------------------------------------------------
// CRezItm::Scan()  (vtable slot 6; re-homed from src/Stub/BoundaryUpper.cpp)
// Reset the position cursor, then (if a FILE* is open) poll the stream until a byte
// is readable, retrying through the owner's Retry() gate. Returns 1 once ready, 0 if
// no FILE* or the gate gave up.
RVA(0x0013c8a0, 0x45)
i32 CRezItm::Scan() {
    m_pos = -1;
    if (m_fp) {
        i32 found;
        do {
            if (RezItmProbe(m_fp) == 0) {
                found = 1;
            } else {
                found = 0;
                if (m_parent->Retry() == 0) {
                    return 0;
                }
            }
        } while (!found);
        return found;
    }
    return 0;
}

// The open-file registry lookup (0x18ccd0): returns the FILE*'s slot or -1 if the
// handle is no longer registered. Statically-linked CRT-ish helper; external no-body
// (reloc-masked). __cdecl, arg on the stack.
extern "C" i32 RezDirLookup(void* fp); // 0x18ccd0

// ---------------------------------------------------------------------------
// CRezItm::Check()  (vtable slot 7; re-homed from src/Stub/BoundaryUpper.cpp)
// Reset the cursor, look the FILE* up in the open-file registry; if still registered
// return 1, else re-Open from the stored filename/flags and normalize to bool.
// @early-stop
// slot-4-devirt wall: the retail slot-7 body dispatches the Open self-call VIRTUALLY
// (`mov eax,[this]; call [eax+0x10]` = vtable slot 4). CRezItm still models its stream
// methods non-virtual, so Open resolves here as a direct `call 0x13c760` - byte-exact
// everywhere except that one call encoding. Closes fully once CRezItm/CRezItmBase are
// converted to real virtuals in retail slot order (Slot00,dtor,Read,Write,Open,Close,
// Scan,Check) - a shared-base vtable conversion larger than this leaf move.
RVA(0x0013c8f0, 0x41)
i32 CRezItm::Check() {
    m_pos = -1;
    if (!m_fp) {
        return 0;
    }
    if (RezDirLookup(m_fp) != -1) {
        return 1;
    }
    return Open((char*)m_readBuf, m_18, 0) != 0;
}

// ---------------------------------------------------------------------------
// CRezDir::CRezDir(parent, rezMgr)
// Base ctor, then the two embedded child-collection list members auto-construct
// (each stamps ??_7CRezDirList @0x1ef7c8 and zeroes head/tail), the derived vtbl
// is stamped, then m_28=0, m_34=0, m_rezMgr=rezMgr, m_30=1.
// @early-stop
// vptr-schedule wall (ALL-VTABLES): real-polymorphic list members auto-stamp their
// vptr FIRST (vptr,head,tail) and the compiler zeroes m_28/m_34 after the derived
// vptr, vs retail's head,tail,vtbl store order + pre-vptr field zeroing. The two
// child collections + the CRezItmBase base are byte-faithful; converted per the
// ALL-VTABLES mandate (was the hand-rolled child-collection double-stamp).
RVA(0x0013c940, 0x46)
CRezDir::CRezDir(void* parent, void* rezMgr) : CRezItmBase(parent) {
    m_28 = 0;
    m_34 = 0;
    m_rezMgr = rezMgr;
    m_30 = 1;
}

// ---------------------------------------------------------------------------
// The two CRezDir-family /GX destructors (re-homed from src/Stub/BoundaryUpper2Eh.cpp),
// co-located next to CRezDir. Their derived vtables (0x1ef760/0x1ef7d0) are CObjListBase-
// family; the shared out-of-line base subobject dtor is 0x13c520 == CRezItmBase's dtor,
// so both derive from the real CRezItmBase (which binds the base-subobject dtor call to
// ??1CRezItmBase). Kept distinct placeholder identities (CRezDir13c9b0/CRezDir13cb80)
// since they are not the same symbol as CRezDir's inline dtor.
// ---------------------------------------------------------------------------
struct RezListNode {
    virtual void v0();
    virtual void Delete(i32); // slot 1 (+0x4)
};
SIZE_UNKNOWN(RezListNode);
struct CRezDir13c9b0 : CRezItmBase {
    void* m_10;        // +0x10
    RezListNode* m_14; // +0x14
    i32 _18;           // +0x18
    void* m_1c;        // +0x1c
    RezListNode* m_20; // +0x20
    virtual ~CRezDir13c9b0() OVERRIDE;
};
SIZE_UNKNOWN(CRezDir13c9b0);
RELOC_VTBL(CRezDir13c9b0, 0x001ef760); // aliases CObjListBase (dtor-stamp verified)
RVA(0x0013c9b0, 0x7f)
CRezDir13c9b0::~CRezDir13c9b0() {
    while (m_14) {
        m_14->Delete(1);
    }
    while (m_20) {
        m_20->Delete(1);
    }
}

// 0x13ca30 - an abstract-base vptr-restore dtor thunk (RVA-adjacent): cl's implicit
// vptr-restore stamps the 0x5ef760 pure-call vtable (the same CObjListBase-aliasing
// vtable CRezDir13c9b0 above uses) into [this] and returns (7-byte
// `mov [ecx],offset ??_7 + ret`). Placeholder polymorphic class; an empty virtual dtor
// emits exactly the stamp+ret. Re-homed from src/Stub/BoundaryThunks.cpp.
struct CAbstract13ca30 {
    virtual ~CAbstract13ca30();
};
SIZE_UNKNOWN(CAbstract13ca30);
RELOC_VTBL(CAbstract13ca30, 0x001ef760); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0013ca30, 0x7)
CAbstract13ca30::~CAbstract13ca30() {}

// CloseAllOpen (0x13ca80): drain the open-handle list. The list head sits at
// CRezFileMgr+0x14 (m_openList.m_head); Close() each file until the list empties
// (each Close moves the node off the open list, advancing the head). Returns 1.
RVA(0x0013ca80, 0x1d)
i32 CRezFileMgr::CloseAllOpen() {
    // The list stores CRezFile nodes; retrieve the head as its concrete type (the
    // typed intrusive-list access - CRezFile's node base is at offset 0, so this is a
    // zero-offset static downcast, matching-neutral). Close() is a direct call.
    while (m_openList.m_head != 0) {
        ((CRezFile*)m_openList.m_head)->Close();
    }
    return 1;
}

// -------------------------------------------------------------------------
// CRezParseNode::CRezParseNode(parent, nameSrc, owner)  (0x13cac0; class in
// RezMgr.h). Base-ctors CRezItmBase(parent), stamps its distinct derived vtable
// (0x1ef7d0), records the owner @+0x18, heap-copies the name into +0x10, and links
// itself into the owner's embedded child list (a CRezList at owner+0x1c) via
// AddHead. Real-polymorphic (CRezItmBase-derived) so the base-ctor call +
// derived-vptr stamp + /GX base-cleanup frame fall out; the derived vtable stays a
// reloc-masked COMDAT here (retail datum 0x1ef7d0 emitted by FinalVtables).
RVA(0x0013cac0, 0x9b)
CRezParseNode::CRezParseNode(void* parent, char* nameSrc, void* owner) : CRezItmBase(parent) {
    m_18 = owner;
    m_14 = 0;
    // operator new returns void*; char* needed for strcpy (language-forced).
    char* buf = (char*)::operator new(strlen(nameSrc) + 1);
    m_10 = buf;
    strcpy(buf, nameSrc);
    // Enroll into the owner's child list. The owner (m_18) is a foreign parser
    // object whose embedded CRezList sits at +0x1c - documented offset access (the
    // codegen reloads it from this+0x18: `mov ecx,[ebx+0x18]; add ecx,0x1c`). The
    // node cast is the generic intrusive-list insertion (CRezList links any node
    // type by its +4/+8 slots, which CRezItmBase carries).
    CRezList* kids = (CRezList*)((char*)m_18 + 0x1c);
    kids->AddHead((CRezListNode*)this);
}

// ===========================================================================
// Class-metadata annotations for the RezMgr.h classes. Hosted at EOF of this TU
// (not in the header): RezMgr.h is pulled into the /O2-sensitive Image.cpp for
// RezAlloc/RezFree, where any header-injected typedef reschedules DecodePcxData
// (verified). Placed after all function bodies so this TU is unperturbed too.
// ===========================================================================
SIZE(RezFindRec, 0x24);     // RE'd WIN32-find-style fixed record
SIZE_UNKNOWN(CRezItmOwner); // abstract Retry-gate interface (no storage/vtable here)

struct RezOwner18 {
    i32 _0[0x1c / 4];
    CObjList m_1c; // +0x1c
};
SIZE_UNKNOWN(RezOwner18);
// Real base is CRezItmBase (0x10-byte node base; its out-of-line subobject dtor is
// 0x13c520). Deriving from it binds the derived dtor's base-subobject call to
// ??1CRezItmBase (0x13c520) instead of the fake RezDirBase placeholder.
struct CRezDir13cb80 : CRezItmBase {
    void* m_10;       // +0x10
    i32 m_14;         // +0x14
    RezOwner18* m_18; // +0x18
    virtual ~CRezDir13cb80() OVERRIDE;
};
SIZE_UNKNOWN(CRezDir13cb80);
RELOC_VTBL(CRezDir13cb80, 0x001ef7d0); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0013cb80, 0x72)
CRezDir13cb80::~CRezDir13cb80() {
    if (m_14) {
        ((CRezFile*)this)->Close();
    }
    if (m_10) {
        RezFree(m_10);
    }
    m_18->m_1c.Remove((CObjNode*)this);
}

// Read (0x13cc00): ensure the handle is open, seek to `pos` (retrying through the
// manager's gate), then fread `count` bytes into buf (retrying short reads through
// the same gate). Returns the bytes read (== count) or 0. `a` is the base-class
// signature's unused leading param. The `count <= 0` guard (unsigned) lowers to
// retail's `test;jbe` (docs/patterns/unsigned-zero-guard-le-not-eq.md).
RVA(0x0013cc00, 0x9f)
i32 CRezFile::Read(i32 a, i32 pos, u32 count, void* buf) {
    (void)a;
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        Open();
    }
    while (RezFSeek(m_handle, pos, 0) != 0) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
    }
    u32 got = RezFRead(buf, 1, count, m_handle);
    while (got != count) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        got = RezFRead(buf, 1, count, m_handle);
    }
    return got;
}

// Write (0x13cca0): the write counterpart of Read (RezFWrite for RezFRead); same
// open/seek/retry gating. Returns the bytes written (== count) or 0.
RVA(0x0013cca0, 0x9f)
i32 CRezFile::Write(i32 a, i32 pos, u32 count, void* buf) {
    (void)a;
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        Open();
    }
    while (RezFSeek(m_handle, pos, 0) != 0) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
    }
    u32 put = RezFWrite(buf, 1, count, m_handle);
    while (put != count) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        put = RezFWrite(buf, 1, count, m_handle);
    }
    return put;
}

// Flush (0x13cd60): fflush the handle, retrying through the manager's gate on
// failure. Returns 1 (no handle / flushed) or 0 (the gate gave up). Same clean
// `(fflush == 0)` neg/sbb/inc bool-normalize as Close (no list/handle teardown).
RVA(0x0013cd60, 0x49)
i32 CRezFile::Flush() {
    if (m_handle != 0) {
        i32 ok = (Eng_fflush(m_handle) == 0);
        while (!ok) {
            if (m_mgr->m_gate->Retry() == 0) {
                return 0;
            }
            ok = (Eng_fflush(m_handle) == 0);
        }
        return ok;
    }
    return 1;
}

// Open (0x13cdc0): lazily (re)open the handle. If already open, return 1. If the
// cache is over its cap, evict the LRU (the open-list tail) via Close(). Then fopen
// with the flag-selected mode ("w+b"/"rb"/"r+b"), retrying through the manager's
// gate on failure; on success move the node from the closed list to the open list
// and bump the open count. Returns 1 (opened) / 0 (gave up or an invalid w+ro mix).
RVA(0x0013cdc0, 0xad)
i32 CRezFile::Open() {
    if (m_handle != 0) {
        return 1;
    }
    if (m_mgr->m_openCount > m_mgr->m_maxOpen) {
        // Typed intrusive-list access: the LRU eviction candidate (the open list's
        // tail) is a CRezFile (zero-offset static downcast; see CloseAllOpen).
        CRezFile* lru = (CRezFile*)m_mgr->m_openList.m_tail;
        if (lru != 0) {
            lru->Close();
        }
    }
    for (;;) {
        if (m_mgr->m_write) {
            if (m_mgr->m_readonly) {
                return 0;
            }
            m_handle = Eng_fopen(m_name, s_wPlusB);
        } else if (m_mgr->m_readonly) {
            m_handle = Eng_fopen(m_name, s_rb);
        } else {
            m_handle = Eng_fopen(m_name, s_rPlusB);
        }
        if (m_handle != 0) {
            break;
        }
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        if (m_handle != 0) {
            break;
        }
    }
    m_mgr->m_closedList.Remove(this);
    m_mgr->m_openList.AddHead(this);
    m_mgr->m_openCount++;
    return 1;
}

// Close (0x13ce70): fclose the handle (retrying through the manager's gate); then
// drop the open count, move the node back to the closed list, and null the handle.
// Returns 1 (no handle / closed) or 0 (the gate gave up). BYTE-EXACT (100%): the
// anticipated esi<->edi coin-flip did not materialize here - MSVC5 pins the same
// registers as retail, so this method needs no early-stop marker.
RVA(0x0013ce70, 0x7c)
i32 CRezFile::Close() {
    if (m_handle == 0) {
        return 1;
    }
    i32 ok = (RezFClose(m_handle) == 0);
    while (!ok) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        ok = (RezFClose(m_handle) == 0);
    }
    m_mgr->m_openCount--;
    m_mgr->m_openList.Remove(this);
    m_mgr->m_closedList.AddHead(this);
    m_handle = 0;
    return ok;
}

// (RezMgr::UpdateClock @0x13ddc0, SpinWaitUntil @0x13dec0, SetFrameRate
// @0x13dee0, TrySetFrameRate @0x13df00 and ::WaitKeyEdge @0x13df30 moved to
// src/Wap32/GameApp.cpp in wave4-K: their text is A-B-A-woven into the GameApp
// obj between CGameMgr::Close/InitTimeFields - dossier #14E. RezMgr::MakeImageKey
// @0x13e5d0 moved to src/DDrawMgr/DDSurface.cpp - text-contained in the DIRSURF
// obj, dossier #14G.)

// (CRezDir::FindEntry @0x13c080 and CRezDirNode::Load @0x13a0f0 moved to
// src/Bute/SymTab.cpp in wave4-K: both are text-contained in the ButeMgr sym
// obj and Load's private .data cell 0x21a070 sits inside the sym band, BEFORE
// this obj's 0x21a0a4 mode strings - dossier #14A. Their Rez-flavored names are
// an @identity-TODO carried there.)

// ===========================================================================
// Class-metadata annotations for the Rez file-system classes (EOF-hosted; the Rez
// headers are pulled into /O2-sensitive TUs, so the headers stay untouched).
// ===========================================================================
SIZE(CRezItmBase, 0x10);               // "16 bytes" base (derived fields start at +0x10)
VTBL(CRezItmBase, 0x001ef768);         // base vtable stamp from ctor 0x13c4e0
SIZE(CRezItm, 0x24);                   // operator new leaf size 0x24
VTBL(CRezItm, 0x001ef788);             // derived vtable stamp from ctor 0x13c540
SIZE(CRezDirList, 0xc);                // embedded child-collection list {vptr,head,tail}
SIZE(CRezDir, 0x38);                   // verified: ParseBuffer `push 0x38; new; call 0x13c940`
SIZE(CRezParseNode, 0x1c);             // verified: ParseRecords `push 0x1c; new; call 0x13cac0`
RELOC_VTBL(CRezParseNode, 0x001ef7d0); // vtable reloc-masks a bound datum (dtor-stamp verified)
SIZE_UNKNOWN(RezStream);               // abstract slot-view (pure virtuals, no vtable)
SIZE_UNKNOWN(RezSrc);                  // partial view of the foreign archive-source object
SIZE_UNKNOWN(CRezDirNode);             // partial view of the loader's recursive dir node
