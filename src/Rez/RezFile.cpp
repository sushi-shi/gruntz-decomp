#include <Bute/SymParser.h> // CSymParser - the proven m_parent owner (Retry, slot 2)
#include <Rez/RezMgr.h>
#include <Rez/RezFile.h> // CRezFile (this TU's own class; shared decls)
#include <Rez/RezList.h> // CRezList AddHead (0x1851e0) - owner child-list enroll
#include <rva.h>

// The two private fopen-mode literals referenced ONLY by Open@CRezItm + OpenFile@
// CRezFile. cl mangles the `extern const char[]` reference with a `P` storage class,
// which a DATA() label (clang's `Q` mangledName) misses; DATA_SYMBOL names the exact
// cl mangling and is authority-checked against rezfile.obj's undefined externals.
// (s_rb @0x20b668 is bound by DirectSoundMgr.cpp, which shares it.)
DATA_SYMBOL(0x0021a0a4, 0x0, ?s_rPlusB@@3PBDB)
DATA_SYMBOL(0x0021a0a8, 0x0, ?s_wPlusB@@3PBDB)

VTBL(CRezDir, 0x001ef7a8);
VTBL(CRezList, 0x001ef7c8);
VTBL(CRezFile, 0x001ef7d0);
DATA(0x0021a0a0)
const char g_wildcard[] = "*.*"; // decl in RezFile.h


RVA(0x0013c4d0, 0x1)
void CRezList::V0() {}

RVA(0x0013c4e0, 0x12)
CRezItmBase::CRezItmBase(void* parent) {
    // Language-forced cast: the ctor's parameter is `void*` in the retail ABI
    // (mangled ??0CRezItmBase@@QAE@PAX@Z = PAX), while the stored member is the
    // typed CSymParser* (the proven parent; ex CRezItmOwner view). Storing the void* param into the typed
    // member requires the reinterpret.
    m_parent = static_cast<CSymParser*>(parent);
}

// The four cl-auto scalar-deleting destructors (vtable slot 1 of each class; the
// compiler generates them from the virtual dtors - no source symbol to RVA()-pin,
// so RVA_COMPGEN pairs the retail copies with the auto-emitted base COMDATs).
RVA_COMPGEN(0x0013c500, 0x1e, ??_GCRezItmBase@@UAEPAXI@Z)

RVA(0x0013c520, 0xe)
CRezItmBase::~CRezItmBase() {
    m_parent = 0;
}

RVA(0x0013c530, 0x1)
void CRezItmBase::Noop() {}

RVA(0x0013c540, 0x28)
CRezItm::CRezItm(void* parent) : CRezItmBase(parent) {
    m_fp = 0;
    m_readBuf = 0;
    m_pos = -1;
}

RVA_COMPGEN(0x0013c570, 0x1e, ??_GCRezItm@@UAEPAXI@Z)

RVA(0x0013c590, 0x66)
CRezItm::~CRezItm() {
    if (m_fp != 0) {
        Close();
    }
    if (m_readBuf != 0) {
        ::operator delete(m_readBuf);
    }
}

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

    m_readonly = readonly;
    if (m_readBuf != 0) {
        ::operator delete(m_readBuf);
    }
    m_readBuf = static_cast<char*>(::operator new(strlen(filename) + 1));
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

extern "C" i32 RezDirLookup(void* fp); // 0x18ccd0

