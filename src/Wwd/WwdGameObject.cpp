#include <Mfc.h> // real MFC CString/CObArray/CMapStringToOb (NAFXCW, reloc-masked)
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/BoundaryUpperViews.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h> // Slot30/34/38 render targets (held surface @+0x2c)
#include <Win32.h>                     // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                     // IDirectDrawSurface::Unlock for the pixel plots
#include <DDrawMgr/DDrawBlitParam.h>
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawGroupChild/Node - the broadcast child interface
#include <rva.h>
#include <string.h>               // inlined memset / strcpy (rep stos / repne scas + rep movs)
#include <stdlib.h>               // abs() / atoi()
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/WwdGameObject.h>
#include <Ints.h>
#include <Wap32/Object.h>       // CObject - the shared engine grand-base
#include <Gruntz/ParseSource.h> // CParseSource value records (m_name/GetEntryTag) - MUST
// precede DDrawSubMgrLeafScan.h (its `class CParseSource` fwd flips MSVC5's default
// access for the later struct definition)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (mgr+0x28 reader)
#include <Wwd/WwdGameObjectFamily.h>      // the CWwdGameObjectE/A/F/B/C dtor-family hierarchy
#include <Gruntz/UserLogic.h>             // CGameObject (the sprite-resource/worker leaves)
#include <Gruntz/ResMgr.h>                // CResMgr + the three registries (m_10/m_14/m_28/m_2c)
#include <DDrawMgr/DDrawWorkerRegistry.h> // THE canonical CDDrawWorkerRegistry (was shadowed here)
#include <Gruntz/Sprite.h>                // CSprite (frame-data), CMapStringToOb, CFrameArray
#include <Gruntz/LogicRecord.h>           // CLogicRecord (the +0x80 worker under its record view)
#include <DDrawMgr/AnimWorkerObj.h>       // AnimWorkerObj (the 0x17c worker; Clear @0x151e70)
#include <DDrawMgr/DDrawWorker.h>         // CDDrawWorker (frame collection; slots 10/14/15/16)
#include <Bute/SymTab.h>                  // CSymTab iteration (FirstSym/NextSym{,2,3})
#include <DDrawMgr/DDrawSurfaceMgr.h>     // m_0c owner (m_flags bit 0x100 = single-frame)
#include <Image/CImage.h>                 // the REAL CImage (was the local CFrameWorker stand-in)
#include <DDrawMgr/DDrawShadeBlit.h>      // CDDrawShadeBlit - CImage::m_owned (was CImageFormat)
#include <Image/ImageSet.h>               // CImageSet (sparse CImage-frame collection)
#include <Gruntz/AniAdvanceCursor.h>      // canonical CAniAdvanceCursor (Advance_15c360)
// WwdGameObject.cpp - the 0x1504d0-0x152636 original TU (wave4-L dossier #15, block
// S1): ONE first-link obj weaving the CWwdGameObject live methods + CWwdGameObjectA
// render slots, the CGameObject sprite-resource/worker leaves (spriteresource +
// userbaselink), the AnimWorkerObj/CLogicRecord record leaves, and the
// CDDrawWorker/CImageSet frame-collection methods (??_7CDDrawWorker slots 10-16 span
// the whole weave - one class's virtuals across all of them). The file IS that obj;
// its former 0x15bxxx dtor block lives in src/Wwd/WwdFactoryObject.cpp (block I) and
// the 0x166xxx render block in src/Wwd/WwdGameObjectRender.cpp (block R).
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor; the wwd
// game-object/plane-object module of the WAP32 engine).
//
// Fields are typed named members at their retail offsets (matching-neutral); only
// the OFFSETS + emitted code bytes are load-bearing (campaign doctrine).

// The owning manager at CWwdGameObject+0x0c IS the game's CResMgr (<Gruntz/ResMgr.h>,
// included above). THIS FILE ALREADY KNEW: ApplyLookupSprite/ApplyName (0x1504d0 /
// 0x150540, below) reach the very same +0x0c pointer as `((CResMgr*)m_0c)->m_10->m_10map`
// - the identical mgr->m_10->map hop the former `WwdMgr`+`WwdMgrSub10` view pair
// re-modelled privately. The six private views that described its sub-objects are
// DISSOLVED (2026-07-13) onto the real classes they always were:
//
//   view (deleted)      real class (offset in CResMgr)        what proved it
//   ------------------  ------------------------------------  ---------------------------
//   WwdGridHolder       CDrawTarget*        (+0x04)           its m_limits@+0x10 IS
//   WwdGridLim          CDrawTarget::SurfaceA (@+0x10)        CDrawTarget::m_10, whose
//                                                             m_10/m_14 ARE the surface
//                                                             width/height Test() culls on
//   WwdMgrSub08         CKeyTable*          (+0x08)           map@+0x48, Lookup 0x1b8760 -
//   CMapStringToObLite  MFC CMapPtrToPtr    (@+0x48)          CKeyTable's own documented
//                                                             probe offset + rva
//   WwdMgrSub10         CImageRegistry*     (+0x10)           m_10map@+0x10; the hop this
//                                                             file already makes at 0x1504d0
//   CDDrawWorkerRegistry  the CANONICAL one (+0x14)           verbatim shadow of the real
//                       <DDrawMgr/DDrawWorkerRegistry.h>      class of the SAME NAME:
//                                                             identical CMapStringToOb@+0x10
//                                                             and identical FindKeyOfValue_
//                                                             165360(CImageSet*), which is
//                                                             already defined 100% EXACT in
//                                                             DDrawSurfacePair.cpp
//
// The "spatial grid" was never a grid: it is the draw surface, and Test()'s non-camera
// branch culls against the surface's width/height.
//
// NOT dissolved (@identity-TODO, see the WwdMgr note below): the manager POINTER itself
// and the +0x24 camera holder.

// The camera-rect holder at CResMgr+0x24. The camera object hangs at its +0x5c and its
// rect at camera+0x40; that rect is the real Win32 RECT (the former `WwdCamRect` view's
// own field comments already read left/top/right/bottom, and Test's four compares are a
// textbook rect-overlap test). @identity-TODO: the HOLDER class itself is unrecovered -
// CResMgr types +0x24 as CMenuViewObj*, which is forward-declared only (no layout), so
// there is no existing class to fold onto. Left as a flagged view, not guessed.
struct WwdCamHolder {
    char m_pad00[0x5c];
    char* m_5c; // +0x5c  the camera object; its RECT sits at +0x40
};
SIZE_UNKNOWN(WwdCamHolder);

// CWwdGameObject+0x0c owning manager.
//
// @identity-TODO: this IS CResMgr (every member below lands on a CResMgr slot at the
// same offset, and the file's own 0x1504d0/0x150540 cast the same pointer to CResMgr).
// It is NOT folded onto CResMgr yet because two of CResMgr's slots are typed by a name
// this file's calls contradict - the same object under two class names:
//   +0x14  CResMgr says CImageRegistry*   / here FindKeyOfValue_165360 proves
//          CDDrawWorkerRegistry  (CResMgr's OWN comments already concede this:
//          "Has @0x155550 IS CDDrawWorkerRegistry::HasKeyEqual_155550")
//   +0x28  CResMgr says CSoundRegistry*   / here FindKeyOfValue_158570 + the +0x10 map
//          prove CDDrawSubMgrLeafScan
// Unifying those names is a CLASS-IDENTITY fold across two polymorphic, vtable-bearing
// classes (CImageRegistry carries a documented vtable wall) - out of scope for a view
// pass. Handed off. Until then the members are typed by the class whose methods are
// ACTUALLY CALLED on them, which is the honest shape.
struct WwdMgr {
    char m_pad00[0x04];
    CDrawTarget* m_drawTarget; // +0x04  active draw surface (Test off-screen cull extent)
    CKeyTable* m_8;            // +0x08  world/key table (its +0x48 map = the kill-cue map)
    char m_pad0c[0x10 - 0x0c];
    CImageRegistry* m_10;       // +0x10  image/tile registry (name -> sprite, m_10map)
    CDDrawWorkerRegistry* m_14; // +0x14  worker registry (FindKeyOfValue_165360)
    char m_pad18[0x24 - 0x18];
    WwdCamHolder* m_camera;     // +0x24  camera-rect holder (Test off-screen cull)
    CDDrawSubMgrLeafScan* m_28; // +0x28  leaf-scan registry (FindKeyOfValue_158570)
};

