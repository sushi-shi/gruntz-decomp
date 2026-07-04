// CPlayPlaneScan.cpp - two CPlay per-frame plane-list sub-steps re-homed from
// src/Stub/Discovered.cpp (trace-attributed to CPlay: this->m_view at +0xc is the
// CView, m_view->m_renderer the renderer that owns the plane list, m_view->m_drawSurf the draw
// surface). Both walk the renderer's embedded plane list (a CObList-style
// {pNext,pPrev,data} node chain rooted at renderer+0x14) and dispatch on the
// plane descriptor's type field (m_desc->m_typeId, a reloc-masked fn-ptr compare).
//
// Self-contained (own class views) so the shared, matched CPlay.h stays
// untouched. /GX EH frame: CPlay_0d53d0 has a stack CString error temp, and
// CPlay_0d9290 has a stack CByteArray shuffle temp - both destructible.
#include <Ints.h>
#include <Gruntz/CPlay.h> // canonical CPlay (one shape)
#include <Gruntz/CGameRegistry.h>
#include <rva.h>
#include <Gruntz/CString.h>

// ---------------------------------------------------------------------------
// Shared plane / renderer / draw-surface layout the two scans walk.
// ---------------------------------------------------------------------------
struct Vec3 {
    i32 a, b, c;
};
struct Vec4 {
    i32 a, b, c, d;
};

// The plane descriptor at plane->m_desc: +0x10 is the type discriminator (a
// function pointer compared against the known plane-type methods, reloc-masked);
// +0xf0/+0x100 are two by-value 4-vectors the covered-tile draw passes.
struct PlaneDesc {
    char pad0[0x10];
    void (*m_typeId)(); // +0x10 type-id fn ptr (compared against the plane-type fns)
    char pad14[0xf0 - 0x14];
    Vec4 m_f0;  // +0xf0
    Vec4 m_100; // +0x100
};

// A plane object (the CObList data ptr).
struct Plane {
    char pad0[4];
    i32 m_4;     // +0x04
    i32 m_flags; // +0x08 flags (|= 0x10000 when consumed)
    char pad0c[0x5c - 0xc];
    i32 m_x;       // +0x5c  x
    i32 m_y;       // +0x60  y
    Vec4 m_blockC; // +0x64  by-value block C
    char pad74[0x7c - 0x74];
    PlaneDesc* m_desc; // +0x7c descriptor
    char pad80[0x114 - 0x80];
    i32 m_114; // +0x114
    i32 m_118; // +0x118
    i32 m_11c; // +0x11c (== 0x32 -> extra insert)
    char pad120[0x124 - 0x120];
    i32 m_quadIndex; // +0x124
    char pad128[0x130 - 0x128];
    i32 m_130;     // +0x130
    Vec4 m_blockF; // +0x134 block F
    Vec4 m_blockE; // +0x144 block E
    Vec4 m_blockD; // +0x154 block D
    i32 m_164;     // +0x164
    i32 m_168;     // +0x168
};

struct PlaneNode {
    PlaneNode* next; // +0x00
    PlaneNode* prev; // +0x04
    Plane* obj;      // +0x08
};

struct PlaneList {
    char m_pad0[4];  // +0x00  (list-head guard word)
    PlaneNode* head; // +0x04
};

// A tile object reached through the covered-tile grid; slot 8 (+0x20) is the
// per-cell query the covered draw passes its result to.
struct TileObj {
    virtual void s0();
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual i32 Query(i32 subX, i32 subY); // slot 8 (+0x20)
};

// The grid geometry at drawSurf->m_grid.
struct GridGeom {
    char pad0[0x20];
    i32* m_cellTable; // +0x20 cell table
    i32* m_rowTable;  // +0x24 row table
    char pad28[0x30 - 0x28];
    i32 m_tileCountX; // +0x30 tile count X
    i32 m_tileCountY; // +0x34 tile count Y
    char pad38[0x8c - 0x38];
    i32 m_shiftX; // +0x8c shift X
    i32 m_shiftY; // +0x90 shift Y
};

// The draw surface at m_view->m_drawSurf.
struct DrawSurf {
    char pad0[0x4c];
    TileObj** m_objTable; // +0x4c object table
    char pad50[0x5c - 0x50];
    GridGeom* m_grid; // +0x5c grid geometry
};

