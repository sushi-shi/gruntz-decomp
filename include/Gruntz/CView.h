// CView.h - the ONE shared shape of the CState +0x0c view/render/resource context.
//
// CState::m_c is the real RTTI class `CView` (vftable ??_7CView@@6B@ @0x5ee1c4, RVA
// 0x1ee1c4). It is a SINGLE object that every game-state reaches through one of
// several facets, each of which used to be a bespoke per-TU reinterpret view:
//   * CPlay::Render (this TU-family) walks the RENDER facet: the renderer-state
//     object (+0x04), renderer A/B (+0x08/+0x0c), the frame-grid lookup (+0x10),
//     the frame profiler (+0x20) and the draw surface (+0x24).
//   * the leaf states' ReleaseResources/dispose walk the RESOURCE facet: the
//     image/name registry (+0x10 Release/Register/Has/Install), the sound registry
//     (+0x28, + its pooled resource at +0x2c) and the anim registry (+0x2c), plus a
//     render/flip view (+0x04 Flush + m_10->m_2c Flip) and a worker list (+0x0c).
// These are ONE object: the +0x10 image registry's embedded name->object map IS the
// frame-grid lookup, the +0x0c renderer-B IS the worker holder, etc. The two facets
// touch the same offsets with different (compatible) method sets, so they are folded
// here into one class - additive method/member decls never move an offset, so the
// render facet stays byte-identical while the resource facet becomes cast-free.
//
// This header is pulled by the MFC state TUs (CPlay.h, GameMode.h) AFTER <Mfc.h>;
// CState.h keeps only a forward decl so the ~60 pure-Win32 TUs stay afx-neutral.
#ifndef GRUNTZ_GRUNTZ_CVIEW_H
#define GRUNTZ_GRUNTZ_CVIEW_H

#include <Mfc.h> // RECT (CDrawSurface::SetClipRect / m_viewport)
#include <rva.h>

// The per-frame input object the credits poll reaches (m_4->m_10->m_2c->m_8); its
// full vtable layout lives in GameMode.h (the only TU that polls it).
struct CGMInputObj;

// The placed-object display list the warlord-sprite loader walks (hung off
// renderer A at +0x10; see CRenderer::m_10). Each node's +0x8 is a placed object.
struct CWarlordListNode; // fully defined in CPlay.cpp
SIZE_UNKNOWN(CWarlordListHead);
struct CWarlordListHead {
    char p0[0x4];
    CWarlordListNode* m_4; // +0x04  first node
};

// The renderer/draw object (m_c->m_8 = renderer A, m_c->m_c = renderer B). Its
// vtable carries the per-frame draw slots: +0x24 begin-scene (1 arg), +0x34
// present (2 args). Modeled with a padded virtual interface so the indirect
// `call [vtbl+0x24]` / `[vtbl+0x34]` shapes fall out.
struct CRenderer {
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void BeginScene(i32 z); // slot 9  (+0x24)
    virtual void s0a();
    virtual void s0b();
    virtual void s0c();
    virtual void Present(void* a, void* b); // slot 13 (+0x34)
    // Non-virtual leaf the play-exit path runs on renderer A (reloc-masked).
    void Refresh(); // 0x159ef0 (thiscall, no arg)
    // Renderer B (+0x0c) is also the resource-facet worker holder the leaf-state
    // dispose path tears down (CMenuState::ReleaseResources); reloc-masked leaf.
    void DisposeWorkers(); // 0x163c60 (thiscall, no arg)
    // Renderer A owns the placed-object display list at +0x10 (LoadWarlordSprites
    // walks it in-level; vptr is at +0x00, so data starts at +0x04).
    char p04[0x10 - 0x4];
    CWarlordListHead m_10; // +0x10  placed-object list head
};

// The draw-surface object at m_c->m_24 (the target of the thiscall PushView +
// the ViewPreStep/PostStep sub-steps). Its +0x5c holds the camera geometry the
// world blit reads (+0x84 / +0x88).
struct CDrawSurface {
    void PushView(void* view, void* renderer);
    void PreStep();
    void PostStep();
    void SetClipRect(RECT* r); // 0x15da80 (thiscall) ClampViewport apply-tail
    char p0[0x10];
    // +0x10: the viewport rect {left,top,right,bottom}; StepScroll reads .left/.top
    // as the scroll origin, DispatchHudClick reads all four as the bounds box.
    RECT m_viewport; // +0x10  viewport rect (also the scroll origin .left/.top)
    char p20[0x5c - 0x20];
    // +0x5c -> a geom block: StepScroll reads (m_5c+0x40).{m_originX,m_originY};
    // the world blit reads (m_5c).{m_84,m_88}.
    struct CameraGeom {
        void DrawA(); // 0x563300  per-frame world-draw sub-step A
        void DrawB(); // 0x563370  per-frame world-draw sub-step B
        char p0[0x40];
        i32 m_originX; // +0x40
        i32 m_originY; // +0x44
        char p48[0x84 - 0x48];
        i32 m_84;
        i32 m_88;
    }* m_5c; // +0x5c camera geom
};