// The 0xa0-byte snapshot record WriteSnapshot assembles on the stack.
struct WwdSnapshot {
    i32 m_00;          // m_4
    i32 m_04;          // m_188
    i32 m_08;          // this->GetTypeId()
    i32 m_0c;          // 0 or this->Vfunc40() when GetTypeId()==0x1c
    i32 m_10;          // 0 or worker->m_18->GetTypeTag()
    char m_name[0x80]; // +0x14  name string from the mgr
    i32 m_94;          // m_5c
    i32 m_98;          // m_60
    i32 m_9c;          // m_74
};

// (The former WorkerSub view of the worker's +0x18 sub-object is DISSOLVED: the
// object is the bound logic leaf - CUserBase/CUserLogic (<Gruntz/UserLogic.h>;
// CGameObjAux::m_logic types the same slot) - and its "+0x8 virtual" is
// CUserBase slot 2 = GetTypeTag, exactly the type tag the snapshot stores.)

// Raw this-offset read of a foreign engine object reached as an opaque void*/int
// handle (found-object refs, the a4 setup source). authentic: these referents are
// heterogeneous unmodeled engine objects (no single concrete class to type), so a
// documented offset read is the deliberate access - only the offset is load-bearing.
#define F(p, off, ty) (*(ty*)((char*)(p) + (off)))

// The global NAFXCW allocator/deallocator (::operator new @0x1b9b46 = ??2@YAPAXI@Z,
// ::operator delete @0x1b9b82 = ??3@YAXPAX@Z; both reloc-masked rel32).
extern void* operator new(u32 size);
extern void operator delete(void* p);

// The per-frame draw-delta / advance-context global handed to Advance_15c360. Canonical
// _g_6bf3bc, DATA-defined in the GruntCreationPoint (tilelogicpump) TU @ RVA 0x2bf3bc;
// referenced by that name so the reloc binds (the former g_defaultGeo was a local misnomer).
extern "C" u32 g_engineFrameDelta; // 0x2bf3bc

// ---------------------------------------------------------------------------
// The frame-worker is a CImage (RTTI .?AVCImage@@, SHARED vtable ??_7CImage@@6B@
// @0x1eaa2c / VA 0x5eaa2c, cataloged in config/vtable_names.csv). The insert
// allocates a raw 0x34-byte CImage and INLINES its construction (vptr stamp + field
// init) at the new-site, then drives the slot-11 Resolve virtual and, on failure,
// the slot-1 scalar dtor. Real-polymorphic (all-vtables mandate).
// ---------------------------------------------------------------------------
// (the CFrameWorker stand-in is GONE - it WAS CImage, exactly as its own comment above
// said: RTTI .?AVCImage@@, the SHARED ??_7CImage@@6B@ vtable @0x1eaa2c, 0x34 bytes, the
// same 13 slots. Its ~12 declared-only virtuals mangled as ?X@CFrameWorker@@ - PHANTOMS
// no obj and no .LIB could ever define. The canonical <Image/CImage.h> class emits the
// real ??_7CImage vtable whose slots are all rva-bound bodies, so they resolve.)

// (the TU-local ShadeSelector view is GONE - and so is the shared one: 0x14dd90 is
// CDDrawShadeBlit::Select, reached straight off CImage::m_owned. ShadeDescr comes from
// <DDrawMgr/DDrawShadeBlit.h>.)

// The frame ctor CreateFrame24/28/30 inline (vptr stamp + field init at the new-site).
// The frame IS a CImage, so retail spells out `new CImage` + the same 7 field stores
// INLINE at each new-site (a CImage ctor call would emit a `call`; a `static inline`
// NewFrame helper is NOT inlined by MSVC5 either - it emits a call). CImage.h is
// included tree-wide and its own comment warns that touching the class (e.g. adding a
// parameterized ctor) is a heavy /O2 butterfly for no binding gain, so the construction
// is duplicated at each site to match retail's inline shape.

// Stamp helper retired: the worker builds are real `new`-less inline constructions
// whose vptr install is dropped (compiler-emitted vtable; % ok per drive-to-0).
static inline void StampWorkerVtbl(AnimWorkerObj* w) {
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
}

// ===========================================================================
// CGameObject::ApplyGeometryDirect @0x58b60 - COMDAT-at-usage exile of this TU's
// geometry-apply pair, kept at the 0x58xxx gruntcombat obj (file-head position).
// The direct counterpart of ApplyLookupGeometry: the sprite source is passed in
// directly (no name lookup). __thiscall, ret 8.
// ===========================================================================
RVA(0x00058b60, 0x2d)
void CGameObject::ApplyGeometryDirect(i32 srcSprite, i32 applyDefault) {
    ((CDDrawBlitParam*)((char*)this + 0x1a0))->Setup_15c2d0((CDDrawBlitParamSrc*)srcSprite);
    if (applyDefault) {
        ((CAniAdvanceCursor*)((char*)this + 0x1a0))->Advance_15c360(g_engineFrameDelta);
    }
}

// ===========================================================================
// CGameObject::ApplyLookupSprite @0x1504d0 - look the named sprite up through
// m_c->m_10, cache it + the caller-supplied frame number/frame ptr.
// (Role-union note: CGameObject's m_0c/+0x190/+0x194/+0x198/+0x19c are ROLE-UNION -
// for a WwdFile-loaded object they are world/source-def/layer/stamp; for a
// CreateSprite'd sprite they are the resource holder / cached CSprite / frame ptr /
// frame number. The reinterpreting casts are the authentic union access.)
// ===========================================================================
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// the `mov [&spr],0` sinks past the arg pushes + the extra frame arg flips the
// sprite/frame eax<->ecx allocation; identical instruction multiset, ~84%. Same
// wall as the sibling ApplyName (89%). Logic complete.
RVA(0x001504d0, 0x6c)
void CGameObject::ApplyLookupSprite(const char* name, i32 frame) {
    CSprite* spr = 0;
    ((CMapStringToOb*)&((CResMgr*)m_0c)->m_10->m_10map)->Lookup(name, (CObject*&)spr);
    m_194 = (char*)spr; // +0x194 union: cached sprite
    if (spr) {
        if (frame >= spr->m_firstFrame && frame <= spr->m_lastFrame) {
            m_190 = frame;
            m_layer = (CGameObjLayer*)spr->m_frames.m_pData[frame]; // +0x198 union: frame ptr
        } else {
            m_190 = frame;
            m_layer = 0;
        }
    }
}

// ===========================================================================
// CGameObject::ApplyName @0x150540 - as ApplyLookupSprite, but the frame number is
// the sprite's own first frame (spr->m_64). The first compare of the inlined range
// guard is m_64 vs m_64 (always equal); written verbatim so MSVC emits both reads.
// ===========================================================================
RVA(0x00150540, 0x65)
void CGameObject::ApplyName(const char* name) {
    CSprite* spr = 0;
    ((CMapStringToOb*)&((CResMgr*)m_0c)->m_10->m_10map)->Lookup(name, (CObject*&)spr);
    m_194 = (char*)spr; // +0x194 role-union: the cached sprite (vs a trigger's source-def)
    if (spr) {
        i32 n = spr->m_firstFrame;
        m_190 = n; // +0x190 role-union: the cached frame number
        if (n >= spr->m_firstFrame && n <= spr->m_lastFrame) {
            m_layer = (CGameObjLayer*)spr->m_frames.m_pData[n]; // +0x198 union: the frame ptr
            return;
        }
    }
    m_layer = 0;
}

