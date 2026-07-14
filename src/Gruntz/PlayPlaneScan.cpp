// PlayPlaneScan.cpp - two CPlay per-frame plane-list sub-steps re-homed from
// src/Stub/Discovered.cpp (trace-attributed to CPlay: this->m_view at +0xc is the
// CSpriteFactoryHolder, m_view->m_renderer the renderer that owns the plane list, m_view->m_drawSurf the draw
// surface). Both walk the renderer's embedded plane list (a CPtrList-style
// {pNext,pPrev,data} node chain rooted at renderer+0x14) and dispatch on the
// plane descriptor's type field (m_desc->m_typeId, a reloc-masked fn-ptr compare).
//
// Self-contained (own class views) so the shared, matched CPlay.h stays
// untouched. /GX EH frame: CPlay_0d53d0 has a stack CString error temp, and
// CPlay_0d9290 has a stack CByteArray shuffle temp - both destructible.
#include <Gruntz/SBI_Image.h>
#include <Gruntz/StatusBarMgr.h> // InsertPtr is a method of the 0x630 host, not the SBI leaf
#include <Ints.h>
#include <Gruntz/Play.h>      // canonical CPlay (one shape)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (the RTTI-true *g_gameReg type; EnterModalUI)
#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h> // renderer A - the real CDDrawChildGroup (the +0x10 list host)
#include <Gruntz/String.h>

// ---------------------------------------------------------------------------
// THE VIEWS ARE DISSOLVED (2026-07-14). Every shape the two scans walked is a
// canonical class this tree already models:
//   Plane        -> CGameObject (<Gruntz/UserLogic.h>): m_04/m_flags(|=0x10000
//        consumed)/m_screenX/m_screenY/m_7c/m_114..m_130 and the four 16-byte
//        quads are the NAMED extent/area/config bands (m_64..m_70 @0x64,
//        m_extentL..B @0x134, m_areaL..B @0x144, m_154..m_160) - with the
//        documented 0x80000000 = "unset" sentinel the scans normalize.
//   PlaneDesc    -> AnimWorkerObj: the "+0x10 type-id fn ptr" IS m_notify (the
//        same creator-fn compare AddLevelGruntz does), +0xf0/+0x100 the two
//        by-value CTrigParam quads (m_f0../m_100..).
//   PlaneNode/PlaneList -> the CDDrawChildGroup +0x10 CObList facet + its
//        CDDrawGroupNode chain (the SAME list AddLevelGruntz walks).
//   TileObj      -> CImageSet1 (<Gruntz/ImageSets.h>): "Query" is the real slot-8
//        virtual GetCollisionAt(x, y) - no padded s0..s7 fabrication needed.
//   GridGeom     -> CLevelPlane (== CDDrawWorkerHost): m_tileGrid/m_colOffsets
//        @0x20/0x24, m_wrapW/m_wrapH @0x30/0x34, m_shiftX/m_shiftY @0x8c/0x90.
//   DrawSurf     -> CGameLevel: the "+0x4c object table" is the m_imageSets
//        CObArray INTERIOR (m_pData @0x4c; operator[] emits the same load) and
//        the "+0x5c grid" is m_mainPlane.
//   ObjSink2e4   -> CTileTriggerContainer (m_beginMarker's real type): its
//        "AddToList1" IS ?AddToList1@CTileTriggerContainer@@ @0x116cf0 and its
//        "BigDraw" IS ?AddLogic@CTileTriggerContainer@@ @0x116610 with
//        logicType 0x1a - the CCoveredPowerupLogic factory arm ("covered
//        powerup", matching this scan's own error string). The six by-value
//        Vec4s were the six CTrigParam marshaling blocks.
//   Vec3/Vec4    -> the 9-dword rock matrix buf + CTrigParam.
// ---------------------------------------------------------------------------
#include <Gruntz/ImageSets.h>            // CImageSet1::GetCollisionAt (slot 8)
#include <Gruntz/TileTriggerContainer.h> // the m_beginMarker sink (AddLogic/AddToList1)
#include <Gruntz/UserLogic.h>            // CGameObject + AnimWorkerObj (the placed objects)
#include <Gruntz/GameLevel.h>            // CGameLevel + CLevelPlane (the tile grid)

