#include <rva.h>
// DDrawWorker.cpp - the owned-collection node of the DDrawMgr "DDraw worker"
// family (placeholder name; engine "tomalla-35"). Non-RTTI engine class;
// vtable @0x5efbe8 (g_ddrawWorkerVtbl), grand-base dtor vtable
// g_wapObjectDtorVtbl @0x5e8cb4. See include/DDrawMgr/DDrawWorker.h.
//
// Two methods (retail-RVA order):
//   0x151eb0  DeleteAll        (delete every owned element, RemoveAll, seed sentinels)
//   0x1557a0  ~CDDrawWorker (stamp own vtbl, DeleteAll, ~CObArray member, base)
//
// The CLoadable-shaped base subobject + the destructible CObArray member give
// the dtor its /GX EH frame (cf. CWwdGrid::~CWwdGrid @0x1682a0).
#include <Mfc.h> // /GX EH-frame helpers

#include <stdlib.h> // atoi (frame-index parse)

#include <Bute/SymTab.h>              // CSymTab iteration (FirstSym/NextSym{,2,3})
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_0c owner (m_flags bit 0x100 = single-frame)
#include <DDrawMgr/DDrawWorker.h>
#include <Gruntz/ParseSource.h> // CParseSource value records (m_name/GetEntryTag)

// The class is real-polymorphic: cl emits ??_7CLoadable (grand-base @0x5e8cb4)
// + the derived vtable @0x5efbe8 implicitly. Both former manual externs
// (g_ddrawWorkerVtbl / g_wapObjectDtorVtbl) were dead here and are removed;
// the derived vtable 0x1efbe8 is named by VTBL(CDDrawWorker) in
// CDDrawWorkerRegistry.cpp (same retail class, fuller 17-slot model).

// ===========================================================================
// 0x151eb0 - DeleteAll: delete every owned element via its scalar-deleting dtor
// (vtbl slot 1, arg 1), RemoveAll the array, then seed the +0x64 cached-index
// sentinel (99999) and clear +0x68. Plain /O2 leaf (no EH frame).
// ===========================================================================
RVA(0x00151eb0, 0x43)
void CDDrawWorker::DeleteAll() {
    for (i32 i = 0; i < m_items.m_nSize; i++) {
        CWorkerElement* el = m_items.m_pData[i];
        if (el != 0) {
            el->Delete(1);
        }
    }
    m_items.SetSize(0, -1); // CObArray::RemoveAll (inlined as SetSize(0,-1))
    m_64 = 99999;
    m_68 = 0;
}

// ===========================================================================
// 0x1521c0: store `elem` at frame index `index` (CObArray::SetAtGrow) and widen the
// cached sentinel window [m_64, m_68] to include it. __thiscall, 2 args (ret 8).
// ===========================================================================
RVA(0x001521c0, 0x2b)
void CDDrawWorker::AddFrameAt_1521c0(void* elem, i32 index) {
    m_items.SetAtGrow(index, elem);
    if (index < m_64) {
        m_64 = index;
    }
    if (index > m_68) {
        m_68 = index;
    }
}

// ===========================================================================
// 0x1521f0 (slot 10): build frames from a CSymTab scope. Walk every value record
// of every symbol; parse a frame index out of each value's name (the first digit
// run) and dispatch InsertFrame(rec, index, 1) (slot 14). Count the frames that
// took. When the owner surface-mgr's single-frame flag (m_flags & 0x100) is set,
// stop after the first success. __thiscall(tab), ret 4.
// ===========================================================================
RVA(0x001521f0, 0xbc)
i32 CDDrawWorker::BuildFramesFromSymTab(CSymTab* tab) {
    i32 count = 0;
    void* sym = tab->FirstSym();
    while (sym != 0) {
        void* val = tab->NextSym2(sym);
        while (val != 0) {
            char* p = ((CParseSource*)val)->m_name;
            while (*p != 0) {
                if (*p >= '0' && *p <= '9') {
                    break;
                }
                p++;
            }
            i32 fi = atoi(p);
            if (InsertFrame(val, fi, 1) != 0) {
                count++;
            }
            val = tab->NextSym3(val);
            if ((((CDDrawSurfaceMgr*)m_0c)->m_flags & 0x100) && count > 0) {
                val = 0;
            }
        }
        sym = tab->NextSym(sym);
        if ((((CDDrawSurfaceMgr*)m_0c)->m_flags & 0x100) && count > 0) {
            sym = 0;
        }
    }
    return count;
}