// ===========================================================================
// CGameObject::ApplyLookupGeometry @0x1505b0 - look a named sprite-set up through
// this->m_c->m_2c->map and, on a hit, drive the geometry sub-player @this+0x1a0:
// Setup_15c2d0(spr); then, when the second arg is set, apply the global default
// geometry source g_engineFrameDelta via Advance_15c360. __thiscall, ret 8.
// ===========================================================================
RVA(0x001505b0, 0x5c)
i32 CGameObject::ApplyLookupGeometry(const char* name, i32 applyDefault) {
    CSprite* spr = 0;
    ((CMapStringToOb*)&((CResMgr*)m_0c)->m_2c->m_10map)->Lookup(name, (CObject*&)spr);
    if (!spr) {
        return 0;
    }
    // +0x1a0 is the per-class anim sub-object (raw offset by CGameObject convention).
    ((CDDrawBlitParam*)((char*)this + 0x1a0))->Setup_15c2d0((CDDrawBlitParamSrc*)(i32)spr);
    if (applyDefault) {
        ((CAniAdvanceCursor*)((char*)this + 0x1a0))->Advance_15c360(g_engineFrameDelta);
    }
    return 1;
}

// ===========================================================================
// CGameObject::LookupAnimSprite @0x150610 - look the named sprite-set up through
// this->m_c->m_28->map; on a hit cache it at +0x19c and return 1. __thiscall, ret 4.
// ===========================================================================
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// identical instruction multiset to the sibling Apply* lookups; the `mov [&spr],0`
// sinks past the arg pushes. ~73%; logic complete, deferred to the final sweep.
RVA(0x00150610, 0x41)
i32 CGameObject::LookupAnimSprite(const char* name) {
    CSprite* spr = 0;
    ((CMapStringToOb*)&((CResMgr*)m_0c)->m_28->m_10map)->Lookup(name, (CObject*&)spr);
    if (spr != 0) {
        m_19c = (i32)spr; // +0x19c union: the cached anim sprite (vs a WwdFile stamp)
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x150660 (vtable slot 12): snapshot the live 9-dword dirty-rect record (0x18..0x3c)
// into the shadow block (0xb8..0xdc); then, if the live record is armed (m_20.b !=
// -1), BltFast the source pair's surface onto the target pair's at the record's
// (left, top) with the record as the source rect + colorkey/wait, and disarm the
// record. __thiscall, 2 args (ret 8).
RVA(0x00150660, 0x49)
void CWwdGameObjectA::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    memcpy(&m_b8, &m_18, 36);
    if (m_20.b != -1) {
        RECT* r = (RECT*)&m_20;
        a->m_surface->BltFast(r->left, r->top, b->m_surface, r, 0x10);
        m_20.b = -1;
    }
}

// ---------------------------------------------------------------------------
// 0x1506b0 (vtable slot 13): CWwdGameObjectA's dirty-rect BltEx dispatch. Same
// two-record (live m_38 / shadow m_d8) structure as CWwdGameObjectC::Slot34, but
// the "both armed" combine uses the Win32 rect API: IntersectRect tests overlap
// and, if they overlap, UnionRect gives the covering rect {left,top,right+1,
// bottom+1}; if disjoint, blit each record separately. Only one armed -> that
// record. Each rect is {x,y,x+w,y+h}. Arg `c` unused. __thiscall, 3 args (ret 0xc).
// @early-stop
// ~74% tail-merge + regalloc wall (twin of CWwdGameObjectC::Slot34 @76%): logic/CFG/
// the IntersectRect/UnionRect union path + the four {x,y,x+w,y+h} BltEx sites over the
// one shared rc buffer all reproduced, but cl cross-jumps (tail-merges) the identical
// BltEx(rc,b->m_surface,rc,...) calls where retail keeps them inline, plus a callee-saved
// record-base coloring swap. Not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x001506b0, 0x1ec)
void CWwdGameObjectA::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) {
    i32 rc[4]; // reused src+dst blit rect buffer
    if (m_20.b != -1 && m_d8 != -1) {
        RECT ir;
        if (IntersectRect(&ir, (RECT*)&m_20, (RECT*)&m_c0)) {
            UnionRect(&ir, (RECT*)&m_20, (RECT*)&m_c0);
            i32 w = ir.right - ir.left + 1;
            i32 h = ir.bottom - ir.top + 1;
            rc[0] = ir.left;
            rc[1] = ir.top;
            rc[2] = ir.left + w;
            rc[3] = ir.top + h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        } else {
            rc[0] = m_18;
            rc[1] = m_1c;
            rc[2] = m_18 + m_20.m_w;
            rc[3] = m_1c + m_20.m_h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
            rc[0] = m_b8;
            rc[1] = m_bc;
            rc[2] = m_b8 + m_d0;
            rc[3] = m_bc + m_d4;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        }
    } else if (m_20.b != -1) {
        rc[0] = m_18;
        rc[1] = m_1c;
        rc[2] = m_18 + m_20.m_w;
        rc[3] = m_1c + m_20.m_h;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    } else if (m_d8 != -1) {
        rc[0] = m_b8;
        rc[1] = m_bc;
        rc[2] = m_b8 + m_d0;
        rc[3] = m_bc + m_d4;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    }
}

// ---------------------------------------------------------------------------
// 0x1508a0 (vtable slot 14): CWwdGameObjectA's dirty-rect blit-hook dispatch. Same
// as Slot34 but dispatches the empty 0x164650 hook per (pos,size) region instead of
// BltEx. "Both armed" combine again via IntersectRect/UnionRect: one region over the
// union {pos={left,top}, size={w,h}} when they overlap, else both records. Only one
// armed -> that record. Arg `c` unused. __thiscall, 3 args (ret 0xc).
// @early-stop
// ~91% zero-register-pinning wall (twin of CWwdGameObjectC::Slot38 @99.7%): logic/CFG/
// the union pos/size build + all four BlitDirtyRect sites byte-exact. Residual is the
// callee-saved coloring of the two hoisted record bases (&m_18,&m_b8) -> retail edi/ebx
// vs cl ebx/edi, cascading a few push operands; the extra IntersectRect/UnionRect path
// (absent in the twin) adds the register pressure that keeps this below the twin's 99.7%.
// Permuter found no operand-order gain. docs/patterns/zero-register-pinning.md.
RVA(0x001508a0, 0x117)
void CWwdGameObjectA::BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) {
    if (m_20.b != -1 && m_d8 != -1) {
        RECT ir;
        if (IntersectRect(&ir, (RECT*)&m_20, (RECT*)&m_c0)) {
            UnionRect(&ir, (RECT*)&m_20, (RECT*)&m_c0);
            i32 pos[2];
            i32 size[2];
            pos[0] = ir.left;
            size[0] = ir.right - ir.left + 1;
            size[1] = ir.bottom - ir.top + 1;
            pos[1] = ir.top;
            a->BlitDirtyRect_164650(b, pos, size);
        } else {
            a->BlitDirtyRect_164650(b, &m_18, &m_20.m_w); // live record
            a->BlitDirtyRect_164650(b, &m_b8, &m_d0);     // shadow record
        }
    } else if (m_20.b != -1) {
        a->BlitDirtyRect_164650(b, &m_18, &m_20.m_w); // live record only
    } else if (m_d8 != -1) {
        a->BlitDirtyRect_164650(b, &m_b8, &m_d0); // shadow record only
    }
}