// The global game-manager singleton (*g_gameReg) at its RTTI-true type. The error
// paths call CGruntzMgr::EnterModalUI(const char*) @0x8ef10 (ILT 0x417e) - read off
// both call sites' rel32 (`mov ecx,[0x64556c]; push msg; call 0x417e`); the former
// `?ReportError@CGameRegistry@@QAEXPBD@Z` was a phantom alias of it.
extern "C" {
    DATA(0x0024556c)
    extern "C" CGruntzMgr* g_gameReg;
}

// The two plane-type discriminators the scans compare m_desc->m_typeId against
// (reloc-masked address operands; identities are irrelevant to the byte match).
extern "C" {
    void PlaneType_Rock();    // 0x40137a
    void PlaneType_Covered(); // 0x403d0f
}

// The byte array ScanShuffleQuads shuffles is the REAL MFC CByteArray (<Mfc.h> ->
// afxcoll.h): the four out-of-line ops the scan calls are NAFXCW library code
// (??0 @0x1b527e, ??1 @0x1b52b1, SetAtGrow @0x1b5485, RemoveAt @0x1b5525 - all
// FID-confirmed CByteArray). The former `CByteArrayV` view was a phantom AND laid
// out wrong (no vptr, fields 4 low); the real class has vptr@0, m_pData@+4.

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
// reached through this TU's local facet views: m_c (+0x0c) is the CSpriteFactoryHolder the plane
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
    CSpriteFactoryHolder* v = m_c;
    // retail null-tests the +0x10 list-facet address then walks its head node
    CObList* pl = (CObList*)&v->m_childGroup->m_pad10;
    if (pl == 0) {
        return 0;
    }
    CDDrawGroupNode* pos = (CDDrawGroupNode*)pl->GetHeadPosition();
    while (pos != 0) {
        CDDrawGroupNode* node = pos;
        pos = node->m_next;
        CGameObject* p = node->m_gameObj;
        if (p == 0) {
            continue;
        }
        if (p->m_extentL == (i32)0x80000000) {
            p->m_extentL = 0;
        }
        if (p->m_areaL == (i32)0x80000000) {
            p->m_areaL = 0;
        }
        if (p->m_154 == (i32)0x80000000) {
            p->m_154 = 0;
        }
        if (p->m_64 == (i32)0x80000000) {
            p->m_64 = 0;
        }
        GameObjNotifyFn vf = p->m_7c->m_notify;
        if ((void*)vf == (void*)PlaneType_Rock) {
            i32 buf[9]; // the extent/area/config bands as a 3x3 record
            buf[0] = p->m_extentL;
            buf[1] = p->m_extentT;
            buf[2] = p->m_extentR;
            buf[3] = p->m_areaL;
            buf[4] = p->m_areaT;
            buf[5] = p->m_areaR;
            buf[6] = p->m_154;
            buf[7] = p->m_158;
            buf[8] = p->m_15c;
            if (m_beginMarker
                    ->AddToList1(p->m_164, p->m_168, p->m_04, buf, p->m_11c, p->m_118, p->m_130)
                == 0) {
                CString s;
                s.Format("Bad rock at: x=%d, y=%d", p->m_screenX, p->m_screenY);
                g_gameReg->EnterModalUI(s);
                return 0;
            }
            if (p->m_11c == 0x32) {
                m_guts->InsertPtr(p->m_118, p->m_114);
            }
            p->m_flags |= 0x10000;
        } else if ((void*)vf == (void*)PlaneType_Covered) {
            CGameLevel* ds = v->m_24;
            i32 x = p->m_screenX;
            i32 y = p->m_screenY;
            if (x < 0) {
                x = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_wrapW;
                if (x >= lim) {
                    x = lim - 1;
                }
            }
            if (y < 0) {
                y = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_wrapH;
                if (y >= lim) {
                    y = lim - 1;
                }
            }
            CLevelPlane* g = ds->m_mainPlane;
            i32 shX = g->m_shiftX;
            i32 tileX = x >> shX;
            i32 shY = g->m_shiftY;
            i32 tileY = y >> shY;
            i32 subX = x - (tileX << shX);
            i32 subY = y - (tileY << shY);
            i32 cell = g->m_tileGrid[g->m_colOffsets[tileY] + tileX];
            i32 tile;
            if (cell == (i32)0xeeeeeeee || cell == (i32)0xffffffff) {
                tile = 0;
            } else {
                // the m_imageSets CObArray element's slot-8 per-pixel collision query
                tile = ((CImageSet1*)ds->m_imageSets[cell & 0xffff])->GetCollisionAt(subX, subY);
            }
            if (m_beginMarker->AddLogic(
                    tile,
                    0x1a, // TRIGID: the CCoveredPowerupLogic factory arm
                    p->m_164,
                    p->m_168,
                    p->m_04,
                    *(CTrigParam*)&p->m_extentL,
                    *(CTrigParam*)&p->m_areaL,
                    *(CTrigParam*)&p->m_154,
                    *(CTrigParam*)&p->m_64,
                    *(CTrigParam*)&p->m_7c->m_f0,
                    *(CTrigParam*)&p->m_7c->m_100,
                    p->m_124,
                    p->m_11c,
                    p->m_118,
                    p->m_130
                )
                == 0) {
                CString s;
                s.Format("Bad covered powerup at: x=%d, y=%d", p->m_screenX, p->m_screenY);
                g_gameReg->EnterModalUI(s);
                return 0;
            }
            if (p->m_11c == 0x32) {
                m_guts->InsertPtr(p->m_118, p->m_114);
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
    CSpriteFactoryHolder* v = m_c;
    // retail null-tests the +0x10 list-facet address then walks its head node
    CObList* pl = (CObList*)&v->m_childGroup->m_pad10;
    if (pl == 0) {
        return 0;
    }
    CDDrawGroupNode* pos = (CDDrawGroupNode*)pl->GetHeadPosition();

    i32 perm[4];
    ::CByteArray arr;
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
        CDDrawGroupNode* node = pos;
        pos = node->m_next;
        CGameObject* p = node->m_gameObj;
        if (p == 0) {
            continue;
        }
        GameObjNotifyFn vf = p->m_7c->m_notify;
        if ((void*)vf == (void*)PlaneQuadA || (void*)vf == (void*)PlaneQuadB
            || (void*)vf == (void*)PlaneQuadC || (void*)vf == (void*)PlaneQuadD
            || (void*)vf == (void*)PlaneQuadE) {
            p->m_124 = perm[p->m_124];
        } else if ((void*)vf == (void*)PlaneQuadF) {
            if (p->m_extentL == (i32)0x80000000) {
                p->m_extentL = 0;
            }
            if (p->m_areaL == (i32)0x80000000) {
                p->m_areaL = 0;
            }
            if (p->m_154 == (i32)0x80000000) {
                p->m_154 = 0;
            }
            if (p->m_64 == (i32)0x80000000) {
                p->m_64 = 0;
            }
            // scatter-permute the four extent-quad corners
            i32 scatter[4];
            scatter[perm[0]] = p->m_extentL;
            scatter[perm[1]] = p->m_extentT;
            scatter[perm[2]] = p->m_extentR;
            scatter[perm[3]] = p->m_extentB;
            p->m_extentL = scatter[0];
            p->m_extentT = scatter[1];
            p->m_extentR = scatter[2];
            p->m_extentB = scatter[3];
        }
    }
    return 1;
}

// --- vtable catalog ---