RVA(0x0013c8f0, 0x41)
i32 CRezItm::Check() {
    m_pos = -1;
    if (!m_fp) {
        return 0;
    }
    if (RezDirLookup(m_fp) != -1) {
        return 1;
    }
    return Open(m_readBuf, m_readonly, 0) != 0;
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

RVA_COMPGEN(0x0013c990, 0x1e, ??_GCRezDir@@UAEPAXI@Z)

RVA(0x0013c9b0, 0x7f)
CRezDir::~CRezDir() {
    // Typed intrusive-list access: the children are CRezItmBase-derived nodes
    // (each `delete` dispatches the node's slot-1 scalar-deleting dtor).
    while (m_openList.m_head != 0) {
        delete reinterpret_cast<CRezItmBase*>(m_openList.m_head);
    }
    while (m_closedList.m_head != 0) {
        delete reinterpret_cast<CRezItmBase*>(m_closedList.m_head);
    }
}

// 0x13ca30 - the standalone out-of-line COMDAT copy of ~CRezList (inline in
// <Rez/RezList.h>), emitted in this obj because ~CRezDir's EH unwind funclets
// (0x1e0cb8/0x1e0cc3, the two member states) take the dtor's address. 7 bytes:
// the own-vtable stamp is dead-store-eliminated into the inlined ~CObjListBase
// base stamp (`mov [ecx],??_7CObjListBase; ret`).
RVA_COMPGEN(0x0013ca30, 0x7, ??1CRezList@@QAE@XZ)

RVA(0x0013ca40, 0x5)
i32 CRezDir::Read(i32 off, i32 base, u32 count, void* buf) {
    return 0;
}
RVA(0x0013ca50, 0x5)
i32 CRezDir::Write(i32 base, i32 off, u32 count, void* buf) {
    return 0;
}

RVA(0x0013ca60, 0x14)
i32 CRezDir::Open(char* name, i32 readonly, i32 write) {
    m_readonly = readonly;
    m_write = write;
    return 1;
}

RVA(0x0013ca80, 0x1d)
i32 CRezDir::Close() {
    // The list stores CRezFile nodes; retrieve the head as its concrete type (the
    // typed intrusive-list access - CRezFile's node base is at offset 0, so this is a
    // zero-offset static downcast, matching-neutral). CloseFile() is a direct call.
    while (m_openList.m_head != 0) {
        (reinterpret_cast<CRezFile*>(m_openList.m_head))->CloseFile();
    }
    return 1;
}

RVA(0x0013caa0, 0x6)
i32 CRezDir::Flush() {
    return 1;
}
RVA(0x0013cab0, 0x6)
i32 CRezDir::Check() {
    return 1;
}

RVA(0x0013cac0, 0x9b)
CRezFile::CRezFile(void* parent, char* nameSrc, CRezDir* dir) : CRezItmBase(parent) {
    m_dir = dir;
    m_handle = 0;
    // operator new returns void*; char* needed for strcpy (language-forced).
    char* buf = static_cast<char*>(::operator new(strlen(nameSrc) + 1));
    m_name = buf;
    strcpy(buf, nameSrc);
    // Enroll into the dir's closed list (new files start closed). The node param
    // is the type-erased CRezListNode view (AddHead links any node by its +4/+8
    // words, which CRezItmBase carries at the same offsets).
    m_dir->m_closedList.AddHead(reinterpret_cast<CRezListNode*>(this));
}

RVA_COMPGEN(0x0013cb60, 0x1e, ??_GCRezFile@@UAEPAXI@Z)

RVA(0x0013cb80, 0x72)
CRezFile::~CRezFile() {
    if (m_handle) {
        CloseFile();
    }
    if (m_name) {
        ::operator delete(m_name);
    }
    m_dir->m_closedList.Remove(reinterpret_cast<CObjNode*>(this));
}

RVA(0x0013cc00, 0x9f)
i32 CRezFile::Read(i32 a, i32 pos, u32 count, void* buf) {
    static_cast<void>(a);
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

RVA(0x0013cca0, 0x9f)
i32 CRezFile::Write(i32 a, i32 pos, u32 count, void* buf) {
    static_cast<void>(a);
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

RVA(0x0013cd40, 0x5)
i32 CRezFile::Open(char* name, i32 readonly, i32 write) {
    return 0;
}
RVA(0x0013cd50, 0x3)
i32 CRezFile::Close() {
    return 0;
}

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

RVA(0x0013cdb0, 0x3)
i32 CRezFile::Check() {
    return 0;
}

RVA(0x0013cdc0, 0xad)
i32 CRezFile::OpenFile() {
    if (m_handle != 0) {
        return 1;
    }
    if (m_dir->m_openCount > m_dir->m_maxOpen) {
        // Typed intrusive-list access: the LRU eviction candidate (the open list's
        // tail) is a CRezFile (zero-offset static downcast; see CRezDir::Close).
        CRezFile* lru = reinterpret_cast<CRezFile*>(m_dir->m_openList.m_tail);
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
    m_dir->m_closedList.Remove(reinterpret_cast<CObjNode*>(this));
    m_dir->m_openList.AddHead(reinterpret_cast<CRezListNode*>(this));
    m_dir->m_openCount++;
    return 1;
}

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
    m_dir->m_openList.Remove(reinterpret_cast<CObjNode*>(this));
    m_dir->m_closedList.AddHead(reinterpret_cast<CRezListNode*>(this));
    m_handle = 0;
    return ok;
}

RVA(0x0013cef0, 0x1)
void CRezFile::Noop() {}

// (CRezDir::FindEntry @0x13c080 and CRezDirNode::Load @0x13a0f0 moved to
// src/Bute/SymTab.cpp in wave4-K: both are text-contained in the ButeMgr sym
// obj and Load's private .data cell 0x21a070 sits inside the sym band, BEFORE
// this obj's 0x21a0a4 mode strings - dossier #14A. Their Rez-flavored names are
// an @identity-TODO carried there.)

VTBL(CRezItmBase, 0x001ef768); // base vtable stamp from ctor 0x13c4e0
VTBL(CRezItm, 0x001ef788);     // derived vtable stamp from ctor 0x13c540