// ===========================================================================
// 0x1522b0 (slot 15): validate that a CSymTab scope's image-type value records
// (tags 'PCX'/'BMP'/'RID'/'PID') each resolve to a frame in the cached window via
// slot 16 (Slot40_1523b0). Returns -1 the moment a resolve fails, or if fewer
// records matched than the count of live frames in [m_64, m_68]; else the match
// count. __thiscall(tab), ret 4.
// ===========================================================================
// @early-stop
// regalloc-coloring wall: body is byte-structure-exact but MSVC colors `this`->ebx
// and coalesces cnt/tab->edi, whereas retail keeps `this`->edi and coalesces
// cnt/tab->ebx (a consistent ebx<->edi swap) plus retail push-saves all 4 GPRs up
// front where cl shrink-wraps them. Every mnemonic/operand-shape matches; only the
// two callee-saved colors differ. permute (start 87.755%) found no better spelling.
RVA(0x001522b0, 0xf7)
i32 CDDrawWorker::ValidateFramesFromSymTab(CSymTab* tab) {
    i32 matched = 0;
    i32 liveFrames;
    liveFrames = 0;
    i32 n = m_items.m_nSize;
    if (n > 0) {
        i32 cnt;
        cnt = 0;
        for (i32 i = 0; i < n; i++) {
            CWorkerElement* el;
            if (i >= m_64 && i <= m_68) {
                el = m_items.m_pData[i];
            } else {
                el = 0;
            }
            if (el != 0) {
                cnt++;
            }
        }
        liveFrames = cnt;
    }
    void* sym = tab->FirstSym();
    while (sym != 0) {
        void* val = tab->NextSym2(sym);
        while (val != 0) {
            i32 tag = ((CParseSource*)val)->GetEntryTag();
            if (tag == 'PCX' || tag == 'BMP' || tag == 'RID' || tag == 'PID') {
                char* p = ((CParseSource*)val)->m_name;
                while (*p != 0) {
                    if (*p >= '0' && *p <= '9') {
                        break;
                    }
                    p++;
                }
                i32 fi = atoi(p);
                if (0 == Slot40_1523b0((i32)val, fi, 1)) {
                    return -1;
                }
                matched++;
            }
            val = tab->NextSym3(val);
        }
        sym = tab->NextSym(sym);
    }
    return (matched >= liveFrames) ? matched : -1;
}

// ===========================================================================
// 0x1523b0 (slot 16): range-query dispatch - if the frame index `n` is within the
// cached sentinel window [m_64, m_68], fetch element m_items[n] and dispatch its
// slot-13 query (rec, flag), returning it as a bool; otherwise 0. __thiscall, ret 0xc.
// ===========================================================================
RVA(0x001523b0, 0x3b)
i32 CDDrawWorker::Slot40_1523b0(i32 rec, i32 n, i32 flag) {
    CWorkerElement* el;
    if (n >= m_64 && n <= m_68) {
        el = m_items.m_pData[n];
    } else {
        el = 0;
    }
    if (el == 0) {
        return 0;
    }
    return el->Query34(rec, flag) != 0;
}

// ===========================================================================
// 0x1557a0 - ~CDDrawWorker: stamp own vtable, run DeleteAll (most-derived
// teardown), then the CObArray member destructs and ~CLoadable folds in
// (resets m_04/m_08/m_0c, restores the grand-base vtable). /GX frame from the
// destructible base+member.
// ===========================================================================
// 100%: re-basing onto the canonical CLoadable : CWapObj : CObject resolved the
// grand-base vptr-stamp-position wall - the real CObject grand-base sinks the 0x5e8cb4
// re-stamp after the m_04/m_08/m_0c resets exactly as retail (was ~95% on the 1-slot
// CLoadable stand-in that stamped the vptr before the field writes).
RVA(0x001557a0, 0x68)
CDDrawWorker::~CDDrawWorker() {
    DeleteAll();
    // m_items.~CWorkerObArray() (trylevel 0) + ~CLoadable() (field resets +
    // grand-base vtable stamp) fold here.
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
// SIZE(CLoadable) now comes from the canonical <Gruntz/Loadable.h>.
SIZE_UNKNOWN(CDDrawWorker);
SIZE_UNKNOWN(CWorkerObArray);
SIZE_UNKNOWN(CWorkerElement);
