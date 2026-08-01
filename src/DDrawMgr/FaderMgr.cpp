#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/FxModeT1.h> // the REAL default-init descriptors (ex-CFaderInit::BuildDefaultInit*)

#include <rva.h>
#include <Mfc.h>
#include <string.h> // memcpy/memset -> rep movs/stos in the inlined SetSize

VTBL(CFaderArray, 0x001f0790); // own vftable @0x5f0790 (uncatalogued -> ??_7CFaderArray)

RVA(0x0017d8f0, 0x1e)
CFaderMgr::CFaderMgr() {
    m_active = 0;
    m_0c = 0;
}

// ===========================================================================
// 0x17d910 - ~CFaderMgr: empty the array (FreeAll), then the member array
// subobject teardown runs implicitly. /GX EH frame (from the member dtor).
// ===========================================================================
RVA(0x0017d910, 0x65)
CFaderMgr::~CFaderMgr() {
    FreeAll();
}

RVA(0x0017d980, 0x1f)
i32 CFaderMgr::SetConfig(CDDSurface* a, CDDSurface* b, CDDrawPtrCollections* pool) {
    m_timerArgA = a;
    m_timerArgB = b;
    m_sharedPtrColl = pool;
    m_active = 1;
    return 1;
}

RVA(0x0017d9a0, 0x11)
void CFaderMgr::FreeAll() {
    DeleteAll();
    m_active = 0;
}