// The plane list the scans walk is the SAME placed-object display list the warlord
// loader walks (shared CRenderer::m_10 CWarlordListHead, at rendererA+0x10); this TU
// reaches it through the canonical CView (m_c->m_rendererA / m_c->m_drawSurface).
// The draw surface's +0x5c tile-grid facet (GridGeom) differs from the shared
// CDrawSurface's +0x5c camera facet, so the draw-surface facet is reached by cast.

// The per-level sinks the scans feed. m_recordSink receives the rebuilt tile/plane
// records (AddToList1 / the big covered-tile draw); m_ptrSink receives the extra
// pointer insert when m_11c == 0x32. External/reloc-masked.
struct ObjSink2e4 {
    // 0x116cf0 (ILT 0x38c8): thiscall, 7 args, callee-clean.
    i32 AddToList1(i32 a164, i32 a168, i32 a4, i32* buf, i32 a11c, i32 a118, i32 a130);
    // 0x116610 (ILT 0x10ff): the big covered-tile draw; thiscall, callee-clean.
    i32 BigDraw(
        i32 tile,
        i32 kind,
        i32 a164,
        i32 a168,
        i32 a4,
        Vec4 blockF,
        Vec4 blockE,
        Vec4 blockD,
        Vec4 blockC,
        Vec4 descF0,
        Vec4 desc100,
        i32 a124,
        i32 a11c,
        i32 a118,
        i32 a130
    );
};
struct ObjSink2dc {
    void InsertPtr(i32 a118, i32 a114); // 0x108410 (ILT 0x1d2f) thiscall
};

// The global game-manager singleton (*g_64556c) whose ReportError the error
// paths call, and the error-formatting free helper.
extern "C" {
    DATA(0x0024556c)
    extern CGameRegistry* g_64556c;
}
// __cdecl error formatter: fills a CString from a printf-style template.
void PlaneErrFmt(CString* dst, const char* fmt, i32 x, i32 y); // 0x1b2cf5

// The two plane-type discriminators the scans compare m_desc->m_typeId against
// (reloc-masked address operands; identities are irrelevant to the byte match).
extern "C" {
    void PlaneType_Rock();    // 0x40137a
    void PlaneType_Covered(); // 0x403d0f
}

// A byte array shuffled by ScanShuffleQuads (MFC CByteArray shape). ctor/dtor and
// the grow/remove ops are external (reloc-masked); GetAt/GetSize inline.
struct CByteArrayV {
    u8* m_pData;                 // +0x00
    i32 m_nSize;                 // +0x04
    i32 m_nMaxSize;              // +0x08
    i32 m_nGrowBy;               // +0x0c
    CByteArrayV();               // 0x1b527e
    ~CByteArrayV();              // 0x1b52b1
    void SetAtGrow(i32 i, u8 v); // 0x1b5485
    void RemoveAt(i32 i, i32 n); // 0x1b5525
    i32 GetSize() {
        return m_nSize;
    }
    u8 GetAt(i32 i) {
        return m_pData[i];
    }
};

// The six plane-type discriminators ScanShuffleQuads compares against.
extern "C" {
    void PlaneQuadA(); // 0x4017e4
    void PlaneQuadB(); // 0x40192e
    void PlaneQuadC(); // 0x403148
    void PlaneQuadD(); // 0x401087
    void PlaneQuadE(); // 0x40164f
    void PlaneQuadF(); // 0x4019bf (the 4-corner permute type)
}

// CPlay's plane-scan sub-objects live on the canonical CState/CPlay members,
// reached through this TU's local facet views: m_c (+0x0c) is the CView the plane
// list hangs off; the guts sink at +0x2dc (m_guts) receives the extra pointer
// insert, the begin-marker sink at +0x2e4 (m_beginMarker) the rebuilt records.