// ---------------------------------------------------------------------------
// CWwdGameObject::Test (0x1509c0): on-screen visibility cull. Derive the object's
// four edges from its centre (m_posX/m_posY) and the sprite half-extents (m_198),
// then bounds-check against either the camera rect (when the 0x40000 flag is set)
// or the plane grid limits. __thiscall, 0 args.
// @early-stop
// regalloc wall (~73%): the four derived edges + m_198/m_mgr/m_flags want 4 callee-saved
// regs where retail packs them into 3 (ebx/esi/edi, m_198 kept in edi, m_flags tested from
// memory). No source spelling reproduces retail's exact edge-register assignment; both the
// camera-rect and grid-extent bounds checks are byte-faithful.
RVA(0x001509c0, 0xab)
i32 CWwdGameObject::Test() {
    CGameObjLayer* e = m_198;
    if (!e) {
        return 0;
    }
    i32 right = m_posX + e->m_halfWidth;
    i32 left = m_posX - e->m_halfWidth;
    i32 top = m_posY - e->m_halfHeight;
    i32 bottom = m_posY + e->m_halfHeight;
    if (m_flags & 0x40000) {
        RECT* r = (RECT*)(m_mgr->m_camera->m_5c + 0x40);
        if (right < r->left) {
            return 0;
        }
        if (left > r->right) {
            return 0;
        }
        if (bottom < r->top) {
            return 0;
        }
        return top <= r->bottom;
    } else {
        // The non-camera cull bounds are the DRAW SURFACE's extent (m_10/m_14 = width/
        // height of the draw target's frame page) - the former "grid limits" view.
        CDrawTarget::SurfaceA* g = m_mgr->m_drawTarget->m_10;
        if (right < 0) {
            return 0;
        }
        if (left >= g->m_10) {
            return 0;
        }
        if (bottom < 0) {
            return 0;
        }
        return top < g->m_14;
    }
}

// ---------------------------------------------------------------------------
// Dispatch (0x150a70): look the request up in the +0x1a0 command map; on a hit,
// route by `type`: 4 -> ReadState, 7 -> Sub150c30 (abort on failure), then play.
// ---------------------------------------------------------------------------
RVA(0x00150a70, 0x89)
i32 CWwdGameObject::Dispatch(i32 a1, i32 type, i32 a3, i32 a4) {
    if (a1 == 0) {
        return 0;
    }
    if (m_cmdMap.Find((CSerialArchive*)a1, type, a3, a4) == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (ReadState(a1) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Sub150c30(a1) == 0) {
                return 0;
            }
            break;
    }
    return Play(a1, type, a3, a4) != 0;
}

