// DDrawWorkers.cpp - the CDDrawWorkerA/CDDrawWorkerB frame-set virtuals (vtable
// slots 11/12/13). Each stashes the requested frame + marks the worker armed
// (m_74 = 2), then forwards to the inherited reset/arm helper (CDDrawWorkerBase::
// Helper_164790, 0x164790) or the own named-object frame fetch
// (CDDrawWorkerB::Helper_166040, 0x166040). The worker hierarchy lives in the
// shared <DDrawMgr/DDrawWorkerNode.h>; calling the (inherited/own) helper directly
// replaces the former (HelperHost*)this cross-cast.
//
// Plain /O2 /MT leaves (no SEH frame). Field names are placeholders; offsets +
// code bytes are load-bearing.
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawSurfacePair.h> // PlotMarker's two targets (m_surface @+0x2c)
#include <DDrawMgr/DDSurface.h>        // CDDSurface Lock/m_b0/m_pitch/m_8
#include <Win32.h>                     // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                     // IDirectDrawSurface::Unlock (m_8 dispatch)

#include <Ints.h>
#include <rva.h>

// The frame-source passed as Vfunc30's a3: an int array @+0x14 indexed by a4,
// bounded by [m_lowerBound, m_upperBound].
struct CDDrawFrameSource {
    char _pad00[0x14];
    i32* m_frameTable; // +0x14  frame table
    char _pad18[0x64 - 0x18];
    i32 m_lowerBound; // +0x64  lower bound
    i32 m_upperBound; // +0x68  upper bound
};
SIZE_UNKNOWN(CDDrawFrameSource);

// ---------------------------------------------------------------------------
// CDDrawWorkerA::~CDDrawWorkerA (0x1570d0; its ??_G scalar-deleting wrapper is 0x1570b0,
// which calls this then conditionally frees). The non-deleting destructor poisons the
// worker's timing/marker fields to their sentinels (the m_20/m_38 0x80000000/-1 timer
// pair, reset THREE times; m_5c; the byte marker m_78) and nulls the header
// (m_04/m_08/m_ctx). cl stamps the CObject grand-base vtable (g_wapObjectDtorVtbl
// @0x5e8cb4) at the tail via the implicit base-subobject teardown - no manual store.
// The redundant m_20/m_38 resets go through volatile lvalues so cl keeps all three
// (retail does not DCE them). (Re-homed from src/Stub/BoundaryUpper2.cpp; the
// CDDWorkerFlatA volatile-view is dissolved onto the real CDDrawWorkerA - clean-room
// mandate: model the class, not the view. The non-polymorphic flat view scored 83.67%
// only by dropping the base stamp; the real polymorphic destructor is the correct shape.)
// @early-stop
// constant-materialization scheduling wall: verified byte-faithful via llvm-objdump -dr -
// EVERY store + the tail vptr stamp match (the stamp reloc is IMAGE_REL_I386_DIR32
// ??_7CObject@@6B@ on BOTH sides). The lone diff is a 2-instruction swap: retail hoists
// `or eax,-1` (the -1 timer sentinel) ahead of the `mov byte [+0x78],0` store; cl schedules
// the byte store first. Not source-steerable (both constants are materialized up front by
// retail; cl defers eax until its first use at m_38). Logic + bytes complete.
RVA(0x001570d0, 0x39)
CDDrawWorkerA::~CDDrawWorkerA() {
    volatile i32* pHi = &m_20;
    volatile i32* pLo = &m_38;
    m_78 = 0;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_5c = (i32)0x80000000;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_04 = -1;
    m_08 = 0;
    m_ctx = 0;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerB::~CDDrawWorkerB (0x157240; ??_G wrapper 0x157220). Mirror of
// ~CDDrawWorkerA - the int-frame worker's m_78 is a DWORD here (byte in A). Same
// volatile-pinned redundant m_20/m_38 pair + the tail ??_7CObject@@6B@ base stamp.
// @early-stop
// constant-materialization scheduling wall: same as ~CDDrawWorkerA - byte-faithful modulo
// the `or eax,-1` / `mov [+0x78],0` 2-instruction swap; the base vptr stamp reloc matches.
RVA(0x00157240, 0x3c)
CDDrawWorkerB::~CDDrawWorkerB() {
    volatile i32* pHi = &m_20;
    volatile i32* pLo = &m_38;
    m_78 = 0;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_5c = (i32)0x80000000;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_04 = -1;
    m_08 = 0;
    m_ctx = 0;
}

// ---------------------------------------------------------------------------
RVA(0x00157110, 0x20)
i32 CDDrawWorkerA::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = (char)a3;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
RVA(0x001572f0, 0x20)
i32 CDDrawWorkerB::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = a3;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
RVA(0x00157280, 0x30)
i32 CDDrawWorkerB::Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4) {
    Helper_166040(a3, a4);
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
// 0x1572b0: store frame `src->m_frameTable[a4]` (0 if a4 out of [m_lowerBound,
// m_upperBound]) into m_78, set m_74=2, then forward (a1,a2) to Helper_164790.
RVA(0x001572b0, 0x38)
i32 CDDrawWorkerB::Vfunc30(i32 a1, i32 a2, CDDrawFrameSource* src, i32 a4) {
    i32 frame;
    if (a4 >= src->m_lowerBound && a4 <= src->m_upperBound) {
        frame = src->m_frameTable[a4];
    } else {
        frame = 0;
    }
    m_78 = frame;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// ---------------------------------------------------------------------------
// 0x165fa0 (vtable slot 10): plot the worker's marker pixel (m_78) at pixel
// (m_5c, m_60) onto BOTH passed surface pairs - the back one (b) first, then the
// front (a). Each: lock the held surface, write the byte at m_b0*x + pitch*y, unlock.
// __thiscall, 2 ptr args (ret 0x8).
// @early-stop
// ~89% spill-slot regalloc wall. The second block (front pair `a`) is byte-exact;
// the first block (`b`) differs only in WHICH coordinate is spilled across the Lock
// call: retail reserves a dedicated local (push ecx) and spills x (m_5c), keeping y
// in ebp; our cl reuses an arg home slot and spills y (m_60), keeping x in ebp - the
// two products then land in swapped registers. Same math, same stores; the two
// independent field loads are scheduler-reordered regardless of source order (the
// permuter confirmed no source spelling closes it). docs/patterns/zero-register-pinning.md.
RVA(0x00165fa0, 0x93)
void CDDrawWorkerA::PlotMarker_165fa0(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    {
        i32 x = m_5c;
        char c = m_78;
        CDDSurface* s = b->m_surface;
        i32 y = m_60;
        char* base = (char*)s->Lock(0);
        if (base != 0) {
            base[s->m_b0 * x + s->m_pitch * y] = c;
            s->m_8->Unlock(0);
        }
    }
    {
        char c = m_78;
        i32 y = m_60;
        i32 x = m_5c;
        CDDSurface* s = a->m_surface;
        char* base = (char*)s->Lock(0);
        if (base != 0) {
            base[s->m_b0 * x + y * s->m_pitch] = c;
            s->m_8->Unlock(0);
        }
    }
}
