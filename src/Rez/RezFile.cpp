// RezFile.cpp - the Rez archive-container file-system family, carved out of
// src/Rez/RezMgr.cpp (holding-TU drain, 2026-07-11). ONE contiguous retail .text obj
// spanning 0x13c4d0-0x13ceec: the CRezItmBase/CRezItm/CRezDir directory-tree nodes
// AND the CRezFile managed open-file node (the dir doubles as the LRU handle cache;
// the former separate "CRezFileMgr"/"CRezParseNode" identities were CRezDir/CRezFile
// under second names - the vtable own-stamps prove it: ctor 0x13cac0 and dtor
// 0x13cb80 both stamp 0x5ef7d0 (=??_7CRezFile), ctor 0x13c940 and dtor 0x13c9b0
// both stamp 0x5ef7a8 (=??_7CRezDir)). Proven ONE file by the shared private
// fopen-mode literals 0x21a0a4 ("r+b") / 0x21a0a8 ("w+b"), referenced ONLY by
// Open@CRezItm + OpenFile@CRezFile, and the text A-B-A weave (CRezDir::Close
// @0x13ca80 inside the CRezItm run).
//
// Both node ctors share the base ctor CRezItmBase::CRezItmBase (stores the base vtable
// and the parent pointer @+0xc), then overwrite the vtable with the derived one
// (two-phase construction; all vtable stores reloc-masked). `operator new` sizes 0x24
// (leaf) / 0x38 (dir) / 0x1c (file) confirm the layouts.
//
// OpenSub is NOT matched here: it runs on a THIRD, distinct node layout (uses +0x1c as
// a child COUNT and +0x10 as a list-append target, conflicting with both the 0x38
// CRezDir ctor's vtable stores and CRezDirNode's +0x10 size / +0x18 source - so the
// three "CRezDir"-labeled functions are actually three different classes).
#include <Rez/RezMgr.h>
#include <Rez/RezFile.h> // CRezFile (this TU's own class; shared decls)
#include <Rez/RezList.h> // CRezList AddHead (0x1851e0) - owner child-list enroll
#include <rva.h>

// The two private fopen-mode literals referenced ONLY by Open@CRezItm + OpenFile@
// CRezFile. cl mangles the `extern const char[]` reference with a `P` storage class,
// which a DATA() label (clang's `Q` mangledName) misses; @data-symbol names the exact
// cl mangling and is authority-checked against rezfile.obj's undefined externals.
// (s_rb @0x20b668 is bound by DirectSoundMgr.cpp, which shares it.)
// @data-symbol: ?s_rPlusB@@3PBDB 0x0021a0a4
// @data-symbol: ?s_wPlusB@@3PBDB 0x0021a0a8

// The find-all glob directly PRECEDING them in this obj's .data run (0x61a0a0
// "*.*"; SymTab's directory walk externs it). Owner-TU definition; length
// NULL-TERMINATOR-PROVEN. extern "C" avoids the P/Q const-array mangling split.
DATA(0x0021a0a0)
extern "C" const char g_wildcard[] = "*.*";

// The four cl-auto scalar-deleting destructors (vtable slot 1 of each class; the
// compiler generates them from the virtual dtors - no source symbol to RVA()-pin,
// so @rva-symbol pairs the retail copies with the auto-emitted base COMDATs).
// @rva-symbol: ??_GCRezItmBase@@UAEPAXI@Z 0x0013c500 0x1e
// @rva-symbol: ??_GCRezItm@@UAEPAXI@Z 0x0013c570 0x1e
// @rva-symbol: ??_GCRezDir@@UAEPAXI@Z 0x0013c990 0x1e
// @rva-symbol: ??_GCRezFile@@UAEPAXI@Z 0x0013cb60 0x1e