// ---------------------------------------------------------------------------
// ReadState (0x150b00): pull four fields back through the archive at the
// requested object (ebx), copy its name string, then re-emit them.
// ---------------------------------------------------------------------------
// @early-stop
// frame-slot-coloring wall (99.39%): buffer corrected to char[0x100] (frame now the
// retail sub esp,0x108, cf. read-twin Sub150c30), body byte-identical, but MSVC5 colors
// the two 4-byte scalars (flag / CStringVal str) into the swapped esp slots vs retail
// (base str@[esp+0x10]/flag@[esp+0x14]; retail flag@0x10/str@0x14) - one `lea ecx`
// operand differs. Not steerable by decl/scope order (tried block/hoist/reorder).
RVA(0x00150b00, 0x12b)
i32 CWwdGameObject::ReadState(i32 src) {
    CSerialArchive* ar = (CSerialArchive*)src;
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_dotColor, 4);
    ar->Write(&m_190, 4);
    i32 flag = 0;
    if (m_198 != 0) {
        flag = 1;
    }
    ar->Write(&flag, 4);

    char tmp[0x100]; // 256-byte name scratch (only 0x80 written; cf. Sub150c30's name[0x100])
    memset(tmp, 0, 0x80);
    if (m_194 != 0) {
        strcpy(tmp, m_194->m_name); // CSprite::m_name IS the +0x24 the raw read used
    }
    ar->Write(tmp, 0x80);

    memset(tmp, 0, 0x80);
    {
        CString str = m_mgr->m_28->FindKeyOfValue_158570(m_19c);
        strcpy(tmp, str);
    }
    ar->Write(tmp, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// Sub150c30 (0x150c30): the read/load counterpart of ReadState - pull two ints
// (+0x18c/+0x190), a flag, and a name back through the archive's +0x2c slot,
// look the name up in the mgr's first map to resolve m_194; when the flag is 1
// and the lookup hit, read m_198 from the resolved object's bounded +0x14 table
// indexed by m_190. Then read a second name, look it up in the mgr's second map
// to resolve m_19c. (Dispatch case 7.)
// ---------------------------------------------------------------------------
RVA(0x00150c30, 0x130)
i32 CWwdGameObject::Sub150c30(i32 src) {
    CSerialArchive* ar = (CSerialArchive*)src;
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_dotColor, 4);
    ar->Read(&m_190, 4);
    i32 flag;
    ar->Read(&flag, 4);
    m_194 = 0;

    char name[0x100];
    ar->Read(name, 0x80);
    if (strlen(name) != 0) {
        // Identical to CGameObject::ApplyLookupSprite (0x1504d0): bounds-check the frame
        // number against the sprite's valid range, then index its frame array. The former
        // F(found,0x64/0x68/0x14) offset reads ARE m_firstFrame / m_lastFrame / m_frames.
        CSprite* found = 0;
        WwdMgr* mgr = m_mgr;
        mgr->m_10->m_10map.Lookup(name, (CObject*&)found);
        m_194 = found;
        if (found != 0 && flag == 1) {
            i32 idx = m_190;
            CGameObjLayer* frame;
            if (idx >= found->m_firstFrame && idx <= found->m_lastFrame) {
                frame = (CGameObjLayer*)found->m_frames.m_pData[idx];
            } else {
                frame = 0;
            }
            m_198 = frame;
        }
    }

    m_19c = 0;
    ar->Read(name, 0x80);
    if (strlen(name) != 0) {
        // m_28's map is a CMapStringToPtr - a genuinely void*-valued container, so the
        // element cast is authentic (it is the map's own interface, not a missing type).
        void* found = 0;
        WwdMgr* mgr = m_mgr;
        mgr->m_28->m_10.Lookup(name, found);
        m_19c = (LeafScanValue*)found;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Setup (0x150d60, vtbl +0x28): wire 3 args + worker, init the wide state
// block, init the worker (vtbl +0x24), and fold its flag bits into m_08.
// ---------------------------------------------------------------------------
// @early-stop
// ~97% scheduling wall: cl hoists the m_5c/m_60 loads (for the m_ac/m_b0
// stores) to the top of the post-Init block and interleaves the three
// 0x80000000 stores into the zero-fill run; retail loads them just-in-time.
// Logic complete; the unrolled init-block store order is not steerable.
RVA(0x00150d60, 0x14d)
i32 CWwdGameObject::Setup(i32 a1, i32 a2, i32 a3, i32 a4) {
    Helper164790(a1, a2);
    m_posX = a1;
    m_posY = a2;
    m_sortKey = a3;
    m_104 = a1;
    AnimWorkerObj* w = m_worker;
    m_108 = a2;
    m_10c = a3;
    m_f8 = 10;
    m_fc = 10;
    m_118 = 0;
    m_114 = 0;
    m_128 = 0;
    m_124 = 0;
    m_11c = 0;
    m_120 = 0;
    m_12c = 0;
    m_130 = 0;
    m_164 = 0;
    m_168 = 0;
    m_e0 = 0;
    m_180 = 0;
    if (w->Vfunc24(F((void*)a4, 0x10, i32), F((void*)a4, 0x8, i32)) == 0) {
        return 0;
    }
    m_80 = 0;
    m_88 = 0;
    m_90 = 0;
    m_84 = 0;
    m_8c = 0;
    m_94 = 0;
    m_e8 = 0;
    m_ec = 0;
    m_f0 = 0;
    m_f4 = 0;
    m_134 = (i32)0x80000000;
    m_144 = (i32)0x80000000;
    m_154 = (i32)0x80000000;
    m_region.m_object = this;
    m_region.m_x = m_posX;
    m_region.m_y = m_posY;
    i32 wf = m_worker->m_08;
    if (wf & 1) {
        m_flags |= 0x800000;
        return 1;
    }
    if (wf & 2) {
        m_flags |= 0x1000000;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGameObject::EnsureWorker80 (0x150eb0): the +0x80 worker variant of
// EnsureWorker88/90 - same lazy build/reuse/feed, but it RETURNS the slot-9 result
// (or 0 on the null guards). Called by AddLogicHit (0x150f50).
// @early-stop
// Expected to share the zero-register-pinning wall of EnsureWorker88/90 (this/0 in
// esi<->edi). Logic byte-exact; a pure allocator coin-flip, not source-steerable.
RVA(0x00150eb0, 0x98)
i32 CGameObject::EnsureWorker80(CGameObject* src) {
    if (src == 0) {
        return 0;
    }
    if (m_80 != 0) {
        m_80->Clear();
    } else {
        AnimWorkerObj* w = (AnimWorkerObj*)::operator new(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_80 = w;
    }
    if (m_80 == 0) {
        return 0;
    }
    return m_80->Vfunc24(src->m_10, 0);
}

// CGameObject's three built-in logic-handler registrars: look the logic-name key
// up in the world's CMapStringToOb (m_0c -> +0x14 -> +0x10), then feed the found
// handler through the matching lazy worker slot (Hit -> 80, Attack -> 88, Bump -> 90).
// @early-stop
// scheduling coin-flip: body byte-exact EXCEPT the `handler = 0` slot-init lands one
// push early (push &out; STORE; push key) where retail schedules it after both pushes
// (push &out; push key; STORE). Same slot, independent store; MSVC5's scheduler places
// it between the arg pushes. No source ordering of the init reproduces the late slot.
RVA(0x00150f50, 0x33)
void CGameObject::AddLogicHit(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    ((CMapStringToPtr*)map)->Lookup(key, (void*&)handler);
    EnsureWorker80(handler);
}

// CGameObject::EnsureWorker88 (0x150f90): lazily build the +0x88 worker - if one
// already exists, just re-run its slot-7 reuse hook; otherwise operator new a
// fresh 0x17c-byte worker (seeded m_04=this->m_4, m_08=0, m_0c=this->m_c, all other
// fields 0), stow it at +0x88, then feed src->m_10 through slot 9.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the whole
// build sequence + both dispatches are byte-identical, but retail pins this->edi
// and 0->esi while cl pins this->esi and 0->edi, and lowers the `arg==0` guard as
// an early `xor eax,eax;ret` block where cl shares the epilogue - the swap cascades
// every esi/edi. Logic exact; a pure allocator coin-flip, not source-steerable.
RVA(0x00150f90, 0x98)
i32 CGameObject::EnsureWorker88(CGameObject* src) {
    if (src == 0) {
        return 0;
    }
    if (m_88 != 0) {
        m_88->Clear();
    } else {
        AnimWorkerObj* w = (AnimWorkerObj*)::operator new(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_88 = w;
    }
    if (m_88 == 0) {
        return 0;
    }
    return m_88->Vfunc24(src->m_10, 0);
}

// @early-stop
// same `handler = 0` scheduling coin-flip as AddLogicHit.
RVA(0x00151030, 0x33)
void CGameObject::AddLogicAttack(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    ((CMapStringToPtr*)map)->Lookup(key, (void*&)handler);
    EnsureWorker88(handler);
}

// CGameObject::EnsureWorker90 (0x151070): identical to EnsureWorker88 but for the
// +0x90 worker slot.
// @early-stop
// same zero-register-pinning wall as EnsureWorker88 (this/0 in esi<->edi).
RVA(0x00151070, 0x98)
i32 CGameObject::EnsureWorker90(CGameObject* src) {
    if (src == 0) {
        return 0;
    }
    if (m_collideWorker != 0) {
        m_collideWorker->Clear();
    } else {
        AnimWorkerObj* w = (AnimWorkerObj*)::operator new(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_collideWorker = w;
    }
    if (m_collideWorker == 0) {
        return 0;
    }
    return m_collideWorker->Vfunc24(src->m_10, 0);
}

// @early-stop
// same `handler = 0` scheduling coin-flip as AddLogicHit.
RVA(0x00151110, 0x33)
void CGameObject::AddLogicBump(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    ((CMapStringToPtr*)map)->Lookup(key, (void*&)handler);
    EnsureWorker90(handler);
}

// ---------------------------------------------------------------------------
// Play (0x151150, vtbl +0x3c): switch on `type` (3..8); drive the worker
// through animation states 0x50..0x53 around the inner step.
// ---------------------------------------------------------------------------
// @early-stop
// tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md): retail
// inlines a separate play-state dance per case (3,4 distinct; 7,8 share the
// 0x15129b tail via fall-through) using different scratch regs (edx vs ecx for
// the worker) per case context; our cl cross-jumps all four dances to one shared
// tail. Logic complete; the per-case regalloc/tail-merge layout is a
// compiler-internal choice not steerable from C.
RVA(0x00151150, 0x175)
i32 CWwdGameObject::Play(i32 a1, i32 type, i32 a3, i32 a4) {
    if (a1 == 0) {
        return 0;
    }
    AnimWorkerObj* w;
    i32 saved;
    i32 node;
    switch (type) {
        case 3: {
            m_184 = 0;
            if (m_98 != 0) {
                m_184 = F(m_98, 0x188, i32);
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x50;
            w->m_collideNotify((CGameObject*)this);
            w = m_worker;
            if (w->m_1c == 0x50) {
                w->m_1c = saved;
            }
            break;
        }
        case 4: {
            if (Serialize(a1) == 0) {
                return 0;
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x51;
            w->m_collideNotify((CGameObject*)this);
            w = m_worker;
            if (w->m_1c == 0x51) {
                w->m_1c = saved;
            }
            break;
        }
        case 7: {
            if (Sub151780(a1) == 0) {
                return 0;
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x52;
            w->m_collideNotify((CGameObject*)this);
            w = m_worker;
            if (w->m_1c == 0x52) {
                w->m_1c = saved;
            }
            break;
        }
        case 8: {
            node = m_184;
            if (node != 0) {
                void* found = 0;
                if (m_mgr->m_8->m_map48.Lookup((void*)node, found) == 0) {
                    m_98 = 0;
                } else {
                    m_98 = found;
                }
            } else {
                m_98 = 0;
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x53;
            w->m_collideNotify((CGameObject*)this);
            w = m_worker;
            if (w->m_1c == 0x53) {
                w->m_1c = saved;
            }
            break;
        }
    }
    return m_killCue->Dispatch(a1, type, (void*)a3, (void*)a4) != 0;
}

// ---------------------------------------------------------------------------
// Serialize (0x151320): read a 0x24 block + a name string, then ~13 dwords.
// ---------------------------------------------------------------------------
RVA(0x00151320, 0x454)
i32 CWwdGameObject::Serialize(i32 arParam) {
    CSerialArchive* ar = (CSerialArchive*)arParam;
    if (ar == 0) {
        return 0;
    }

    ar->Write(m_b8, 0x24);

    char tmp[0x80];
    memset(tmp, 0, sizeof(tmp));
    strcpy(tmp, m_name);
    ar->Write(tmp, 0x80);

    ar->Write(&m_e4, 4);
    ar->Write(&m_e8, 4);
    ar->Write(&m_ec, 4);
    ar->Write(&m_f0, 4);
    ar->Write(&m_f4, 4);
    ar->Write(&m_f8, 4);
    ar->Write(&m_fc, 4);
    ar->Write(&m_100, 4);
    ar->Write(&m_104, 4);
    ar->Write(&m_108, 4);
    ar->Write(&m_10c, 4);
    ar->Write(&m_110, 4);
    ar->Write(&m_114, 4);
    ar->Write(&m_118, 4);
    ar->Write(&m_11c, 4);
    ar->Write(&m_120, 4);
    ar->Write(&m_124, 4);
    ar->Write(&m_128, 4);
    ar->Write(&m_12c, 4);
    ar->Write(&m_130, 4);
    ar->Write(&m_134, 0x10);
    ar->Write(&m_144, 0x10);
    ar->Write(&m_154, 0x10);
    ar->Write(&m_164, 4);
    ar->Write(&m_168, 4);
    ar->Write(&m_16c, 4);
    ar->Write(&m_170, 4);
    ar->Write(&m_174, 4);
    ar->Write(&m_178, 4);
    ar->Write(&m_17c, 4);
    ar->Write(&m_180, 4);
    ar->Write(&m_10, 4);
    ar->Write(&m_14, 4);
    ar->Write(&m_lastX, 0x24); // +0x18 render-state block
    ar->Write(&m_40, 4);
    ar->Write(&m_44, 4);
    ar->Write(&m_48, 4);
    ar->Write(&m_50, 4);
    ar->Write(&m_54, 4);
    ar->Write(&m_58, 4);
    ar->Write(&m_clipLeft, 0x10); // +0x64 clip rect
    ar->Write(&m_04, 4);
    ar->Write(&m_flags, 4);
    ar->Write(&m_184, 4);

    memset(tmp, 0, sizeof(tmp));
    if (m_80 != 0) {
        CString str = m_mgr->m_14->FindKeyOfValue_165360((CImageSet*)m_80);
        strcpy(tmp, str);
    }
    ar->Write(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    if (m_88 != 0) {
        CString str = m_mgr->m_14->FindKeyOfValue_165360((CImageSet*)m_88);
        strcpy(tmp, str);
    }
    ar->Write(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    if (m_90 != 0) {
        CString str = m_mgr->m_14->FindKeyOfValue_165360((CImageSet*)m_90);
        strcpy(tmp, str);
    }
    ar->Write(tmp, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// Sub151780 (0x151780): the read/load mirror of Serialize - pull the same field
// block back through the archive's +0x2c read slot (assigning the +0xdc name
// CString), then resolve three object references by reading a name, looking it
// up in the mgr's registry map, and handing the hit to the matching setter.
// (Dispatch/Play case 7.) Same offset/size sweep as Serialize, reversed.
// ---------------------------------------------------------------------------
RVA(0x00151780, 0x40d)
i32 CWwdGameObject::Sub151780(i32 arParam) {
    CSerialArchive* ar = (CSerialArchive*)arParam;
    if (ar == 0) {
        return 0;
    }

    ar->Read(m_b8, 0x24);

    char name[0x80];
    ar->Read(name, 0x80);
    *(CString*)&m_name = name;

    ar->Read(&m_e4, 4);
    ar->Read(&m_e8, 4);
    ar->Read(&m_ec, 4);
    ar->Read(&m_f0, 4);
    ar->Read(&m_f4, 4);
    ar->Read(&m_f8, 4);
    ar->Read(&m_fc, 4);
    ar->Read(&m_100, 4);
    ar->Read(&m_104, 4);
    ar->Read(&m_108, 4);
    ar->Read(&m_10c, 4);
    ar->Read(&m_110, 4);
    ar->Read(&m_114, 4);
    ar->Read(&m_118, 4);
    ar->Read(&m_11c, 4);
    ar->Read(&m_120, 4);
    ar->Read(&m_124, 4);
    ar->Read(&m_128, 4);
    ar->Read(&m_12c, 4);
    ar->Read(&m_130, 4);
    ar->Read(&m_134, 0x10);
    ar->Read(&m_144, 0x10);
    ar->Read(&m_154, 0x10);
    ar->Read(&m_164, 4);
    ar->Read(&m_168, 4);
    ar->Read(&m_16c, 4);
    ar->Read(&m_170, 4);
    ar->Read(&m_174, 4);
    ar->Read(&m_178, 4);
    ar->Read(&m_17c, 4);
    ar->Read(&m_180, 4);
    ar->Read(&m_10, 4);
    ar->Read(&m_14, 4);
    ar->Read(&m_lastX, 0x24); // +0x18 render-state block
    ar->Read(&m_40, 4);
    ar->Read(&m_44, 4);
    ar->Read(&m_48, 4);
    ar->Read(&m_50, 4);
    ar->Read(&m_54, 4);
    ar->Read(&m_58, 4);
    ar->Read(&m_clipLeft, 0x10); // +0x64 clip rect
    ar->Read(&m_04, 4);
    ar->Read(&m_flags, 4);
    ar->Read(&m_184, 4);

    ar->Read(name, 0x80);
    if (strlen(name) != 0) {
        CObject* found = 0;
        m_mgr->m_14->m_map.Lookup(name, found);
        if (((CGameObject*)this)->EnsureWorker80((CGameObject*)found) == 0) {
            return 0;
        }
    }

    ar->Read(name, 0x80);
    if (strlen(name) != 0) {
        CObject* found = 0;
        m_mgr->m_14->m_map.Lookup(name, found);
        if (((CGameObject*)this)->EnsureWorker88((CGameObject*)found) == 0) {
            return 0;
        }
    }

    ar->Read(name, 0x80);
    if (strlen(name) != 0) {
        CObject* found = 0;
        m_mgr->m_14->m_map.Lookup(name, found);
        if (((CGameObject*)this)->EnsureWorker90((CGameObject*)found) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Sub151b90 (0x151b90): cache the linked object (m_98) resolved from the
// serialized key handle (m_184) through the manager's kill-cue map
// (m_mgr->m_8->m_map48, the real CMapPtrToPtr::Lookup @0x1b8760). Gated on a non-null
// caller arg; a null key or a lookup miss clears m_98. __thiscall, ret 4.
// @early-stop
// Logic complete + verified (74.6%). Residual is a pure MSVC5 block-layout tiebreak:
// retail lays the lookup-MISS block inline (fall-through of `jne`, reusing the tested
// eax=0 for `m_98 = 0`) with the FOUND block out-of-line, and sinks the `found = 0`
// init store to just after the two arg pushes. cl unconditionally lays the then-block
// (found) inline (`je`) and hoists the init store; every source polarity (==0 / !=0 /
// if-else / temp-hit) canonicalizes to the same found-inline shape (permuter: no
// change). Not source-steerable.
// ---------------------------------------------------------------------------
RVA(0x00151b90, 0x70)
i32 CWwdGameObject::Sub151b90(i32 gate) {
    if (gate == 0) {
        return 0;
    }
    if (m_184 != 0) {
        void* found = 0;
        if (m_mgr->m_8->m_map48.Lookup((void*)m_184, found) == 0) {
            m_98 = 0;
            return 1;
        }
        m_98 = found;
        return 1;
    }
    m_98 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// WriteSnapshot (0x151c00): assemble a 0xa0-byte record from this + the worker
// and emit it through the archive at +0x30.
// ---------------------------------------------------------------------------
// @early-stop
// ~96% reloc/scheduling plateau: the two externals (Build/Dtor) reloc-mask
// against differently-named symbols (entropy tail) and a couple of record-field
// stores schedule one slot off retail. Logic complete; not steerable.
// Two __thiscall params (ret 8): dst = the archive (used), the 2nd is unused (retail
// never reads [esp+0xb4]); modeling both fixes the epilogue ret operand.
RVA(0x00151c00, 0x118)
i32 CWwdGameObject::WriteSnapshot(i32 dst, i32 unused) {
    CSerialArchive* ar = (CSerialArchive*)dst;
    if (ar == 0) {
        return 0;
    }
    AnimWorkerObj* w = m_worker;
    if (w == 0) {
        return 0;
    }
    if (w->m_1c == 0) {
        w->m_collideNotify((CGameObject*)this);
    }

    i32 ebx = 0;
    if (this->GetTypeId() == 0x1c) {
        ebx = this->Vfunc40();
    }

    w = m_worker;
    i32 edi = 0;
    if (w->m_18 != 0) {
        edi = w->m_18->GetTypeTag();
    }

    WwdSnapshot rec;
    rec.m_00 = m_04;
    rec.m_08 = this->GetTypeId();
    rec.m_04 = m_188;
    rec.m_94 = m_posX;
    rec.m_98 = m_posY;
    rec.m_9c = m_sortKey;
    rec.m_0c = ebx;
    rec.m_10 = edi;

    {
        CString str = m_mgr->m_14->FindKeyOfValue_165360((CImageSet*)m_worker);
        strcpy(rec.m_name, str);
    }
    ar->Write(&rec, 0xa0);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x151d20 - notify a hooked callback (placeholder-identity object RVA-adjacent to
// this TU): stash/replace m_7c->m_1c with arg, invoke the +0x10 callback(this),
// restore m_1c if unchanged. __thiscall, 1 arg. The +0x7c hook-owner class carries no
// recoverable edge, so the B_151d20/Cb151d20 views (in BoundaryUpperViews.h, already
// included above) stand in. Re-homed from src/Stub/BoundaryUpper.cpp.
RVA(0x00151d20, 0x3a)
i32 B_151d20::Notify(void* arg) {
    Cb151d20* p = m_7c;
    if (!p) {
        return 0;
    }
    void* saved = p->m_1c;
    p->m_1c = arg;
    m_7c->fn(this);
    if (m_7c->m_1c == arg) {
        m_7c->m_1c = saved;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ~CLogicRecord (0x151da0, __thiscall, /GX). Stamp the derived vptr, free the
// owned heap block (m_14), tear down the polymorphic sub-record (m_18) via its
// virtual slot-0 destructor, zero the live fields, then restamp the base vptr.
// (Identity: this record class IS the 0x17c AnimWorkerObj under a second view -
// see <DDrawMgr/AnimWorkerObj.h>.)
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// the body (derived vptr stamp, m_14 free, m_18->vtbl[0](1), field zeroing, base
// vptr restamp) is byte-exact, but the retail /GX frame (push -1 / fs:0 / trylevel)
// comes from a non-trivial CObject base subobject the manual-vptr non-polymorphic
// model can't emit. Defer to the final sweep once the base + full vtable are modeled.
RVA(0x00151da0, 0x80)
CLogicRecord::~CLogicRecord() {
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    m_10 = 0;
    if (m_14) {
        ::operator delete(m_14);
        m_14 = 0;
        m_178 = 0;
    }
    if (m_18) {
        m_18->Destroy(1);
        m_18 = 0;
    }
    m_170 = 0;
    m_08 = 0;
    m_0c = 0;
    m_04 = -1;
    // base-subobject vptr restore is compiler-managed via the CObject base
}

// ---------------------------------------------------------------------------
// CLogicRecord::Init (0x151e20, __thiscall). Bind the fire callback (m_10) and
// the frame stamp (m_08), zeroing the working fields. Returns 0 if callback null.
RVA(0x00151e20, 0x46)
i32 CLogicRecord::Init(void(__cdecl* callback)(void*), i32 frame) {
    if (callback == 0) {
        return 0;
    }
    m_10 = callback;
    m_08 = frame;
    m_14 = 0;
    m_18 = 0;
    m_serial[(0x20 - 0x20) / 4] = 0; // m_20
    m_serial[(0x24 - 0x20) / 4] = 0; // m_24
    m_serial[(0x2c - 0x20) / 4] = 0; // m_2c
    m_serial[(0x34 - 0x20) / 4] = 0; // m_34
    m_serial[(0x30 - 0x20) / 4] = 0; // m_30
    m_serial[(0x38 - 0x20) / 4] = 0; // m_38
    m_168 = 0;
    m_16c = 0;
    m_serial[(0x28 - 0x20) / 4] = 0; // m_28
    return 1;
}

// ---------------------------------------------------------------------------
// AnimWorkerObj::Clear (0x151e70, vtable slot 7): reset the worker - zero m_10,
// release the owned +0x14 buffer (+ its m_178 size), scalar-delete the +0x18
// sub-object (slot 0, arg 1), zero m_170.
// ---------------------------------------------------------------------------
RVA(0x00151e70, 0x3b)
void AnimWorkerObj::Clear() {
    m_collideNotify = 0;
    if (m_14) {
        ::operator delete(m_14);
        m_14 = 0;
        m_178 = 0;
    }
    if (m_18) {
        delete m_18; // the CUserBase virtual scalar dtor at slot 0 (push 1; call [eax])
        m_18 = 0;
    }
    m_170 = 0;
}

// ===========================================================================
// 0x151eb0 - CDDrawWorker::DeleteAll: delete every owned element via its
// scalar-deleting dtor (vtbl slot 1, arg 1), RemoveAll the array, then seed the
// +0x64 cached-index sentinel (99999) and clear +0x68. Plain /O2 leaf.
// ===========================================================================
RVA(0x00151eb0, 0x43)
void CDDrawWorker::DeleteAll() {
    for (i32 i = 0; i < m_items.GetSize(); i++) {
        CWorkerElement* el = (CWorkerElement*)m_items.GetAt(i);
        if (el != 0) {
            el->Delete(1);
        }
    }
    m_items.SetSize(0, -1); // CObArray::SetSize @0x1b5653 - the m_items
    // vtable (0x1ed494) CRuntimeClass-names it CObArray; the old (CDWordArray*) cast bound
    // this reloc to the WRONG library symbol (CDWordArray lives at [0x1b4b43,0x1b4f0b)).
    m_64 = 99999;
    m_68 = 0;
}

// ===========================================================================
// CSprite::InsertFrame @0x151f00 - build and install a frame worker (a CImage,
// vftable @0x5eaa2c) at frame number `n` in the sprite's +0x10 frame CObArray.
// __thiscall, ret 0xc.
// @early-stop
// vptr-scheduler wall (99.5%): the real `new CImage(n, m_c)` ctor
// (docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md) fixed the regalloc that
// used to cap this at ~84% (the this/n/worker coloring resolved once the construction
// became a clean ctor). The only residual is the vptr store position (cl 1st vs retail
// 4th) - same wall as CreateFrame30.
RVA(0x00151f00, 0xa4)
CImage* CSprite::InsertFrame(void* src, i32 n, i32 mode) {
    if (n < m_frames.m_nSize && m_frames.m_pData[n] != 0) {
        return 0;
    }
    // Two casts SURVIVE here, and they are honest: they are telling us two types above them
    // are still fake, NOT something to force away.
    //   (CImageParent*)m_c - CSprite::m_c is still `void* m_c` (Sprite.h). Type that member
    //                        and this falls out.
    //   (CParseSource*)src - InsertFrame's `void* src` is a VIRTUAL-SLOT signature shared
    //                        with CDDrawWorker::InsertFrame (slot 14, the SAME rva 0x151f00 -
    //                        worth a look on its own), so retyping it ripples through that
    //                        vtable. Deferred, not bodged.
    CImage* worker = new CImage(n, m_c); // real frame ctor (vptr interleaved)
    if (!worker->Resolve((CParseSource*)src, mode)) { // slot 11 @+0x2c  CImage::Resolve
        if (worker) {
            delete worker; // slot 1 @+0x04  scalar-deleting dtor
        }
        return 0;
    }
    ((CObArray*)&m_frames)->SetAtGrow(n, (CObject*)worker);
    if (n < m_firstFrame) {
        m_firstFrame = n;
    }
    if (n > m_lastFrame) {
        m_lastFrame = n;
    }
    return worker;
}

// CImageSet::CreateFrame30 (__thiscall, ret 0xc). Refuse if a frame already
// occupies `index`; else allocate a CImage frame, run its loader virtual at slot
// +0x30, insert it (SetAtGrow at `index`) and widen the populated index range.
// The (CImageFrameDesc*) casts on a0 are honest: CreateFrame*'s `i32 a0` params are
// still the fake type (the callers hand in a descriptor pointer as an int).
// @early-stop
// vptr-scheduler wall (99.5%): the real `new CImage(index, m_owner)` ctor
// (docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md) fixed the whole regalloc
// (was ~66% with a spelled-out new+store or a helper call). The ONLY residual is the
// vptr store position: cl stamps `mov [nf],??_7CImage` at ctor entry (1st store) while
// retail schedules it 4th - after m_status/m_08/m_parent, before m_width. The scheduler
// won't sink the vptr past scalar member stores from any source form tried (body-order,
// member-init-list); a source-level fix would need the 3 leading fields to come from a
// base-class ctor. (The other diff, `[eax+edi*4]` vs `[eax+4*edi]`, is a byte-neutral
// disasm-spelling artifact.)
RVA(0x00151fb0, 0xa4)
CImage* CImageSet::CreateFrame30(i32 a0, i32 index, i32 a2) {
    if (index < m_count && m_frames[index] != 0) {
        return 0;
    }

    CImage* nf = new CImage(index, m_owner); // real frame ctor (vptr interleaved)

    if (nf->Create((CImageFrameDesc*)a0, a2) == 0) { // slot 12 @+0x30  CImage::Create
        if (nf != 0) {
            delete nf; // slot 1 @+0x04  scalar-deleting dtor
        }
        return 0;
    }

    ((CObArray*)&m_array)->SetAtGrow(index, (CObject*)nf);
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

// CreateFrame28 (__thiscall, ret 0x10). As CreateFrame30, but the loader
// virtual is at slot +0x28 and takes (a0, a1, a3, 1).
// @early-stop
// Same vptr-scheduler wall as CreateFrame30 (see there). 99.5%.
RVA(0x00152060, 0xab)
CImage* CImageSet::CreateFrame28(i32 a0, i32 a1, i32 index, i32 a3) {
    if (index < m_count && m_frames[index] != 0) {
        return 0;
    }

    CImage* nf = new CImage(index, m_owner); // real frame ctor (vptr interleaved)

    // slot 10 @+0x28  CImage::LoadDispatch
    if (nf->LoadDispatch((CImageFrameDesc*)a0, (u32)a1, (void*)a3, 1) == 0) {
        if (nf != 0) {
            delete nf; // slot 1 @+0x04  scalar-deleting dtor
        }
        return 0;
    }

    ((CObArray*)&m_array)->SetAtGrow(index, (CObject*)nf);
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

// CreateFrame24 (__thiscall, ret 0x10). As CreateFrame30, but the loader
// virtual is at slot +0x24 and takes (a0, a1, a3).
// @early-stop
// Same vptr-scheduler wall as CreateFrame30 (see there). 99.5%.
RVA(0x00152110, 0xa9)
CImage* CImageSet::CreateFrame24(i32 a0, i32 a1, i32 index, i32 a3) {
    if (index < m_count && m_frames[index] != 0) {
        return 0;
    }

    CImage* nf = new CImage(index, m_owner); // real frame ctor (vptr interleaved)

    if (nf->Create24((CImageFrameDesc*)a0, a1, a3) == 0) { // slot 9 @+0x24  CImage::Create24
        if (nf != 0) {
            delete nf; // slot 1 @+0x04  scalar-deleting dtor
        }
        return 0;
    }

    ((CObArray*)&m_array)->SetAtGrow(index, (CObject*)nf);
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

// ===========================================================================
// 0x1521c0: store `elem` at frame index `index` (CObArray::SetAtGrow) and widen the
// cached sentinel window [m_64, m_68] to include it. __thiscall, 2 args (ret 8).
// ===========================================================================
RVA(0x001521c0, 0x2b)
void CDDrawWorker::AddFrameAt_1521c0(void* elem, i32 index) {
    m_items.SetAtGrow(index, (CObject*)elem); // CObArray::SetAtGrow @0x1b5822
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
    i32 n = m_items.GetSize();
    if (n > 0) {
        i32 cnt;
        cnt = 0;
        for (i32 i = 0; i < n; i++) {
            CWorkerElement* el;
            if (i >= m_64 && i <= m_68) {
                el = (CWorkerElement*)m_items.GetAt(i);
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
        el = (CWorkerElement*)m_items.GetAt(n);
    } else {
        el = 0;
    }
    if (el == 0) {
        return 0;
    }
    return el->Query34(rec, flag) != 0;
}

// CImageSet::GetMemoryUsage (__thiscall, ret 4). Walk every populated frame in
// [m_minIndex, m_maxIndex] (the inlined bounds-checked GetAt) and accumulate its
// decoded byte size: width*height, doubled for a 16bpp held surface or tripled for
// 24bpp, overridden by the owned object's exact count when one is present, plus a
// fixed 0x34-byte per-frame overhead when `raw` is 0.
// @early-stop
// 99.96% - every instruction byte-identical except the commutative `width*height` imul:
// retail keeps m_height in esi and reads m_width as the imul memory operand; cl canonicalizes
// to the reverse (keeps m_width, reads m_height) for EVERY spelling (a*b, b*a, temp + *=). A
// 2-byte (displacement) instruction-selection canonicalization, not source-steerable.
RVA(0x001523f0, 0x82)
i32 CImageSet::GetMemoryUsage(i32 raw) {
    i32 sum = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame) {
            i32 size = frame->m_height * frame->m_width;
            if (frame->m_surface && frame->m_surface->m_bitDepth == 0x10) {
                size += size;
            }
            if (frame->m_surface && frame->m_surface->m_bitDepth == 0x18) {
                size = size * 3;
            }
            if (frame->m_owned) {
                size = frame->m_owned->m_rleLen; // the owned sprite's exact decoded count
            }
            if (raw == 0) {
                size += 0x34; // == sizeof(CImage), the frame element itself
            }
            sum += size;
        }
    }
    return sum;
}

// SetAllTypes (__thiscall, ret 4). Set every populated frame's owned-sprite draw type
// (m_drawType, +0x14) via CDDrawShadeBlit::Select @0x14dd90. Returns the number touched.
// (The (ShadeSelector*) cast that used to sit here is GONE: 0x14dd90 is bound to its real
// owner now, so the owned sprite is called directly.)
RVA(0x00152480, 0x4e)
i32 CImageSet::SetAllTypes(i32 type) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->Select(type, 0);
            count++;
        }
    }
    return count;
}

// SetAllField18 (__thiscall, ret 4). Walk every populated frame in
// [m_minIndex, m_maxIndex] and write `value` into its owned sprite's light level
// (m_light, +0x18); returns the count touched. Unlike SetAllFormats there is no
// up-front null guard - the empty range simply yields count 0 (the `jg` skips
// straight to the return). CheatEclipseToggle drives it with rand() % 256, which is
// exactly what a 0..255 light level wants.
RVA(0x001524d0, 0x41)
i32 CImageSet::SetAllField18(i32 value) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->m_light = value;
            count++;
        }
    }
    return count;
}

// SetAllFormats (__thiscall, ret 4). Point every populated frame's owned sprite at the
// shade/palette descriptor `format`. Returns the number of frames touched.
// The (ShadeDescr*) cast is honest: the `i32 format` param is still the fake type (every
// caller casts a descriptor POINTER into it) - see the @fake-param note in ImageSet.h.
RVA(0x00152520, 0x4b)
i32 CImageSet::SetAllFormats(i32 format) {
    if (!format) {
        return 0;
    }
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->m_palDescr = (ShadeDescr*)format;
            count++;
        }
    }
    return count;
}

// GetFirstFrameState (__thiscall, ret 0). Read the draw type (+0x14) of the
// lowest-indexed frame's owned sprite; returns 1 when that frame or its sprite is null.
RVA(0x00152570, 0x24)
i32 CImageSet::GetFirstFrameState() {
    CImage* frame = m_frames[m_minIndex];
    if (frame == 0) {
        return 1;
    }
    CDDrawShadeBlit* fmt = frame->m_owned;
    if (fmt == 0) {
        return 1;
    }
    return fmt->m_drawType;
}

// FindFrame (__thiscall, ret 0xc). Returns 1 on a hit, 0 otherwise.
RVA(0x001525c0, 0x76)
i32 CImageSet::FindFrame(CImage* frame, char* outName, i32* outIndex) {
    if (frame) {
        for (i32 i = 0; i < m_count; i++) {
            CImage* cur = m_frames[i];
            if (cur && cur == frame) {
                if (outName) {
                    strcpy(outName, m_name);
                }
                if (outIndex) {
                    *outIndex = i;
                }
                return 1;
            }
        }
    }
    return 0;
}

// g_logicTypesRegistered (RVA 0x2bf674, VA 0x6bf674): the one-shot logic-type guard.
DATA(0x002bf674)
i32 g_logicTypesRegistered;

// class-metadata sweep (SIZE_UNKNOWN = retail size TBD, at .cpp EOF).
SIZE_UNKNOWN(WwdMgr);
SIZE_UNKNOWN(WwdSnapshot);
SIZE_UNKNOWN(CObject);
SIZE_UNKNOWN(CDDrawWorker);
SIZE_UNKNOWN(CWorkerElement);
