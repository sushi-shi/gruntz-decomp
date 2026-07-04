// LightEffectSetup.cpp - a shade/lighting effect setup pass (RVA 0x1804a0).
//
// This is CFaderLight::ApplyInit (0x1804a0), the type-2 CFader subtype's apply
// method (subtype CFaderLight declared in <Gruntz/FaderSubtypes.h>, allocated by
// CFaderMgr::Add). Modeled here as the flat body-view CFaderLightApply because the
// method reads the CFader base-region slots repurposed as surface/palette; fold
// into CFaderLight (: public CFader) is a follow-up matcher.
//
// Captures a descriptor's surface/colour parameters into the effect object, clips
// the centre point to the surface rect (early-out if outside), fills the per-scan
// span tables, and resolves the hue-ramp shade table. Field names are
// placeholders; only offsets + code bytes are load-bearing.
#include <rva.h>

#include <Win32.h>

#include <DDrawMgr/ShadeTableCache.h>

// SIZE annotations for the ShadeTableCache.h classes are hosted here rather than
// in the header: shadetablecache.cpp is a /O2-sensitive TU whose CompareHue/
// GammaTable reschedule under any header-injected typedef (verified), so the
// completeness annotations live in this (neutral) sibling includer instead.
SIZE(CShadeTable, 0x10);      // array-element stride (0x10-byte buffer wrapper)
SIZE(CShadeTableArray, 0x14); // MFC CObArray-shaped subobject (cache 0x18 - 0x04)
SIZE(PalEntry, 0x4);          // 4-byte palette record (256-entry array stride)
SIZE(CShadeTableCache, 0x18); // RE'd heap-alloc size (CGruntzMgr +0x50)

// PtInRect reached through a game-owned function pointer (ff 15).
DATA(0x006c456c)
extern BOOL(WINAPI* g_pPtInRect)(const RECT*, POINT);

struct Surf {
    char m_pad00[0x18];
    i32 m_height; // +0x18 height
    i32 m_width;  // +0x1c width
};
SIZE_UNKNOWN(Surf);
struct PalHolder {
    char m_pad00[0xc];
    PalEntry* m_palette; // +0x0c palette
};
SIZE_UNKNOWN(PalHolder);
struct LightDesc {
    char m_pad00[0x4];
    Surf* m_surface;              // +0x04
    i32 m_8;                      // +0x08
    PalHolder* m_paletteHolder;   // +0x0c
    i32 m_10;                     // +0x10
    i32 m_spanCount;              // +0x14 span count
    i32 m_centerX;                // +0x18 centre x
    i32 m_centerY;                // +0x1c centre y
    CShadeTable* m_overrideTable; // +0x20 override table (pointer)
};
SIZE_UNKNOWN(LightDesc);

class CFaderLightApply {
public:
    i32 Setup(LightDesc* d);

    i32 m_00;                  // +0x00
    CShadeTableCache m_cache;  // +0x04  embedded shade-table cache (0x18 B -> +0x1c)
    CShadeTable* m_shadeTable; // +0x1c shade table
    i32 m_20;                  // +0x20
    Surf* m_defaultSurface;    // +0x24 default surface
    i32 m_28;                  // +0x28
    char m_pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    char m_pad34[0x38 - 0x34];
    Surf* m_activeSurface; // +0x38 active surface
    i32 m_3c;              // +0x3c
    char m_pad40[0x44 - 0x40];
    PalHolder* m_paletteHolder; // +0x44
    i32 m_48;                   // +0x48
    i32 m_centerX;              // +0x4c centre x
    i32 m_centerY;              // +0x50 centre y
    char m_pad54[0x60 - 0x54];
    i32 m_spanStarts[1024]; // +0x60   span starts
    i32 m_spanEnds[1024];   // +0x1060 span ends
    i32 m_spanCount;        // +0x2060 span count
    i32 m_surfaceWidth;     // +0x2064 surface width
    i32 m_surfaceHeight;    // +0x2068 surface height
};
SIZE_UNKNOWN(CFaderLightApply);

// @early-stop
// 92% - /O2 regalloc entropy tail: the descriptor field loads and the m_3c/m_activeSurface
// conditional reuse eax in retail but the recompile distributes them across
// ecx/edx/eax; same instruction selection + scheduling, only the register names
// differ. Logic + externs match retail. Final sweep.
RVA(0x0001804a0, 0x182)
i32 CFaderLightApply::Setup(LightDesc* d) {
    m_20 = 0;
    Surf* s = d->m_surface;
    if (s == 0) {
        s = m_defaultSurface;
    }
    m_activeSurface = s;
    i32 b = d->m_8;
    if (b == 0) {
        m_3c = m_28;
    } else {
        m_3c = b;
    }
    m_48 = d->m_10;
    m_centerX = d->m_centerX;
    m_centerY = d->m_centerY;
    PalHolder* pal = d->m_paletteHolder;
    m_paletteHolder = pal;
    i32 cnt = d->m_spanCount;
    m_spanCount = cnt;
    if (cnt > 0 && d->m_overrideTable == 0 && pal == 0) {
        return 0;
    }
    if (m_activeSurface == 0) {
        return 0;
    }
    if (m_3c == 0 && m_48 == 0) {
        return 0;
    }
    RECT rect;
    rect.right = m_activeSurface->m_width;
    m_surfaceWidth = rect.right;
    rect.bottom = m_activeSurface->m_height;
    m_surfaceHeight = rect.bottom;
    rect.left = 0;
    rect.top = 0;
    POINT pt;
    pt.x = m_centerX;
    pt.y = m_centerY;
    if (g_pPtInRect(&rect, pt) == 0) {
        return 0;
    }
    if (m_48 != 0) {
        i32 i = 0;
        if (m_surfaceHeight > 0) {
            do {
                m_spanStarts[i] = 0;
                m_spanEnds[i] = m_surfaceWidth;
                i++;
            } while (i < m_surfaceHeight);
        }
    } else {
        i32 i = 0;
        if (m_surfaceHeight > 0) {
            do {
                m_spanStarts[i] = m_centerX;
                m_spanEnds[i] = m_centerX;
                i++;
            } while (i < m_surfaceHeight);
        }
    }
    if (m_spanCount > 0) {
        if (d->m_overrideTable == 0) {
            m_shadeTable = m_cache.HueRampTable(m_paletteHolder->m_palette, m_spanCount, 0);
            m_30 = 1;
            return 1;
        }
        m_shadeTable = d->m_overrideTable;
    }
    return 1;
}