// ---------------------------------------------------------------------------
// CRezList::V0 (0x13c4d0, vtable slot 0 - the CObjListBase pure-slot override):
// an empty body (bare ret). The original method's role is unrecovered; the slot's
// EXISTENCE + emptiness is the binary truth (retail ??_7CRezList @0x1ef7c8 is
// exactly this one slot; the sibling parser-list's empty slot-0 copy is 0x13c4c0).
RVA(0x0013c4d0, 0x1)
void CRezList::V0() {}

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
// CRezItmBase::Slot00_13c530 (vtable slot 0): an empty body (bare ret). The
// base's concrete slot-0 default; CRezFile carries its own identical empty copy
// (0x13cef0), CRezItm/CRezDir inherit this one.
RVA(0x0013c530, 0x1)
void CRezItmBase::Slot00_13c530() {}

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
        ::operator delete(m_readBuf);
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
// unsigned-zero-guard-le-not-eq.md). The four buffered-IO calls are the real CRT
// fseek/fread (retail 0x18c3a0/0x18c220), now bound by name (reloc-faithful).
RVA(0x0013c600, 0xbd)
i32 CRezItm::Read(i32 off, i32 base, u32 count, void* buf) {
    if (count <= 0) {
        return 0;
    }

    i32 pos = base + off;

    if (m_pos != pos) {
        while (fseek(m_fp, pos, 0) != 0) {
            if (m_parent->Retry() == 0) {
                m_pos = -1;
                return 0;
            }
        }
    }

    u32 got = fread(buf, 1, count, m_fp);
    while (got != count) {
        if (m_parent->Retry() == 0) {
            m_pos = -1;
            return 0;
        }
        got = fread(buf, 1, count, m_fp);
    }

    m_pos = got + pos;
    return got;
}

// ---------------------------------------------------------------------------
// CRezItm::Write(base, off, count, buf)  (vtable slot 3)
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
// unsigned-zero-guard-le-not-eq.md). The buffered-IO calls are the real CRT
// fseek/fwrite (retail 0x18c3a0/0x18cb40), now bound by name (reloc-faithful).
RVA(0x0013c6c0, 0x97)
i32 CRezItm::Write(i32 base, i32 off, u32 count, void* buf) {
    m_pos = -1;
    if (count <= 0) {
        return 0;
    }

    i32 pos = off + base;

    while (fseek(m_fp, pos, 0) != 0) {
        if (m_parent->Retry() == 0) {
            return 0;
        }
    }

    u32 put = fwrite(buf, 1, count, m_fp);
    while (put != count) {
        if (m_parent->Retry() == 0) {
            return 0;
        }
        put = fwrite(buf, 1, count, m_fp);
    }
    return put;
}