// ---------------------------------------------------------------------------
// ScanBuildTiles (0x0d53d0): walk the renderer plane list; for each rock plane
// rebuild its record onto the record sink (m_beginMarker->AddToList1), for each
// covered-tile plane sample the tile grid then feed the big draw. Reports "Bad
// rock"/"Bad covered powerup"
// (and bails) on a failed insert.
// ---------------------------------------------------------------------------
// @early-stop
// scheduling wall (~99.6%, topic:scheduling): only residual is the 9-dword rock
// buf fill (llvm-objdump -dr base vs target) - retail emits strict load;store
// pairs (mov [m134],edx / mov edx,[m138] / ...) while cl schedules the m_138/m_13c
// loads 2-ahead of the first buf store. An MSVC5 load-look-ahead coin-flip; the
// struct-copy and array spellings both still hoist. All logic/relocs byte-match.
RVA(0x000d53d0, 0x466)
i32 CPlay::ScanBuildTiles() {
    CView* v = m_c;
    PlaneList* pl = (PlaneList*)&v->m_rendererA->m_10;
    if (pl == 0) {
        return 0;
    }
    PlaneNode* pos = pl->head;
    while (pos != 0) {
        PlaneNode* node = pos;
        pos = node->next;
        Plane* p = node->obj;
        if (p == 0) {
            continue;
        }
        if (p->m_blockF.a == (i32)0x80000000) {
            p->m_blockF.a = 0;
        }
        if (p->m_blockE.a == (i32)0x80000000) {
            p->m_blockE.a = 0;
        }
        if (p->m_blockD.a == (i32)0x80000000) {
            p->m_blockD.a = 0;
        }
        if (p->m_blockC.a == (i32)0x80000000) {
            p->m_blockC.a = 0;
        }
        void (*vf)() = p->m_desc->m_typeId;
        if (vf == PlaneType_Rock) {
            struct {
                Vec3 v0, v1, v2;
            } buf;
            buf.v0 = *(Vec3*)&p->m_blockF;
            buf.v1 = *(Vec3*)&p->m_blockE;
            buf.v2 = *(Vec3*)&p->m_blockD;
            if (((ObjSink2e4*)m_beginMarker)
                    ->AddToList1(
                        p->m_164,
                        p->m_168,
                        p->m_4,
                        (i32*)&buf,
                        p->m_11c,
                        p->m_118,
                        p->m_130
                    )
                == 0) {
                CString s;
                PlaneErrFmt(&s, "Bad rock at: x=%d, y=%d", p->m_x, p->m_y);
                g_64556c->ReportError(s);
                return 0;
            }
            if (p->m_11c == 0x32) {
                ((ObjSink2dc*)m_guts)->InsertPtr(p->m_118, p->m_114);
            }
            p->m_flags |= 0x10000;
        } else if (vf == PlaneType_Covered) {
            DrawSurf* ds = (DrawSurf*)(v->m_drawSurface);
            i32 x = p->m_x;
            i32 y = p->m_y;
            if (x < 0) {
                x = 0;
            } else {
                i32 lim = ds->m_grid->m_tileCountX;
                if (x >= lim) {
                    x = lim - 1;
                }
            }
            if (y < 0) {
                y = 0;
            } else {
                i32 lim = ds->m_grid->m_tileCountY;
                if (y >= lim) {
                    y = lim - 1;
                }
            }
            GridGeom* g = ds->m_grid;
            i32 shX = g->m_shiftX;
            i32 tileX = x >> shX;
            i32 shY = g->m_shiftY;
            i32 tileY = y >> shY;
            i32 subX = x - (tileX << shX);
            i32 subY = y - (tileY << shY);
            i32 cell = g->m_cellTable[g->m_rowTable[tileY] + tileX];
            i32 tile;
            if (cell == (i32)0xeeeeeeee || cell == (i32)0xffffffff) {
                tile = 0;
            } else {
                tile = ds->m_objTable[cell & 0xffff]->Query(subX, subY);
            }
            if (((ObjSink2e4*)m_beginMarker)
                    ->BigDraw(
                        tile,
                        0x1a,
                        p->m_164,
                        p->m_168,
                        p->m_4,
                        p->m_blockF,
                        p->m_blockE,
                        p->m_blockD,
                        p->m_blockC,
                        p->m_desc->m_f0,
                        p->m_desc->m_100,
                        p->m_quadIndex,
                        p->m_11c,
                        p->m_118,
                        p->m_130
                    )
                == 0) {
                CString s;
                PlaneErrFmt(&s, "Bad covered powerup at: x=%d, y=%d", p->m_x, p->m_y);
                g_64556c->ReportError(s);
                return 0;
            }
            if (p->m_11c == 0x32) {
                ((ObjSink2dc*)m_guts)->InsertPtr(p->m_118, p->m_114);
            }
            p->m_flags |= 0x10000;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ScanShuffleQuads (0x0d9290): build a 4-element index array [0,1,2,3], Fisher-
// Yates it with rand() into a random permutation, then walk the renderer plane
// list applying the permutation - remapping m_quadIndex for the five ordinary quad
// types and scatter-permuting the four m_blockF corners for the special type.
// ---------------------------------------------------------------------------
// @early-stop
// CByteArray Fisher-Yates shuffle-idiom wall (~77.7%, topic:codegen-idiom): the
// guard + the whole plane-walk (5 quad types -> perm[m_quadIndex]; the special type's
// NAN-fixup + 4-corner scatter-permute) byte-match. The residual is the shuffle
// prologue - retail's `rand() % n` per round carries a defensive n==0 branchless
// fallback (`lea esi,[n-1]; lea edi,[esi+1]; cmp edi,0; jne idiv; else ~-(rand&1)
// & (n-1)`) that a plain `rand() % arr.GetSize()` does not emit, plus the exact
// MFC CByteArray field offsets the shuffle indexes. Complete algorithm; the exact
// RNG-helper idiom is deferred to the final sweep.
RVA(0x000d9290, 0x2a7)
i32 CPlay::ScanShuffleQuads() {
    CView* v = m_c;
    PlaneList* pl = (PlaneList*)&v->m_rendererA->m_10;
    if (pl == 0) {
        return 0;
    }
    PlaneNode* pos = pl->head;

    i32 perm[4];
    CByteArrayV arr;
    arr.SetAtGrow(arr.GetSize(), 0);
    arr.SetAtGrow(arr.GetSize(), 1);
    arr.SetAtGrow(arr.GetSize(), 2);
    arr.SetAtGrow(arr.GetSize(), 3);
    i32 r;
    r = rand() % arr.GetSize();
    perm[0] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    r = rand() % arr.GetSize();
    perm[1] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    r = rand() % arr.GetSize();
    perm[2] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    perm[3] = arr.GetAt(0);
    arr.RemoveAt(0, 1);

    while (pos != 0) {
        PlaneNode* node = pos;
        pos = node->next;
        Plane* p = node->obj;
        if (p == 0) {
            continue;
        }
        void (*vf)() = p->m_desc->m_typeId;
        if (vf == PlaneQuadA || vf == PlaneQuadB || vf == PlaneQuadC || vf == PlaneQuadD
            || vf == PlaneQuadE) {
            p->m_quadIndex = perm[p->m_quadIndex];
        } else if (vf == PlaneQuadF) {
            if (p->m_blockF.a == (i32)0x80000000) {
                p->m_blockF.a = 0;
            }
            if (p->m_blockE.a == (i32)0x80000000) {
                p->m_blockE.a = 0;
            }
            if (p->m_blockD.a == (i32)0x80000000) {
                p->m_blockD.a = 0;
            }
            if (p->m_blockC.a == (i32)0x80000000) {
                p->m_blockC.a = 0;
            }
            i32 scatter[4];
            scatter[perm[0]] = p->m_blockF.a;
            scatter[perm[1]] = p->m_blockF.b;
            scatter[perm[2]] = p->m_blockF.c;
            scatter[perm[3]] = p->m_blockF.d;
            p->m_blockF.a = scatter[0];
            p->m_blockF.b = scatter[1];
            p->m_blockF.c = scatter[2];
            p->m_blockF.d = scatter[3];
        }
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CByteArrayV);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(DrawSurf);
SIZE_UNKNOWN(GridGeom);
SIZE_UNKNOWN(ObjSink2dc);
SIZE_UNKNOWN(ObjSink2e4);
SIZE_UNKNOWN(Plane);
SIZE_UNKNOWN(PlaneDesc);
SIZE_UNKNOWN(PlaneList);
SIZE_UNKNOWN(PlaneNode);
SIZE_UNKNOWN(TileObj);
SIZE_UNKNOWN(Vec3);
SIZE_UNKNOWN(Vec4);