// The pooled resource the leaf states free before releasing their namespaces
// (m_c->m_28->m_2c). Reloc-masked __thiscall.
SIZE_UNKNOWN(CViewPooledRes);
struct CViewPooledRes {
    void Free();          // FUN_00137a80, no-arg
    void TickAnim(i32 z); // FUN_00136e20, ret 4
};

// m_c (the real CView: render + resource context).
struct CView {
    char p0[0x4];
    struct RenderState { // +0x04  the renderer-state object / render-flip view
        void Flush();    // FUN_00558ee0 (resource-facet flip prep, no-arg)
        char p0[0x10];
        struct SurfaceA {
            char p0[0x2c];
            // +0x2c the frame surface: passed by-ptr to the engine flush/begin-scene
            // (CPlay casts it to its CProfFlush view), flipped by the slot-10 poll and
            // (credits) drawn to; its +0x08 holds the per-frame input object.
            struct Surface2c {
                void Flip(i32 z); // FUN_0013e850 (resource-facet flip, ret 4)
                void Draw(i32 z); // credits draw-target draw (thiscall)
                char p0[0x8];
                CGMInputObj* m_8; // +0x08  input obj (credits input poll source)
            }* m_2c;
        }* m_10; // +0x10 -> +0x2c surface
        struct SurfaceB {
            void Blit(i32 arg); // credits blit-target blit (thiscall)
            char p0[0x2c];
            struct Held {
                void Prepare(i32 z);      // 0x13e760 (thiscall) ClampViewport apply-tail
                void NotifyClip(RECT* r); // 0x13e7d0 (thiscall) NotifyVisibleEntities
            }* m_2c;
        }* m_14;    // +0x14 -> +0x2c draw surface (view obj)
        void* m_18; // +0x18  the present target
    }* m_4;
    CRenderer* m_8; // +0x08  renderer A
    CRenderer* m_c; // +0x0c  renderer B (present) / resource worker holder
    // +0x10 -> the image/name registry (polymorphic; vptr @+0x00). BeginGridWalk looks
    // up the frame grid in its embedded name->object map (+0x10); the leaf states
    // Release named namespaces (non-virtual FUN_00155360) and Install resolved sets
    // through vtable slot 18 (+0x48). Declared-only virtuals -> cl emits no ??_7 (the
    // object is only ever reached by pointer) yet dispatches through the real vtable.
    struct CImageRegistry {
        void
        Release(const char* szName, const char* szKey); // FUN_00155360 (namespace register/release)
        void Register(const char* szName, const char* szKey); // FUN_00155360 (loader alias)
        i32 Has(const char* szName);                          // FUN_00155550 -> found
        virtual void v00();
        virtual void v01();
        virtual void v02();
        virtual void v03();
        virtual void v04();
        virtual void v05();
        virtual void v06();
        virtual void v07();
        virtual void v08();
        virtual void v09();
        virtual void v10();
        virtual void v11();
        virtual void v12();
        virtual void v13();
        virtual void v14();
        virtual void v15();
        virtual void v16();
        virtual void v17();
        // slot 18 (+0x48): install a resolved set under a name; the 3rd arg is a
        // namespace key ("_") for the level loaders and a byte-flag out-ptr for the
        // effect-sprite install (that site casts its unsigned char*).
        virtual void Install(void* set, const char* szName, const char* szKey);
        char p04[0x10 - 0x4]; // after the vptr (+0x00)
        struct CMap {
            void Lookup(i32 key, void*& out); // 0x1b8008 (thiscall)
        } m_10;                               // +0x10  the name->object map / frame grid
    }* m_10;                                  // +0x10
    char p14[0x20 - 0x14];
    void* m_20;         // +0x20  a frame profiler timer (timeGetTime x2)
    CDrawSurface* m_24; // +0x24  the draw-surface (PushView / Pre/PostStep)
    // Resource facet (offsets the render facet does not touch):
    struct SoundRegistry {                                    // +0x28
        void Release(const char* szName, const char* szKey);  // FUN_00157c70
        void Register(const char* szName, const char* szKey); // FUN_00157c70 (loader alias)
        i32 Has(const char* szName);                          // FUN_001583c0 -> found
        void Install(void* set, const char* szName, const char* szKey); // FUN_00157ee0
        char p0[0x2c];
        CViewPooledRes* m_2c; // +0x2c  pooled resource (Free() if set)
    }* m_28;                  // +0x28  sound registry + pooled resource
    struct AnimRegistry {     // +0x2c
        void Release(const char* szName, const char* szKey); // FUN_00152720 (credits reg)
        i32 Has(const char* szName);                         // FUN_00152c50 -> found
        void Install(void* set, const char* szName, const char* szKey); // FUN_00152ad0
    }* m_2c; // +0x2c  anim/third registry
};

#endif // GRUNTZ_GRUNTZ_CVIEW_H