// ---------------------------------------------------------------------------
// CRezItm::Open(filename, readonly, write)  (vtable slot 4)
// Pick the fopen mode from the readonly/write flags (write+readonly is invalid),
// (re)open the FILE* recovering through the owner's Retry() gate, then stash the
// readonly flag (m_18), keep a RezAlloc'd copy of the filename (m_readBuf) and
// reset the position cursor. Same mode ladder as CRezFile::OpenFile.
RVA(0x0013c760, 0xc1)
i32 CRezItm::Open(char* filename, i32 readonly, i32 write) {
    for (;;) {
        if (write) {
            if (readonly) {
                return 0;
            }
            m_fp = fopen(filename, s_wPlusB);
        } else if (readonly) {
            m_fp = fopen(filename, s_rb);
        } else {
            m_fp = fopen(filename, s_rPlusB);
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
        ::operator delete(m_readBuf);
    }
    m_readBuf = (char*)::operator new(strlen(filename) + 1);
    if (m_readBuf != 0) {
        strcpy(m_readBuf, filename);
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
        if (fclose(m_fp) == 0) {
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
        ::operator delete(m_readBuf);
    }
    m_readBuf = 0;
    m_pos = -1;
    return ok;
}

// ---------------------------------------------------------------------------
// CRezItm::Flush()  (vtable slot 6; re-homed from src/Stub/BoundaryUpper.cpp)
// Reset the position cursor, then (if a FILE* is open) fflush the stream, retrying
// through the owner's Retry() gate until the flush succeeds (fflush==0). Returns 1
// once flushed, 0 if no FILE* or the gate gave up. (The 0x125b50 callee is the CRT
// fflush; a prior reconstruction reloc-masked it under the fake name RezItmProbe.)
RVA(0x0013c8a0, 0x45)
i32 CRezItm::Flush() {
    m_pos = -1;
    if (m_fp) {
        i32 found;
        do {
            if (fflush(m_fp) == 0) {
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
// return 1, else re-Open from the stored filename/flags and normalize to bool. The
// Open self-call dispatches through vtable slot 4 (`mov eax,[this]; call [eax+0x10]`)
// exactly as retail - the former "slot-4-devirt wall" was the non-virtual model, now
// dissolved by the real CRezItmBase 8-slot interface.
RVA(0x0013c8f0, 0x41)
i32 CRezItm::Check() {
    m_pos = -1;
    if (!m_fp) {
        return 0;
    }
    if (RezDirLookup(m_fp) != -1) {
        return 1;
    }
    return Open(m_readBuf, m_18, 0) != 0;
}

// ---------------------------------------------------------------------------
// CRezDir::CRezDir(parent, maxOpen)
// Base ctor, then the two embedded child-collection list members auto-construct
// (each stamps ??_7CRezList @0x1ef7c8 and zeroes head/tail), the derived vtbl
// is stamped, then m_openCount=0, m_write=0, m_maxOpen=maxOpen, m_readonly=1.
// @early-stop
// vptr-schedule wall (ALL-VTABLES): real-polymorphic list members auto-stamp their
// vptr FIRST (vptr,head,tail) and the compiler zeroes m_openCount/m_write after the
// derived vptr, vs retail's head,tail,vtbl store order + pre-vptr field zeroing. The
// two child collections + the CRezItmBase base are byte-faithful; converted per the
// ALL-VTABLES mandate (was the hand-rolled child-collection double-stamp).
RVA(0x0013c940, 0x46)
CRezDir::CRezDir(void* parent, i32 maxOpen) : CRezItmBase(parent) {
    m_openCount = 0;
    m_write = 0;
    m_maxOpen = maxOpen;
    m_readonly = 1;
}

// ---------------------------------------------------------------------------
// CRezDir::~CRezDir (0x13c9b0; was the CRezDir13c9b0 placeholder - its own-vtable
// stamp 0x5ef7a8 == the ctor's, one class): delete every child node in the open
// then the closed list (each `delete` dispatches the node's slot-1 scalar-deleting
// dtor: `mov eax,[node]; push 1; call [eax+4]`, and each Close/dtor unlinks the
// node so the head re-reads advance). Then the two CRezList members destruct
// (inlined: /O2 dead-store-eliminates their own-vtable re-stamp into the single
// shared ??_7CObjListBase store retail shows - `mov eax,0x5ef760; mov [esi+0x1c],
// eax; mov [esi+0x10],eax`, reverse member order) and ~CRezItmBase folds. /GX
// frame with the two member states (retail trylevel 2).
RVA(0x0013c9b0, 0x7f)
CRezDir::~CRezDir() {
    // Typed intrusive-list access: the children are CRezItmBase-derived nodes
    // (each `delete` dispatches the node's slot-1 scalar-deleting dtor).
    while (m_openList.m_head != 0) {
        delete (CRezItmBase*)m_openList.m_head;
    }
    while (m_closedList.m_head != 0) {
        delete (CRezItmBase*)m_closedList.m_head;
    }
}

// 0x13ca30 - the standalone out-of-line COMDAT copy of ~CRezList (inline in
// <Rez/RezList.h>), emitted in this obj because ~CRezDir's EH unwind funclets
// (0x1e0cb8/0x1e0cc3, the two member states) take the dtor's address. 7 bytes:
// the own-vtable stamp is dead-store-eliminated into the inlined ~CObjListBase
// base stamp (`mov [ecx],??_7CObjListBase; ret`). (Was the fake placeholder
// CAbstract13ca30, a RELOC_VTBL alias.)
// @rva-symbol: ??1CRezList@@QAE@XZ 0x0013ca30 0x7

// ---------------------------------------------------------------------------
// CRezDir::Read/Write (0x13ca40/0x13ca50, vtable slots 2/3): a directory is not a
// byte stream - both stubs return 0.
RVA(0x0013ca40, 0x5)
i32 CRezDir::Read(i32 off, i32 base, u32 count, void* buf) {
    return 0;
}
RVA(0x0013ca50, 0x5)
i32 CRezDir::Write(i32 base, i32 off, u32 count, void* buf) {
    return 0;
}

// CRezDir::Open (0x13ca60, vtable slot 4): latch the readonly/write mode flags
// the children's lazy OpenFile selects the fopen mode by; the name is unused.
RVA(0x0013ca60, 0x14)
i32 CRezDir::Open(char* name, i32 readonly, i32 write) {
    m_readonly = readonly;
    m_write = write;
    return 1;
}

// CRezDir::Close (0x13ca80, vtable slot 5): drain the open-handle list - CloseFile()
// each child until the list empties (each CloseFile moves the node off the open
// list, advancing the head). Returns 1. (Was misattributed as a separate
// "CRezFileMgr::CloseAllOpen"; the vtable places it as CRezDir's slot 5.)
RVA(0x0013ca80, 0x1d)
i32 CRezDir::Close() {
    // The list stores CRezFile nodes; retrieve the head as its concrete type (the
    // typed intrusive-list access - CRezFile's node base is at offset 0, so this is a
    // zero-offset static downcast, matching-neutral). CloseFile() is a direct call.
    while (m_openList.m_head != 0) {
        ((CRezFile*)m_openList.m_head)->CloseFile();
    }
    return 1;
}

// CRezDir::Flush/Check (0x13caa0/0x13cab0, vtable slots 6/7): trivial successes.
RVA(0x0013caa0, 0x6)
i32 CRezDir::Flush() {
    return 1;
}
RVA(0x0013cab0, 0x6)
i32 CRezDir::Check() {
    return 1;
}

// -------------------------------------------------------------------------
// CRezFile::CRezFile(parent, nameSrc, dir)  (0x13cac0; was the fake identity
// "CRezParseNode" - the ctor stamps 0x5ef7d0, the same own vtable as the dtor
// 0x13cb80). Base-ctors CRezItmBase(parent), stamps the derived vtable, records
// the owning dir @+0x18, heap-copies the filename into +0x10, and enrolls itself
// into the dir's closed list via CRezList::AddHead. Real-polymorphic so the
// base-ctor call + derived-vptr stamp + /GX base-cleanup frame fall out.
RVA(0x0013cac0, 0x9b)
CRezFile::CRezFile(void* parent, char* nameSrc, CRezDir* dir) : CRezItmBase(parent) {
    m_dir = dir;
    m_handle = 0;
    // operator new returns void*; char* needed for strcpy (language-forced).
    char* buf = (char*)::operator new(strlen(nameSrc) + 1);
    m_name = buf;
    strcpy(buf, nameSrc);
    // Enroll into the dir's closed list (new files start closed). The node param
    // is the type-erased CRezListNode view (AddHead links any node by its +4/+8
    // words, which CRezItmBase carries at the same offsets).
    m_dir->m_closedList.AddHead((CRezListNode*)this);
}

// ---------------------------------------------------------------------------
// CRezFile::~CRezFile (0x13cb80; was the CRezDir13cb80 placeholder): close the
// handle if open (CloseFile, direct call 0x13ce70), free the heap filename, then
// unlink from the dir's closed list (CloseFile just moved it there) and fold the
// CRezItmBase base. /GX frame from the destructible base subobject.
RVA(0x0013cb80, 0x72)
CRezFile::~CRezFile() {
    if (m_handle) {
        CloseFile();
    }
    if (m_name) {
        ::operator delete(m_name);
    }
    m_dir->m_closedList.Remove((CObjNode*)this);
}

// Read (0x13cc00, vtable slot 2): ensure the handle is open, seek to `pos`
// (retrying through the dir's gate), then fread `count` bytes into buf (retrying
// short reads through the same gate). Returns the bytes read (== count) or 0.
// `a` is the base signature's unused leading param. The `count <= 0` guard
// (unsigned) lowers to retail's `test;jbe` (docs/patterns/
// unsigned-zero-guard-le-not-eq.md).
RVA(0x0013cc00, 0x9f)
i32 CRezFile::Read(i32 a, i32 pos, u32 count, void* buf) {
    (void)a;
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        OpenFile();
    }
    while (fseek(m_handle, pos, 0) != 0) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
    }
    u32 got = fread(buf, 1, count, m_handle);
    while (got != count) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        got = fread(buf, 1, count, m_handle);
    }
    return got;
}

// Write (0x13cca0, vtable slot 3): the write counterpart of Read (RezFWrite for
// RezFRead); same open/seek/retry gating. Returns the bytes written (== count) or 0.
RVA(0x0013cca0, 0x9f)
i32 CRezFile::Write(i32 a, i32 pos, u32 count, void* buf) {
    (void)a;
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        OpenFile();
    }
    while (fseek(m_handle, pos, 0) != 0) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
    }
    u32 put = fwrite(buf, 1, count, m_handle);
    while (put != count) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        put = fwrite(buf, 1, count, m_handle);
    }
    return put;
}

