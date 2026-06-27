// m5_LightEffectSetup.cpp - a shade/lighting effect setup pass (RVA 0x1804a0).
//
// Captures a descriptor's surface/colour parameters into the effect object, clips
// the centre point to the surface rect (early-out if outside), fills the per-scan
// span tables, and resolves the hue-ramp shade table. Field names are
// placeholders; only offsets + code bytes are load-bearing.
#include <rva.h>

#include <Win32.h>

#include <DDrawMgr/ShadeTableCache.h>

// PtInRect reached through a game-owned function pointer (ff 15).
DATA(0x006c456c)
extern BOOL(__stdcall* g_pPtInRect)(const RECT*, POINT);

struct Surf {
    char m_pad00[0x18];
    i32 m_18; // +0x18 height
    i32 m_1c; // +0x1c width
};
struct PalHolder {
    char m_pad00[0xc];
    PalEntry* m_c; // +0x0c palette
};
struct LightDesc {
    char m_pad00[0x4];
    Surf* m_4;       // +0x04
    i32 m_8;         // +0x08
    PalHolder* m_c;  // +0x0c
    i32 m_10;        // +0x10
    i32 m_14;        // +0x14 span count
    i32 m_18;        // +0x18 centre x
    i32 m_1c;        // +0x1c centre y
    i32 m_20;        // +0x20 override table
};

class CLightEffect {
public:
    i32 Setup(LightDesc* d);

    char m_pad00[0x1c];
    void* m_1c; // +0x1c shade table
    i32 m_20;   // +0x20
    Surf* m_24; // +0x24 default surface
    i32 m_28;   // +0x28
    char m_pad2c[0x30 - 0x2c];
    i32 m_30;   // +0x30
    char m_pad34[0x38 - 0x34];
    Surf* m_38; // +0x38 active surface
    i32 m_3c;   // +0x3c
    char m_pad40[0x44 - 0x40];
    PalHolder* m_44; // +0x44
    i32 m_48;        // +0x48
    i32 m_4c;        // +0x4c centre x
    i32 m_50;        // +0x50 centre y
    char m_pad54[0x60 - 0x54];
    i32 m_60[1024];   // +0x60   span starts
    i32 m_1060[1024]; // +0x1060 span ends
    i32 m_2060;       // +0x2060 span count
    i32 m_2064;       // +0x2064 surface width
    i32 m_2068;       // +0x2068 surface height
};

// @early-stop
// 92% - /O2 regalloc entropy tail: the descriptor field loads and the m_3c/m_38
// conditional reuse eax in retail but the recompile distributes them across
// ecx/edx/eax; same instruction selection + scheduling, only the register names
// differ. Logic + externs match retail. Final sweep.
RVA(0x0001804a0, 0x182)
i32 CLightEffect::Setup(LightDesc* d) {
    m_20 = 0;
    Surf* s = d->m_4;
    if (s == 0) {
        s = m_24;
    }
    m_38 = s;
    i32 b = d->m_8;
    if (b == 0) {
        m_3c = m_28;
    } else {
        m_3c = b;
    }
    m_48 = d->m_10;
    m_4c = d->m_18;
    m_50 = d->m_1c;
    PalHolder* pal = d->m_c;
    m_44 = pal;
    i32 cnt = d->m_14;
    m_2060 = cnt;
    if (cnt > 0 && d->m_20 == 0 && pal == 0) {
        return 0;
    }
    if (m_38 == 0) {
        return 0;
    }
    if (m_3c == 0 && m_48 == 0) {
        return 0;
    }
    RECT rect;
    rect.right = m_38->m_1c;
    m_2064 = rect.right;
    rect.bottom = m_38->m_18;
    m_2068 = rect.bottom;
    rect.left = 0;
    rect.top = 0;
    POINT pt;
    pt.x = m_4c;
    pt.y = m_50;
    if (g_pPtInRect(&rect, pt) == 0) {
        return 0;
    }
    if (m_48 != 0) {
        i32 i = 0;
        if (m_2068 > 0) {
            do {
                m_60[i] = 0;
                m_1060[i] = m_2064;
                i++;
            } while (i < m_2068);
        }
    } else {
        i32 i = 0;
        if (m_2068 > 0) {
            do {
                m_60[i] = m_4c;
                m_1060[i] = m_4c;
                i++;
            } while (i < m_2068);
        }
    }
    if (m_2060 > 0) {
        if (d->m_20 == 0) {
            m_1c = ((CShadeTableCache*)((char*)this + 4))->HueRampTable(m_44->m_c, m_2060, 0);
            m_30 = 1;
            return 1;
        }
        m_1c = (void*)d->m_20;
    }
    return 1;
}
