#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/FxModeT1.h> // the REAL default-init descriptors (ex-CFaderInit::BuildDefaultInit*)

#include <rva.h>
#include <Mfc.h>
#include <string.h> // memcpy/memset -> rep movs/stos in the inlined SetSize

extern "C" void Fader_Trace(const char* msg); // 0x1b9d4c - CString(const char*)/TRACE

static inline i32 InitTypeId(CFxModeDesc* pInit) {
    return pInit->m_type;
}

RVA(0x0017d8f0, 0x1e)
CFaderMgr::CFaderMgr() {
    m_active = 0;
    m_0c = 0;
}

// ===========================================================================
// 0x17d910 - ~CFaderMgr: empty the array (FreeAll), then the member array
// subobject teardown runs implicitly. /GX EH frame (from the member dtor).
// ===========================================================================
// @early-stop
// EH inline-member-stamp wall (docs/patterns/eh-dtor-inline-member-vtable-stamp-
// thisadjust.md): body byte-identical; residue is the inlined ~CFaderArray's
// this-adjust register-base + the /GX state value ($0x8 vs $0x0, state 1 vs -1).
// A clean exit needs an external ~CFaderArray (no separate RVA exists for it), not
// a manual vtable stamp. ~83%.
RVA(0x0017d910, 0x65)
CFaderMgr::~CFaderMgr() {
    FreeAll();
}

RVA(0x0017d980, 0x1f)
i32 CFaderMgr::SetConfig(i32 a, i32 b, i32 c) {
    m_timerArgA = a;
    m_timerArgB = b;
    m_sharedSet2cArg = c;
    m_active = 1;
    return 1;
}

RVA(0x0017d9a0, 0x11)
void CFaderMgr::FreeAll() {
    DeleteAll();
    m_active = 0;
}

// ===========================================================================
// 0x17d9c0 - Add(nFaderType, pInit): 7-way fader factory. Validate pInit's type-id
// against nFaderType, allocate the concrete subtype, prime it (SetTimers from
// m_timerArgA/m_timerArgB, Set2c from m_sharedSet2cArg), default- or copy-init it, validate, and append it
// to the array - tracing + deleting the fader on any failure. /GX EH frame; the
// SetAtGrow(GetSize(), pNew) append is inlined.
// @early-stop
// EH-state + inlined-SetAtGrow wall: six near-identical cases each wrap a
// destructible CFaderInit local (embedded ~CString) in a per-case /GX try region;
// MSVC5's EH-state machine + the inlined CObArray grow do not reproduce
// byte-for-byte from this spelling. Logic complete; parked for the final sweep.
RVA(0x0017d9c0, 0x786)
CFader* CFaderMgr::Add(i32 nFaderType, CFxModeDesc* pInit) {
    CFader* fader = 0;

    switch (nFaderType) {
        case 0: {
            if (pInit && InitTypeId(pInit) != 1) {
                goto wrongclass;
            }
            CFaderShape* f = new CFaderShape;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFxModeT1 init;
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->ApplyInit(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 1: {
            if (pInit && InitTypeId(pInit) != 2) {
                goto wrongclass;
            }
            CFaderLight* f = new CFaderLight;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFxModeT2 init;
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->ApplyInit(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 2: {
            if (pInit && InitTypeId(pInit) != 3) {
                goto wrongclass;
            }
            CFaderSine* f = new CFaderSine;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFxModeT3 init;
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->ApplyInit(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 3: {
            if (pInit && InitTypeId(pInit) != 4) {
                goto wrongclass;
            }
            CFaderRadial* f = new CFaderRadial;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFxModeT4 init;
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->ApplyInit(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 4: {
            if (pInit && InitTypeId(pInit) != 5) {
                goto wrongclass;
            }
            CFaderFlat* f = new CFaderFlat;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFxModeT5 init;
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->ApplyInit(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 5: {
            if (pInit && InitTypeId(pInit) != 6) {
                goto wrongclass;
            }
            CFaderMesh* f = new CFaderMesh;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFxModeT6 init;
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->ApplyInit(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        default:
        wrongclass:
            Fader_Trace(
                "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived class"
            );
            return 0;
    }

badinit:
    Fader_Trace("CFaderMgr::Add (...) - Invalid init class");
    delete fader;
    return 0;

append:
    if (fader) {
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
            if (newSize > newMax) {
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

// ===========================================================================
// 0x17e170 - Remove(pFader): find pFader in the array; on hit, memmove the tail
// down one slot, drop the count, and delete the fader (its scalar-deleting dtor).
// __thiscall, one arg.
// ===========================================================================
// @early-stop
// Regalloc wall: logic byte-for-byte correct, but MSVC5 assigns this->ebp /
// pFader->ebx while retail uses this->ebx / pFader->ebp (and reads m_nSize into a
// fresh temp vs a kept reg). Not source-steerable. ~67%.
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
// just has to NAME it, which is exactly what @rva-symbol is for. Naming it binds the
// two vptr stores to the real ??_7CFaderArray / ??_7CObject rvas (zero UNBOUND relocs)
// with no source-level duplicate and no change to the inline teardown.
// @rva-symbol: ??1CFaderArray@@UAE@XZ 0x0017e240 0x51

RVA(0x0017e230, 0xc)
void __stdcall Fader_TraceStr(CString s) {
    static_cast<void>(s);
}

// ===========================================================================
// 0x17e2a0 - CFaderArray::Serialize (slot 2): the MFC CObArray<CFader*>::Serialize
// with SetSize inlined. Storing: WriteCount then Write the raw 4-byte-element block;
// loading: ReadCount, resize the buffer (alloc / grow-with-copy / shrink-in-place per
// the m_nSize/8 clamped-[4,0x400] grow heuristic), then Read the raw block. Elements
// are 4-byte pointers stored as raw dwords. Archive helpers + operator new/delete are
// reloc-masked. (Was a declared-only slot in <Gruntz/FaderMgr.h>.)
// ===========================================================================
// @early-stop
// regalloc wall (~81%, same family as TArray::Serialize @0x39fa0 / the ArraySerialize
// twin): the SetSize load/grow/shrink logic + the store/load branch senses are byte-
// faithful, but retail pins this->ebx / ar->esi while cl lands ebp/edi, cascading the
// scratch edx/ecx picks through the realloc lea chains. Not source-steerable.
RVA(0x0017e2a0, 0x188)
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
