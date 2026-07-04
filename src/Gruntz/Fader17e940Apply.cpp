// Fader17e940Apply.cpp - CFader17e940::ApplyInit (0x17ea00), the type-6
// screen-fader's "apply the transition descriptor" method (the mesh builder).
//
// DE-VIEW NOTE (was mislabeled CFxModeT3::Build): Ghidra RTTI names 0x17ea00
// ?Build@CFxModeT3@@QAEHPAUFxConfig@@@Z, but that attribution is WRONG. This
// method's `this` (ecx) is the 0x6c-byte CFader17e940 (ctor 0x17e940, own vftable
// 0x5f07c0, growable mesh buffer at +0x58), NOT the 0x14-byte CFxModeT3 mode
// descriptor (ctor 0x17e880, fields only 0x00..0x10). Proof: 0x17e880 writes just
// 0x14 bytes, whereas this method reads/writes +0x24/+0x28/+0x3c..+0x54 and inits a
// buffer at +0x58, so 0x58+0x14 == 0x6c == the CFader17e940 allocation size. Its
// only caller is CFaderMgr::Add's case-6 arm (RezAlloc(0x6c) -> ctor 0x17e940 ->
// this method), and the descriptor argument it consumes is the CFxMode-family record
// CFaderMgr builds via CFxModeT6::CFxModeT6 (0x17e910) / CFaderInit::BuildDefaultInit5.
// So the real owner is CFader17e940 (canonical decl in <Gruntz/FaderSubtypes.h>),
// the real method name is ApplyInit, and CFxModeT3 the descriptor stays modeled at
// its true 0x14 size in <Gruntz/FxModeDesc.h>.
//
// It latches the descriptor into this, sizes the source/dest boxes, derives the
// per-cell step (dx = box->h / m_50, dy = box->w / m_54) and the cell radius
// (sqrt(dx*dx+dy*dy)), then walks an (m_54 x m_50) grid emitting one projected mesh
// point per cell into the growable buffer at this+0x58. Each point ellipse-projects
// the cell center: the radial distance v = sqrt((row-halfH)^2 + (col-halfW)^2)
// drives a normalized (x,y) direction, scaled by the cell radius and accumulated via
// OffsetRect onto two sub-rects (pt48 = src cell, pt64 = dst cell), assembled (order
// gated on m_4c) into the 40-byte mesh record. Field names are placeholders; offsets
// + code bytes are load-bearing. The point conversions go through the CRT __ftol; the
// buffer grow is the inlined MFC CArray::SetAtGrow(GetSize(),&pt) (RezAlloc/RezFree).
#include <Ints.h>

#include <math.h> // sqrt -> fsqrt
#include <rva.h>
#include <string.h> // rep-movs element copy / memset in the buffer grow
#include <Win32.h>  // WINAPI (windows.h) for the g_OffsetRect import-pointer type
#include <Globals.h>

#include <Gruntz/FaderSubtypes.h> // the real owner: CFader17e940 (+ CFaderInit fwd)

// A box whose +0x18 / +0x1c are its width / height (read by the param derive). The
// active src/dst box pointers live in CFader17e940::m_3c / m_38 (stored as dwords).
struct FxBox {
    char pad[0x18];
    i32 m_width;  // +0x18
    i32 m_height; // +0x1c
};

// The CFxMode transition descriptor (the ApplyInit argument): the mode record
// CFaderMgr::Add fills (CFxModeT6::CFxModeT6 / BuildDefaultInit5) or passes through.
// CFaderInit models the same bytes as an opaque blob + a trailing CString; this view
// names the int parameter fields ApplyInit reads. +0x00 is the type discriminator.
struct FxTransDesc {
    i32 m_00;
    FxBox* m_srcBox; // +0x04 src box override (else this->m_timerA)
    FxBox* m_dstBox; // +0x08 dst box override (else this->m_timerB)
    i32 m_0c;
    i32 m_10;          // gate: 0 -> bail
    i32 m_recordOrder; // +0x14
    i32 m_18;
    i32 m_columns; // +0x1c
    i32 m_rows;    // +0x20
};

