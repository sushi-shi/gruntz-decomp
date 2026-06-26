// CFxModeT3.cpp - the "type 3" screen-fade effect mesh builder (CFxModeT3,
// tracer ctor 0x17e880; reached from CFaderMgr::Add's case-2 fader). __thiscall,
// one arg (the effect config rect-set). It latches the config into this, sizes
// the source/dest boxes, derives the per-cell step (dx = box->h / m_50, dy =
// box->w / m_54) and the cell radius (sqrt(dx*dx+dy*dy)), then walks an
// (m_54 x m_50) grid emitting one projected mesh point per cell into the growable
// CArray at this+0x58. Each point ellipse-projects the cell center: the radial
// distance v = sqrt((row-halfH)^2 + (col-halfW)^2) drives a normalized (x,y)
// direction, scaled by the cell radius and accumulated via OffsetRect onto two
// sub-rects (pt48 = src cell, pt64 = dst cell), assembled (order gated on m_4c)
// into the 40-byte mesh record. Field names are placeholders; offsets + code
// bytes are load-bearing. The point conversions go through the CRT __ftol; the
// array grow is the inlined MFC CArray::SetAtGrow(GetSize(),&pt) (RezAlloc/RezFree).
#include <Ints.h>

#include <math.h> // sqrt -> fsqrt
#include <rva.h>
#include <string.h> // rep-movs element copy / memset in the array grow

// A box whose +0x18 / +0x1c are its width / height (read by the param derive).
struct FxBox;

// The effect config (arg0): a set of source/dest rect pointers + counts.
struct FxConfig {
    i32 m_00;
    FxBox* m_04; // src box override (else this->m_24)
    FxBox* m_08; // dst box override (else this->m_28)
    i32 m_0c;
    i32 m_10; // gate: 0 -> bail
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
};

// A box whose +0x18 / +0x1c are its width / height (read by the param derive).
struct FxBox {
    char pad[0x18];
    i32 m_18;
    i32 m_1c;
};

// The 40-byte (10-dword) mesh point pushed into the CArray.
struct FxPoint {
    i32 v[10];
};

// The growable point array embedded at this+0x58 (MFC CArray<FxPoint>).
struct FxPointArray {
    void* m_vtbl;     // +0x00
    FxPoint* m_pData; // +0x04
    i32 m_nSize;      // +0x08
    i32 m_nMaxSize;   // +0x0c
    i32 m_nGrowBy;    // +0x10
    void Init(i32 a, i32 b); // 0x17f390 (external, reloc-masked)
};

// The OffsetRect import (reached via the global function pointer at 0x6c4490) and
// the two .rdata float constants the projection compares/biases against.
DATA(0x002c4490)
extern void(__stdcall* g_OffsetRect)(void* r, i32 dx, i32 dy); // PTR_OffsetRect_006c4490
DATA(0x001f07ec)
extern float g_fxBias; // 0x5f07ec
DATA(0x001f07f4)
extern float g_fxEps; // 0x5f07f4

// Rez heap for the array grow (reloc-masked).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

class CFxModeT3 {
public:
    i32 Build(FxConfig* cfg); // 0x17ea00

    char m_00[0x24];
    FxBox* m_24; // +0x24 default src box
    FxBox* m_28; // +0x28 default dst box
    char m_2c[0x3c - 0x2c];
    FxBox* m_3c; // +0x3c active src box
    FxBox* m_38; // +0x38 active dst box
    i32 m_40;    // +0x40
    i32 m_44;    // +0x44
    i32 m_48;    // +0x48
    i32 m_4c;    // +0x4c record-order flag
    i32 m_50;    // +0x50 columns
    i32 m_54;    // +0x54 rows
    FxPointArray m_58; // +0x58 mesh-point array
};

// ===========================================================================
// 0x17ea00 - Build the (m_54 x m_50) ellipse mesh into this->m_58.
// ===========================================================================
// @early-stop
// x87-schedule + inlined-array-grow wall: the algorithm is reconstructed in full
// (config latch, param derive, the radial projection per grid cell, the two
// OffsetRect-accumulated sub-rects, the m_4c-gated 40-byte record, the inlined
// CArray::SetAtGrow push), but MSVC5's exact fild/fsqrt/fdiv/fmul ordering across
// the radius + per-cell projection, the __ftol sequencing, and the open-coded
// MFC array grow (RezAlloc/RezFree + rep-movs) do not reproduce instruction-for-
// instruction from this C spelling, and the dense stack-temp layout diverges.
// Grid topology, the sqrt distance projection and the field assembly are correct;
// the FP/array-grow schedule parks it. Final-sweep candidate.
RVA(0x0017ea00, 0x4fc)
i32 CFxModeT3::Build(FxConfig* cfg) {
    m_3c = cfg->m_04 ? cfg->m_04 : m_24;
    m_38 = cfg->m_08 ? cfg->m_08 : m_28;
    if (cfg->m_10 == 0) {
        return 0;
    }
    m_40 = cfg->m_0c;
    m_44 = cfg->m_10;
    m_48 = cfg->m_18;
    m_4c = cfg->m_14;
    m_50 = cfg->m_1c;
    m_54 = cfg->m_20;

    m_58.Init(0, -1);

    i32 halfW = m_3c->m_18 / 2;
    i32 halfH = m_3c->m_1c / 2;
    i32 dx = m_38->m_1c / m_50;
    i32 dy = m_38->m_18 / m_54;
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
            i32 idx = m_58.m_nSize;
            i32 newSize = idx + 1;
            if (newSize == 0) {
                if (m_58.m_pData) {
                    RezFree(m_58.m_pData);
                    m_58.m_pData = 0;
                }
                m_58.m_nMaxSize = 0;
                m_58.m_nSize = 0;
            } else if (m_58.m_pData == 0) {
                m_58.m_pData = (FxPoint*)RezAlloc(newSize * sizeof(FxPoint));
                memset(m_58.m_pData, 0, newSize * sizeof(FxPoint));
                m_58.m_nMaxSize = newSize;
                m_58.m_nSize = newSize;
            } else if (newSize <= m_58.m_nMaxSize) {
                if (newSize > idx) {
                    memset(&m_58.m_pData[idx], 0, (newSize - idx) * sizeof(FxPoint));
                }
                m_58.m_nSize = newSize;
            } else {
                i32 grow = m_58.m_nGrowBy;
                if (grow == 0) {
                    grow = idx / 8;
                    if (grow < 4) {
                        grow = 4;
                    } else if (grow > 0x400) {
                        grow = 0x400;
                    }
                }
                i32 newMax = m_58.m_nMaxSize + grow;
                if (newSize > newMax) {
                    newMax = newSize;
                }
                FxPoint* nd = (FxPoint*)RezAlloc(newMax * sizeof(FxPoint));
                memcpy(nd, m_58.m_pData, m_58.m_nSize * sizeof(FxPoint));
                memset(&nd[m_58.m_nSize], 0, (newSize - m_58.m_nSize) * sizeof(FxPoint));
                RezFree(m_58.m_pData);
                m_58.m_pData = nd;
                m_58.m_nSize = newSize;
                m_58.m_nMaxSize = newMax;
            }
            m_58.m_pData[idx] = pt;
        }
    }
    return 1;
}
