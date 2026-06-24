// ShadeTableCache.cpp - the DDrawMgr color/shade lookup-table cache (tracer
// placeholder ClassUnknown_2). A CGruntzMgr member: a polymorphic owner of a
// growable array of 0x10-byte CShadeTable buffers, each holding a 64KB RGB565 (or
// raw-RGB) color-conversion table built from the live screen RGB-format globals.
//
// Methods in ascending retail-RVA order. Field names are placeholders; offsets +
// code bytes are load-bearing. The array grow/element helpers (0x150040 /
// 0x150180 / 0x150190 / 0x1501a0 / 0x1503c0) and operator new/delete are
// external/reloc-masked.
#include <DDrawMgr/ShadeTableCache.h>

#include <rva.h>
#include <string.h> // inlined memcpy (rep movsl) in FindRemove

// The live screen RGB-format shift/mask table at 0x683ea0..0x683eb4 - already
// named by CLightFxRender.cpp / CDDrawShadeBlit.cpp. The builders gate on the
// RGB565 magic state (rUp=10, gUp=5, rDown/gDown/bDown=3). Reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4

// The two foreign vftables stamped by the array ctor/dtor, as DIR32 data.
DATA(0x001efb28)
extern void* g_shadeArrayVtbl; // 0x5efb28 - the element-array base vtable
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4 - the grand-base dtor vtable

// ===========================================================================
// CShadeTableArray - the embedded element-array subobject. Its inline ctor/dtor
// fold into the cache ctor/dtor: stamp the array vtable, zero/free m_pData.
// ===========================================================================
inline CShadeTableArray::CShadeTableArray() {
    m_vtbl = &g_shadeArrayVtbl;
    m_pData = 0;
    m_nGrowBy = 0;
    m_nMaxSize = 0;
    m_nSize = 0;
}

inline CShadeTableArray::~CShadeTableArray() {
    m_vtbl = &g_shadeArrayVtbl;
    if (m_pData) {
        operator delete(m_pData);
    }
    m_vtbl = &g_remusBaseDtorVtbl;
}

// ===========================================================================
// 0x14de30 - ctor: array subobject ctor (stamp vtable, zero fields), then the
// leading gate.
// ===========================================================================
RVA(0x0014de30, 0x1a)
CShadeTableCache::CShadeTableCache() {
    m_00 = 0;
}

// ===========================================================================
// 0x14de50 - ~ : FreeNodes, then the inline array-subobject teardown. EH frame.
// ===========================================================================
RVA(0x0014de50, 0x6b)
CShadeTableCache::~CShadeTableCache() {
    if (m_00) {
        FreeNodes();
    }
}

// ===========================================================================
// 0x14dec0 - Init: mark the gate and report success.
// ===========================================================================
RVA(0x0014dec0, 0xc)
i32 CShadeTableCache::Init() {
    m_00 = 1;
    return 1;
}

// ===========================================================================
// 0x14ded0 - FreeNodes: destroy + free every element, then drop the array.
// ===========================================================================
RVA(0x0014ded0, 0x64)
void CShadeTableCache::FreeNodes() {
    for (i32 i = 0; i < m_arr.m_nSize; i++) {
        m_arr.m_pData[i]->Destroy();
        CShadeTable* t = m_arr.m_pData[i];
        if (t) {
            t->Free();
            operator delete(t);
        }
    }
    if (m_arr.m_pData) {
        operator delete(m_arr.m_pData);
        m_arr.m_pData = 0;
    }
    m_arr.m_nMaxSize = 0;
    m_arr.m_nSize = 0;
}

// ===========================================================================
// 0x14eef0 - GreyTable: allocate a 0x20000-byte (64K-entry u16) identity remap.
// Two loop variants gated on the RGB565 screen format. EH frame (documented wall:
// the RezAlloc + external element ctor emits no /GX frame on MSVC5). @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall: retail's `new`+throwing-element-ctor carries a /GX frame this
// recompile can't emit (element ctor is external ClassUnknown_4_150180); body
// byte-exact, frame+epilogue cascade absent. See rezalloc-placement-new-no-eh-frame.md
RVA(0x0014eef0, 0x183)
CShadeTable* CShadeTableCache::GreyTable() {
    CShadeTable* t = (CShadeTable*)operator new(0x10);
    if (t) {
        t = t->Ctor();
    } else {
        t = 0;
    }
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x20000, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u16* out = (u16*)t->m_data;
    if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
        for (i32 v = 0; v < 0x10000; v++) {
            *out++ = (u16)(((((u8)(v >> 0xb) << 4) + ((v >> 6) & 0xf)) << 4) + ((v >> 1) & 0xf));
        }
    } else {
        for (i32 v = 0; v < 0x10000; v++) {
            *out++ = (u16)(((((u8)(v >> 0xc) << 4) + ((v >> 7) & 0xf)) << 4) + ((v >> 1) & 0xf));
        }
    }
    return t;
}

// ===========================================================================
// 0x14f5b0 - AlphaTable: allocate a 0x200-byte (256-entry u16) table and fill it
// with the RGB565 conversion of a 256-color RGBA palette (arg). EH frame.
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md): body byte-exact, the
// /GX ctor-in-flight frame is absent on MSVC5; ~66%.
RVA(0x0014f5b0, 0x10a)
CShadeTable* CShadeTableCache::AlphaTable(u8* pal) {
    CShadeTable* t = (CShadeTable*)operator new(0x10);
    if (t) {
        t = t->Ctor();
    } else {
        t = 0;
    }
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x200, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u16* out = (u16*)t->m_data;
    u8* p = pal;
    for (i32 i = 0x100; i != 0; i--) {
        u16 v = (u16)(((u8)(p[0] >> (u8)g_rDown) << g_rUp) | ((u8)(p[1] >> (u8)g_gDown) << g_gUp)
                      | (u8)(p[2] >> (u8)g_bDown));
        *out++ = v;
        p += 4;
    }
    return t;
}

// ===========================================================================
// 0x14fb80 - FindRemove: locate the table whose +0 matches the key, destroy it,
// and memmove the tail down one slot. __thiscall, one arg.
// ===========================================================================
RVA(0x0014fb80, 0x68)
void CShadeTableCache::FindRemove(CShadeTable* key) {
    i32 n = m_arr.m_nSize;
    i32 i = 0;
    if (n > 0) {
        CShadeTable** w = m_arr.m_pData;
        for (;;) {
            if (*w == key) {
                break;
            }
            i++;
            w++;
            if (i >= n) {
                return;
            }
        }
        m_arr.m_pData[i]->Destroy();
        CShadeTable* t = m_arr.m_pData[i];
        if (t) {
            t->Free();
            operator delete(t);
        }
        i32 cnt = m_arr.m_nSize - i - 1;
        CShadeTable** dst = &m_arr.m_pData[i];
        if (cnt) {
            CShadeTable** src = &m_arr.m_pData[i + 1];
            memcpy(dst, src, cnt * sizeof(CShadeTable*));
        }
        m_arr.m_nSize--;
    }
}