// CRezFile::Open/Close (0x13cd40/0x13cd50, vtable slots 4/5): trivial refusals -
// the managed file opens/closes ONLY through the non-virtual lazy OpenFile/
// CloseFile cache path, never through the generic stream slots.
RVA(0x0013cd40, 0x5)
i32 CRezFile::Open(char* name, i32 readonly, i32 write) {
    return 0;
}
RVA(0x0013cd50, 0x3)
i32 CRezFile::Close() {
    return 0;
}

// Flush (0x13cd60, vtable slot 6): fflush the handle, retrying through the dir's
// gate on failure. Returns 1 (no handle / flushed) or 0 (the gate gave up). Same
// clean `(fflush == 0)` neg/sbb/inc bool-normalize as CloseFile.
RVA(0x0013cd60, 0x49)
i32 CRezFile::Flush() {
    if (m_handle != 0) {
        i32 ok = (fflush(m_handle) == 0);
        while (!ok) {
            if (m_dir->m_parent->Retry() == 0) {
                return 0;
            }
            ok = (fflush(m_handle) == 0);
        }
        return ok;
    }
    return 1;
}

// CRezFile::Check (0x13cdb0, vtable slot 7): trivial refusal (the cache node has
// no re-acquire probe; contrast CRezItm::Check's registry lookup).
RVA(0x0013cdb0, 0x3)
i32 CRezFile::Check() {
    return 0;
}