// The 40-byte (10-dword) mesh point pushed into the buffer.
struct FxPoint {
    i32 v[10];
};

// A typed view of the growable buffer sub-object embedded at CFader17e940::m_58
// (the CRezBufferObject / MFC CArray<FxPoint>): vptr + pData/size/max/growby.
struct FxMeshBuffer {
    void* m_vtbl;            // +0x00 (+0x58)
    FxPoint* m_pData;        // +0x04 (+0x5c)
    i32 m_nSize;             // +0x08 (+0x60)
    i32 m_nMaxSize;          // +0x0c (+0x64)
    i32 m_nGrowBy;           // +0x10 (+0x68)
    void Init(i32 a, i32 b); // 0x17f390 (external, reloc-masked)
};

// The OffsetRect import (reached via the global function pointer at 0x6c4490) and
// the two .rdata float constants the projection compares/biases against.
DATA(0x002c4490)
extern void(WINAPI* g_OffsetRect)(void* r, i32 dx, i32 dy); // PTR_OffsetRect_006c4490

// Rez heap for the buffer grow (reloc-masked).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

// ===========================================================================
// 0x17ea00 - CFader17e940::ApplyInit(desc): build the (m_54 x m_50) ellipse mesh
// into this->m_58 from the transition descriptor. Returns 0 on a null gate, else 1.
// ===========================================================================
// @early-stop
// regalloc this-pin + x87-schedule wall (proven with llvm-objdump -dr base vs
// target): the descriptor latch is instruction-for-instruction identical
// (same loads [eax+N], same stores [this+N], same tests/branches, same offsets),
// but retail pins `this` in EBP while cl colours it ESI - a callee-saved recolor
// that changes the ModRM encoding of every this-relative access through the whole
// body (this is the docs/patterns/zero-register-pinning.md family), and the frame
// is 0x9c vs cl's 0x90 (retail reserves 12 B more x87 stack-temps). On top of that
// the radius + per-cell projection is a dense x87 expression whose fild/fsqrt/fdiv/
// fmul + __ftol ordering is not source-steerable (docs/patterns/x87-fp-stack-
// schedule.md, ~60-75% band), and the open-coded MFC buffer grow (RezAlloc/RezFree
// + rep-movs) diverges likewise. Grid topology, the sqrt distance projection and the
// field assembly are all correct; the this-recolor + FP schedule park it (~68.4%,
// up from ~65.6% pre-de-view). Final-sweep candidate. The de-view (CFxModeT3::Build
// -> CFader17e940::ApplyInit) is byte-neutral/positive: only the mangled symbol name
// changes (paired by RVA), not the algorithm.
RVA(0x0017ea00, 0x4fc)
i32 CFader17e940::ApplyInit(CFaderInit* descOpaque) {
    // The arg is the CFxMode transition descriptor; m_58 is this fader's growable
    // mesh buffer. Both are typed views onto the real (opaque) shapes.
    FxTransDesc* cfg = (FxTransDesc*)descOpaque;
    FxMeshBuffer* mesh = (FxMeshBuffer*)&m_58;

    m_3c = cfg->m_srcBox ? (i32)cfg->m_srcBox : m_timerA;
    m_38 = cfg->m_dstBox ? (i32)cfg->m_dstBox : m_timerB;
    if (cfg->m_10 == 0) {
        return 0;
    }
    m_40 = cfg->m_0c;
    m_44 = cfg->m_10;
    m_48 = cfg->m_18;
    m_4c = cfg->m_recordOrder;
    m_50 = cfg->m_columns;
    m_54 = cfg->m_rows;

    mesh->Init(0, -1);

    i32 halfW = ((FxBox*)m_3c)->m_width / 2;
    i32 halfH = ((FxBox*)m_3c)->m_height / 2;
    i32 dx = ((FxBox*)m_38)->m_height / m_50;
    i32 dy = ((FxBox*)m_38)->m_width / m_54;
    float radius = (float)sqrt((double)(dx * dx + dy * dy));
    if (m_54 <= 0) {
        return 1;
    }

    for (i32 row = 0; row < m_54; row++) {
        i32 cellW2 = halfW * halfW;
        i32 cellD = halfW * halfW + halfH * halfH;
        float cellR = (float)sqrt((double)cellD) + radius - g_fxBias;
        if (m_50 <= 0) {
            continue;
        }
        for (i32 col = 0; col < m_50; col++) {
            i32 d2 = halfH * halfH + cellW2;
            float v = (float)sqrt((double)d2);
            float normX, normY;
            if (v > g_fxEps) {
                normY = (float)(row - halfH) / v;
                normX = (float)(col - halfW) / v;
            } else {
                normY = 0.0f;
                normX = 1.0f;
            }

            i32 pt48[4];
            pt48[0] = 0;
            pt48[1] = 0;
            pt48[2] = dx;
            pt48[3] = dy;
            g_OffsetRect(pt48, row, col);
            i32 ox = (i32)(cellR * normX);
            i32 oy = (i32)(cellR * normY);
            g_OffsetRect(pt48, oy, ox);

            i32 pt64[4];
            pt64[0] = 0;
            pt64[1] = 0;
            pt64[2] = d2;
            pt64[3] = dy;
            g_OffsetRect(pt64, row, col);

            FxPoint pt;
            if (m_4c) {
                pt.v[0] = pt64[2];
                pt.v[1] = pt64[3];
                pt.v[2] = pt64[0];
                pt.v[3] = pt64[1];
                pt.v[4] = pt48[0];
                pt.v[5] = pt48[1];
                pt.v[6] = pt48[2];
                pt.v[7] = pt48[3];
            } else {
                pt.v[0] = pt48[0];
                pt.v[1] = pt48[1];
                pt.v[2] = pt48[2];
                pt.v[3] = pt48[3];
                pt.v[4] = pt64[2];
                pt.v[5] = pt64[3];
                pt.v[6] = pt64[0];
                pt.v[7] = pt64[1];
            }
            pt.v[8] = 0;
            pt.v[9] = 0x3f800000;

            // Inlined MFC CArray::SetAtGrow(GetSize(), &pt).
            i32 idx = mesh->m_nSize;
            i32 newSize = idx + 1;
            if (newSize == 0) {
                if (mesh->m_pData) {
                    RezFree(mesh->m_pData);
                    mesh->m_pData = 0;
                }
                mesh->m_nMaxSize = 0;
                mesh->m_nSize = 0;
            } else if (mesh->m_pData == 0) {
                mesh->m_pData = (FxPoint*)RezAlloc(newSize * sizeof(FxPoint));
                memset(mesh->m_pData, 0, newSize * sizeof(FxPoint));
                mesh->m_nMaxSize = newSize;
                mesh->m_nSize = newSize;
            } else if (newSize <= mesh->m_nMaxSize) {
                if (newSize > idx) {
                    memset(&mesh->m_pData[idx], 0, (newSize - idx) * sizeof(FxPoint));
                }
                mesh->m_nSize = newSize;
            } else {
                i32 grow = mesh->m_nGrowBy;
                if (grow == 0) {
                    grow = idx / 8;
                    if (grow < 4) {
                        grow = 4;
                    } else if (grow > 0x400) {
                        grow = 0x400;
                    }
                }
                i32 newMax = mesh->m_nMaxSize + grow;
                if (newSize > newMax) {
                    newMax = newSize;
                }
                FxPoint* nd = (FxPoint*)RezAlloc(newMax * sizeof(FxPoint));
                memcpy(nd, mesh->m_pData, mesh->m_nSize * sizeof(FxPoint));
                memset(&nd[mesh->m_nSize], 0, (newSize - mesh->m_nSize) * sizeof(FxPoint));
                RezFree(mesh->m_pData);
                mesh->m_pData = nd;
                mesh->m_nSize = newSize;
                mesh->m_nMaxSize = newMax;
            }
            mesh->m_pData[idx] = pt;
        }
    }
    return 1;
}

SIZE_UNKNOWN(FxBox);
SIZE_UNKNOWN(FxTransDesc);
SIZE_UNKNOWN(FxPoint);
SIZE_UNKNOWN(FxMeshBuffer);