// ===========================================================================
// 0x17d9c0 - Add(nFaderType, pInit): the 6-way fader factory, and the biggest body in
// the TU (0x786 B) because retail duplicates the whole failure epilogue into every arm.
//
// Per arm: reject a pInit whose descriptor tag is not the arm's own (trace + return 0);
// `new` the concrete subtype; prime it from the manager's two default surfaces
// (SetTimers) and the shared pointer pool (Set2c); then apply either the caller's
// descriptor or a default-constructed CFxModeT<n> built on the stack. A rejected
// descriptor traces, deletes the half-built fader and returns 0 - and that trace/delete/
// return trio is emitted TWICE per arm (once per init branch), which is why the retail
// bytes hold twelve copies of the "Invalid init class" literal.
//
// Only case 0's CFxModeT1 has a non-trivial destructor (a CString at +0x24), so only
// that arm carries EH states 0 and 1 and an inlined ~CString; the other five arms get a
// single `new`-cleanup state each (2..6) and no init teardown.
//
// An out-of-range nFaderType traces a DIFFERENT message ("nFaderType is invalid") and
// falls into the shared tail with fader still null, so the append is skipped and 0 is
// returned - it is a `break`, not a `return`.
//
// The tail is CObArray::SetAtGrow(GetSize(), pNew) fully inlined: the four-way resize
// (empty / first alloc / fits in the current capacity / grow-with-copy under the
// m_nSize/8 clamped-[4,0x400] heuristic) followed by the element store.
// /GX EH frame.
//
// The RVA span includes the trailing jump table: code ends 0x17e146 (`jmp 0x17dace`),
// 2 pad bytes (`8b ff`), then 6 dwords at 0x17e148..0x17e160 all targeting inside Add.
// ===========================================================================
// @early-stop
RVA(0x0017d9c0, 0x7a0)
CFader* CFaderMgr::Add(i32 nFaderType, CFxModeDesc* pInit) {
    CFader* fader = 0;

    switch (nFaderType) {
        case 0: {
            if (pInit != 0 && pInit->m_type != 1) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderShape* f = new CFaderShape;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT1 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 1: {
            if (pInit != 0 && pInit->m_type != 2) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderLight* f = new CFaderLight;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT2 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 2: {
            if (pInit != 0 && pInit->m_type != 3) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderSine* f = new CFaderSine;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT3 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 3: {
            if (pInit != 0 && pInit->m_type != 4) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderRadial* f = new CFaderRadial;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT4 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 4: {
            if (pInit != 0 && pInit->m_type != 5) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderFlat* f = new CFaderFlat;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT5 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 5: {
            if (pInit != 0 && pInit->m_type != 6) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderMesh* f = new CFaderMesh;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT6 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        default:
            Trace("CFaderMgr::Add (...) - nFaderType is invalid");
            break;
    }

    if (fader != 0) {
        i32 idx = m_arr.m_nSize;
        i32 newSize = idx + 1;
        if (newSize == 0) {
            if (m_arr.m_pData) {
                operator delete(m_arr.m_pData);
                m_arr.m_pData = 0;
            }
            m_arr.m_nMaxSize = 0;
            m_arr.m_nSize = 0;
        } else if (m_arr.m_pData == 0) {
            m_arr.m_pData = static_cast<CFader**>(operator new(newSize * 4));
            memset(m_arr.m_pData, 0, newSize * 4);
            m_arr.m_nMaxSize = newSize;
            m_arr.m_nSize = newSize;
        } else if (newSize <= m_arr.m_nMaxSize) {
            if (newSize > idx) {
                memset(&m_arr.m_pData[idx], 0, (newSize - idx) * 4);
            }
            m_arr.m_nSize = newSize;
        } else {
            i32 grow = m_arr.m_nGrowBy;
            if (grow == 0) {
                grow = idx / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newMax = m_arr.m_nMaxSize + grow;
            // `>=`, not `>`: retail's guard is `cmp <newSize>,<newMax> / jl <keep>`,
            // so the clamp fires on equal too (MFC's own `if (nNewSize < m_nMaxSize +
            // nGrowBy) nNewMax = m_nMaxSize + nGrowBy; else nNewMax = nNewSize;`).
            if (newSize >= newMax) {
                newMax = newSize;
            }
            CFader** nd = static_cast<CFader**>(operator new(newMax * 4));
            memcpy(nd, m_arr.m_pData, m_arr.m_nSize * 4);
            memset(&nd[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
            operator delete(m_arr.m_pData);
            m_arr.m_pData = nd;
            m_arr.m_nSize = newSize;
            m_arr.m_nMaxSize = newMax;
        }
        m_arr.m_pData[idx] = fader;
    }
    return fader;
}

// The stack CFxModeT1 in Add makes VC5 materialize its otherwise implicit
// destructor out of line: destroy the CString member at +0x24.
RVA_COMPGEN(0x0017e160, 0x8, ??1CFxModeT1@@QAE@XZ)

// ===========================================================================
// 0x17e170 - Remove(pFader): find pFader in the array; on hit, memmove the tail
// down one slot, drop the count, and delete the fader (its scalar-deleting dtor).
// __thiscall, one arg.
// ===========================================================================
// @early-stop
RVA(0x0017e170, 0x5b)
void CFaderMgr::Remove(CFader* pFader) {
    i32 i = 0;
    i32 last = m_arr.m_nSize - 1;
    if (last >= 0) {
        CFader** w = m_arr.m_pData;
        while (*w != pFader) {
            i++;
            w++;
            if (i > last) {
                return;
            }
        }
        i32 cnt = m_arr.m_nSize - i - 1;
        CFader** dst = &m_arr.m_pData[i];
        if (cnt) {
            memcpy(dst, dst + 1, cnt * sizeof(CFader*));
        }
        m_arr.m_nSize--;
        delete pFader;
    }
}

RVA(0x0017e1d0, 0x4d)
void CFaderMgr::DeleteAll() {
    i32 i = 0;
    i32 last = m_arr.m_nSize - 1;
    if (last >= 0) {
        do {
            CFader* p = m_arr.m_pData[i];
            delete p;
            i++;
            last = m_arr.m_nSize - 1;
        } while (i <= last);
    }
    if (m_arr.m_pData) {
        operator delete(m_arr.m_pData);
        m_arr.m_pData = 0;
    }
    m_arr.m_nMaxSize = 0;
    m_arr.m_nSize = 0;
}

RVA(0x0017e230, 0xc)
void CFaderMgr::Trace(CString s) {
    static_cast<void>(s);
}

// 0x17e240 - ~CFaderArray, out-of-line. This is NOT a distinct class: it is the COMDAT
// copy cl emits of CFaderArray's INLINE destructor (<Gruntz/FaderMgr.h>), which the
// vtable slot's function pointer forces out of line. The former `C17e240 : Sev17e240`
// pair here was a FAKE VIEW of it - a hand-written clone of the same body over two
// address-minted placeholder classes, both of whose cl-emitted vtables merely MASKED a
// real, already-named retail vtable (C17e240 -> ??_7CFaderArray@@6B@ @0x1f0790,
// Sev17e240 -> the grand-base ??_7CObject@@6B@ @0x1e8cb4). Both are dissolved.
//
// No source definition is needed - and none may be written: ~CFaderArray must stay
// INLINE (retail ~CFaderMgr @0x17d910 inlines this member teardown - no call at that
// offset in its reloc table), and an out-of-line definition here would break that. cl
// ALREADY emits `??1CFaderArray@@UAE@XZ` into this obj for the vtable slot; the label
// just has to NAME it, which is exactly what RVA_COMPGEN is for. Naming it binds the
// two vptr stores to the real ??_7CFaderArray / ??_7CObject rvas (zero UNBOUND relocs)
// with no source-level duplicate and no change to the inline teardown.
RVA_COMPGEN(0x0017e240, 0x51, ??1CFaderArray@@UAE@XZ)

// ===========================================================================
// 0x17e2a0 - CFaderArray::Serialize (slot 2): the MFC CObArray<CFader*>::Serialize
// with SetSize inlined. Storing: WriteCount then Write the raw 4-byte-element block;
// loading: ReadCount, resize the buffer (alloc / grow-with-copy / shrink-in-place per
// the m_nSize/8 clamped-[4,0x400] grow heuristic), then Read the raw block. Elements
// are 4-byte pointers stored as raw dwords. Archive helpers + operator new/delete are
// reloc-masked. (Was a declared-only slot in <Gruntz/FaderMgr.h>.)
// ===========================================================================
// @early-stop
RVA(0x0017e2a0, 0x188)
RVA_COMPGEN(0x0017e430, 0x1e, ??_GCFaderArray@@UAEPAXI@Z)
void CFaderArray::Serialize(CArchive& ar) {
    if (ar.IsStoring()) {
        ar.WriteCount(m_nSize);
    } else {
        i32 n = ar.ReadCount();
        if (n == 0) {
            if (m_pData != 0) {
                ::operator delete(m_pData);
                m_pData = 0;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == 0) {
            m_pData = static_cast<CFader**>(::operator new(n * 4));
            memset(m_pData, 0, n * 4);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                memset(m_pData + m_nSize, 0, (n - m_nSize) * 4);
            }
            m_nSize = n;
        } else {
            i32 grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newMax = m_nMaxSize + grow;
            if (n >= newMax) {
                newMax = n;
            }
            CFader** nd = static_cast<CFader**>(::operator new(newMax * 4));
            memcpy(nd, m_pData, m_nSize * 4);
            memset(nd + m_nSize, 0, (n - m_nSize) * 4);
            ::operator delete(m_pData);
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newMax;
        }
    }
    if (ar.IsStoring()) {
        ar.Write(m_pData, m_nSize * 4);
    } else {
        ar.Read(m_pData, m_nSize * 4);
    }
}
