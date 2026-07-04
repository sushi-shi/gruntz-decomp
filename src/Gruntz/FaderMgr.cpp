// CFaderMgr.cpp - the Gruntz screen-fader manager (tracer placeholder
// tomalla-48): a polymorphic owner of a growable CPtrArray of CFader*.
// CFaderMgr::Add is a 7-way factory keyed on nFaderType (0..5): allocate the
// concrete CFader subtype (CFader1816c0/CFader180410/CFaderSine/CFader17f9a0/
// CFaderFlat/CFader17e940), prime it from the manager's shared timing fields
// (SetTimers/Set2c inherited from CFader), default- or copy-init it from pInit,
// validate, and append it - tracing "CFaderMgr::Add (...) - ..." + deleting the
// fader on any failure.
//
// The subtypes derive from CFader (see <Gruntz/FaderSubtypes.h>), so the factory
// upcasts implicitly (fader = f) and calls the inherited setters directly - no
// (CFader*)/(CFaderImpl*) casts.
//
// Methods in ascending retail-RVA order. Field names are placeholders; offsets +
// code bytes are load-bearing.
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSubtypes.h>

#include <rva.h>
#include <Mfc.h>

// ===========================================================================
// Reloc-masked externals.
// ===========================================================================

// The "CFaderMgr::Add (...)" diagnostic strings (0x624e04/0x624e34/0x624e88).
// Modeled as their real literals so the constants land in $SG and the call sites
// match by content (reloc-masked).
extern "C" void Fader_Trace(const char* msg); // 0x1b9d4c - CString(const char*)/TRACE

// The default-init descriptor built on the Add stack when pInit is null: an
// embedded CString forces the /GX frame, and the subtype's default parameters are
// filled by one of the six reloc-masked default builders (__thiscall on the
// descriptor), one per fader type (0..5).
SIZE_UNKNOWN(CFaderInit);
struct CFaderInit {
    char m_blob[0x24]; // +0x00..+0x23
    CString m_str;     // +0x24  destructible member (forces /GX)

    void BuildDefaultInit0(); // 0x17e7c0  fader type 0
    void BuildDefaultInit1(); // 0x17e840  fader type 1
    void BuildDefaultInit2(); // 0x17e880  fader type 2
    void BuildDefaultInit3(); // 0x17e8b0  fader type 3
    void BuildDefaultInit4(); // 0x17e8e0  fader type 4
    void BuildDefaultInit5(); // 0x17e910  fader type 5
};

// pInit (when non-null) is a CFxMode transition descriptor the caller passes
// through the retail CFader* interface; Add validates its type-id (stored at the
// descriptor's +0) against nFaderType. Read here through the CFader* the retail
// signature carries - a bounded interface-forced reinterpret, not a class miscast.
static inline i32 InitTypeId(CFader* pInit) {
    return *(i32*)pInit;
}

// ===========================================================================
// 0x17d8f0 - CFaderMgr(): construct the embedded element-array subobject (stamp
// its vftable + zero its bookkeeping, inlined) then zero m_active/m_0c. The shared
// timer fields m_timerArgA/m_timerArgB/m_sharedSet2cArg are left for SetConfig. Returns this.
// ===========================================================================
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

// ===========================================================================
// 0x17d980 - SetConfig(a, b, c): latch the shared timer fields (m_timerArgA = a,
// m_timerArgB = b, m_sharedSet2cArg = c) and mark the manager active (m_active = 1). Returns 1.
// __thiscall, three args. Assignment order is load-bearing: timerArgA/timerArgB/sharedSet2cArg
// makes cl hold `b` in edx across the sharedSet2cArg store (retail's schedule); swapping
// sharedSet2cArg ahead of timerArgB holds `c` instead and reorders the two stores.
// ===========================================================================
RVA(0x0017d980, 0x1f)
i32 CFaderMgr::SetConfig(i32 a, i32 b, i32 c) {
    m_timerArgA = a;
    m_timerArgB = b;
    m_sharedSet2cArg = c;
    m_active = 1;
    return 1;
}

// ===========================================================================
// 0x17d9a0 - FreeAll: DeleteAll, then clear m_active.
// ===========================================================================
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
CFader* CFaderMgr::Add(i32 nFaderType, CFader* pInit) {
    CFader* fader = 0;

    switch (nFaderType) {
        case 0: {
            if (pInit && InitTypeId(pInit) != 1) {
                goto wrongclass;
            }
            CFader1816c0* f = new CFader1816c0;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFaderInit init;
                init.BuildDefaultInit0();
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->CopyFrom(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 1: {
            if (pInit && InitTypeId(pInit) != 2) {
                goto wrongclass;
            }
            CFader180410* f = new CFader180410;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFaderInit init;
                init.BuildDefaultInit1();
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->CopyFrom(pInit)) {
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
                CFaderInit init;
                init.BuildDefaultInit2();
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->CopyFrom(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 3: {
            if (pInit && InitTypeId(pInit) != 4) {
                goto wrongclass;
            }
            CFader17f9a0* f = new CFader17f9a0;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFaderInit init;
                init.BuildDefaultInit3();
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->CopyFrom(pInit)) {
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
                CFaderInit init;
                init.BuildDefaultInit4();
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->CopyFrom(pInit)) {
                    goto append;
                }
            }
            goto append;
        }
        case 5: {
            if (pInit && InitTypeId(pInit) != 6) {
                goto wrongclass;
            }
            CFader17e940* f = new CFader17e940;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedSet2cArg);
            if (!pInit) {
                CFaderInit init;
                init.BuildDefaultInit5();
                if (!f->ApplyInit(&init)) {
                    goto badinit;
                }
            } else {
                if (!f->CopyFrom(pInit)) {
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
            m_arr.m_pData = (CFader**)operator new(newSize * 4);
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
            CFader** nd = (CFader**)operator new(newMax * 4);
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
// 0x17e160 - Flush: a thin forwarder that tail-jumps to the engine method
// (0x1b9cde) on the sub-object embedded at this+0x24 (`add ecx,0x24; jmp`).
// Returns the callee's value. The sub-object/callee is external (reloc-masked).
// ===========================================================================
SIZE_UNKNOWN(CFaderTail);
struct CFaderTail {
    i32 Flush(); // 0x1b9cde (__thiscall, 0 args)
};
RVA(0x0017e160, 0x8)
i32 CFaderMgr::Flush() {
    return ((CFaderTail*)&m_sharedSet2cArg)->Flush();
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

// ===========================================================================
// 0x17e1d0 - DeleteAll: delete every fader (scalar-deleting dtor), free the data
// buffer, and zero the array fields.
// ===========================================================================
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
