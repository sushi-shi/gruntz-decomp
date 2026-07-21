#ifndef GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
#define GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

#include <Ints.h>
#include <Gruntz/ResolveNode.h> // CResolveNode : CLoadable - the worker base
#include <rva.h>

class CDDrawSurfaceMgr; // the +0x0c owner (the ex-CDDrawWorkerCtx view)

class CDDrawWorker; // the frame-source (ex CDDrawFrameSource view)

class CDDrawSurfacePair;

// VTBL_ABSENT: the base vtable is never emitted - every factory builds a derived
// worker with a SINGLE derived stamp (retail 0x157150; the NO_SEED base ctor is
// inline-empty so the intermediate stamp dies), and no standalone base ctor/dtor
// exists. The family-shared slot bodies (0x157200/...) are ITS methods, dispatched
// only through the A/B vtables.
VTBL_ABSENT(CDDrawWorkerBase);
class CDDrawWorkerBase : public CResolveNode {
public:
    virtual i32 IsLoaded() OVERRIDE;   // [5] 0x157200 (family-shared body)
    virtual i32 Unload() OVERRIDE;     // [7] 0x157310 (family-shared body)
    virtual i32 GetClassId() OVERRIDE; // [8] 0x157210 (family-shared body)
    // [9] 0x157080: the worker re-arm override; the Vfunc* bodies call the BASE
    // 0x164790 directly (qualified CResolveNode::SetPosition - retail's rel32).
    virtual i32 SetPosition(i32 x, i32 y) OVERRIDE; // [9] 0x157080 (family-shared body)
    // [10] the per-frame render onto the two surface pairs (A: 0x165fa0 marker
    // plot, B: 0x1660b0 frame-node blit) - the slot PruneWorkers dispatches per
    // element. Declared here so the list dispatches through the base.
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b);

    // (+0x68..+0x73 is the base node's m_clip tail - no pad needed: the node ends at +0x74)
    i32 m_refCount;           // +0x74  frames-remaining count (Vfunc* re-arm to 2)
    // +0x78 the frame slot - a BASE field (the shared slot bodies read/clear it as a
    // dword: IsLoaded @0x157200 `[ecx+0x78]!=0`, Unload @0x157310 `[ecx+0x78]=0`),
    // overlaid per-kind: A stores its BYTE frame (factory seed `mov byte [eax+0x78],bl`
    // @0x157012), B its int frame/node pointer (dword seed @0x157192).
    union {
        i32 m_78;          // the dword reading (the base predicates test/clear it)
        class CImage* m_frame; // B's frame image (RenderFrame dispatches its slot 14)
        char m_78b;        // A's byte frame
    };

    CDDrawWorkerBase() {}
    // The worker-seed ctor: CDDrawWorkerList's CreateWorkerA/B* factories all build a
    // worker with this SAME 9-field base seed (m_0c = the ctx handle, the rest
    // constants - the CResolveNode-default seed set with m_04/m_flags zeroed). The
    // derived CDDrawWorkerA/B ctors delegate here and add m_78. cl emits the base
    // seed, then the DERIVED vptr, then m_78 - matching retail's store order + single
    // vptr stamp; see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md.
    CDDrawWorkerBase(CDDrawSurfaceMgr* ctx) : CResolveNode(NO_SEED) {
        m_04 = 0;
        m_0c = reinterpret_cast<i32>(ctx); // the CLoadable-family int owner handle
        m_flags = 0;
        m_dirtyRect.left = static_cast<i32>(0x80000000);
        m_dirtyArmed = -1;
        m_screenX = static_cast<i32>(0x80000000);
        m_clip.left = static_cast<i32>(0x80000000);
        m_level = 0;
        m_stateFlags = 0;
    }
};
SIZE(CDDrawWorkerBase, 0x7c);

struct CDDrawWorkerA : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerA() OVERRIDE; // slot 1 (compiler ??_G @0x1570b0; ~ @0x1570d0)
    // [10] 0x165fa0: plot the marker pixel (m_78) at (m_5c,m_60) onto both pairs.
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    CDDrawWorkerA() {}
    CDDrawWorkerA(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_78b = 0; // the BYTE frame seed (retail `mov byte [eax+0x78],bl`)
    }
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3); // [11] 0x157110
    // +0x78 byte frame = the base union's m_78b.
};
SIZE(CDDrawWorkerA, 0x7c);
VTBL(CDDrawWorkerA, 0x001efea0); // vtable_names -> code (RTTI game class)

struct CDDrawWorkerB : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerB() OVERRIDE; // slot 1 (compiler ??_G @0x157220; ~ @0x157240)
    // [10] 0x1660b0: draw the current frame node (m_78) onto the two surface-pair
    // targets (unconditional first, gated second on m_2c live + not flagged 0x20000).
    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    CDDrawWorkerB() {}
    CDDrawWorkerB(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_78 = 0;
    }
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a3);                    // [11] 0x1572f0
    virtual i32 PlaceFrame(i32 a1, i32 a2, CDDrawWorker* src, i32 a4); // [12] 0x1572b0
    virtual i32 PlaceBound(i32 a1, i32 a2, i32 a3, i32 a4);            // [13] 0x157280

    // Non-virtual: look up a named object in the owner map, fetch element[idx] when
    // in range, cache at m_78, return whether it is non-null.
    i32 Helper(i32 key, i32 idx); // 0x166040
    // +0x78 int frame/node = the base union's m_78.
};
SIZE(CDDrawWorkerB, 0x7c);

VTBL(CDDrawWorkerB, 0x001efed0);

#endif // GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