// OpenFile (0x13cdc0, non-virtual): lazily (re)open the handle. If already open,
// return 1. If the cache is over its cap, evict the LRU (the open-list tail) via
// CloseFile. Then fopen with the flag-selected mode ("w+b"/"rb"/"r+b"), retrying
// through the dir's gate on failure; on success move the node from the closed
// list to the open list and bump the open count. Returns 1 (opened) / 0 (gave up
// or an invalid w+ro mix).
RVA(0x0013cdc0, 0xad)
i32 CRezFile::OpenFile() {
    if (m_handle != 0) {
        return 1;
    }
    if (m_dir->m_openCount > m_dir->m_maxOpen) {
        // Typed intrusive-list access: the LRU eviction candidate (the open list's
        // tail) is a CRezFile (zero-offset static downcast; see CRezDir::Close).
        CRezFile* lru = (CRezFile*)m_dir->m_openList.m_tail;
        if (lru != 0) {
            lru->CloseFile();
        }
    }
    for (;;) {
        if (m_dir->m_write) {
            if (m_dir->m_readonly) {
                return 0;
            }
            m_handle = fopen(m_name, s_wPlusB);
        } else if (m_dir->m_readonly) {
            m_handle = fopen(m_name, s_rb);
        } else {
            m_handle = fopen(m_name, s_rPlusB);
        }
        if (m_handle != 0) {
            break;
        }
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        if (m_handle != 0) {
            break;
        }
    }
    m_dir->m_closedList.Remove((CObjNode*)this);
    m_dir->m_openList.AddHead((CRezListNode*)this);
    m_dir->m_openCount++;
    return 1;
}

// CloseFile (0x13ce70, non-virtual): fclose the handle (retrying through the dir's
// gate); then drop the open count, move the node back to the closed list, and null
// the handle. Returns 1 (no handle / closed) or 0 (the gate gave up). BYTE-EXACT
// (100%): the anticipated esi<->edi coin-flip did not materialize here - MSVC5 pins
// the same registers as retail, so this method needs no early-stop marker.
RVA(0x0013ce70, 0x7c)
i32 CRezFile::CloseFile() {
    if (m_handle == 0) {
        return 1;
    }
    i32 ok = (fclose(m_handle) == 0);
    while (!ok) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        ok = (fclose(m_handle) == 0);
    }
    m_dir->m_openCount--;
    m_dir->m_openList.Remove((CObjNode*)this);
    m_dir->m_closedList.AddHead((CRezListNode*)this);
    m_handle = 0;
    return ok;
}

// CRezFile::Slot00_13c530 (0x13cef0, vtable slot 0): the class's own empty copy of the
// slot-0 default (retail emits it standalone; the base's identical copy is
// 0x13c530 - MSVC5 has no ICF, so both bodies survive).
RVA(0x0013cef0, 0x1)
void CRezFile::Slot00_13c530() {}

// (WAP32::CGameMgr::PerFrameTick @0x13ddc0 (ex "RezMgr::UpdateClock"; the base
// vtable slot 4), SpinWaitUntil @0x13dec0, SetFrameRate @0x13dee0, TrySetFrameRate
// @0x13df00 and ::WaitKeyEdge @0x13df30 live in src/Wap32/GameApp.cpp (wave4-K):
// their text is A-B-A-woven into the GameApp obj between CGameMgr::Close/
// InitTimeFields - dossier #14E. MakeImageKey @0x13e5d0 moved to
// src/DDrawMgr/DDSurface.cpp - text-contained in the DIRSURF obj, dossier #14G.)

// (CRezDir::FindEntry @0x13c080 and CRezDirNode::Load @0x13a0f0 moved to
// src/Bute/SymTab.cpp in wave4-K: both are text-contained in the ButeMgr sym
// obj and Load's private .data cell 0x21a070 sits inside the sym band, BEFORE
// this obj's 0x21a0a4 mode strings - dossier #14A. Their Rez-flavored names are
// an @identity-TODO carried there.)

// ===========================================================================
// Class-metadata annotations for the Rez file-system classes (EOF-hosted; the Rez
// headers are pulled into /O2-sensitive TUs, so the headers stay untouched).
// ===========================================================================
SIZE(CRezItmBase, 0x10);       // "16 bytes" base (derived fields start at +0x10)
VTBL(CRezItmBase, 0x001ef768); // base vtable stamp from ctor 0x13c4e0
SIZE(RezFindRec, 0x24);        // RE'd WIN32-find-style fixed record
SIZE_UNKNOWN(CRezItmOwner);    // abstract Retry-gate interface (no storage/vtable here)
SIZE(CRezItm, 0x24);           // operator new leaf size 0x24
VTBL(CRezItm, 0x001ef788);     // derived vtable stamp from ctor 0x13c540
SIZE(CRezDir, 0x38);           // verified: ParseBuffer `push 0x38; new; call 0x13c940`
SIZE_UNKNOWN(RezStream);       // abstract slot-view (pure virtuals, no vtable)
SIZE_UNKNOWN(RezSrc);          // partial view of the foreign archive-source object
SIZE_UNKNOWN(CRezDirNode);     // partial view of the loader's recursive dir node
