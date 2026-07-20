// WwdFactoryObject.cpp - the 0x15b2c0-0x15ccc8 original TU (wave4-L dossier #15,
// block I): the wwd object-lifecycle obj - the base-object ctors (CResolveNode
// 3-arg / AnimWorkerObj 3-arg / CWwdGameObjBaseCtor / CAniAdvanceCursor), the five
// ??1CWwdGameObject[A-F] /GX destructors, the CWwdFactoryObject Release/Reset
// pass + Notify, CDDrawBlitParam (init/setup/serialize), the animation Advance
// cursor + its Clamp pair, and the Init/SetupDeferred/SetupFlagged out-of-lines.
// Held at the dossier-#9 boundary 4 (0x15b2c0); a correct partial - it may yet
// merge with block H (weave across 0x15b2c0 is limited to the COMDAT-able factory
// ctor 0x15b390).
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor).
#include <Mfc.h>
#include <Image/CImage.h> // complete CImage: the CObArray-element downcasts are static (CImage : CWapObj : CObject)
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <rva.h>
#include <Ints.h>
#include <string.h>                  // strcpy/strlen (blit-param label buffer)
#include <Wwd/WwdGameObjectFamily.h> // the CGameObject/A/F/B/C dtor-family hierarchy
#include <Gruntz/WwdGameObject.h>    // canonical CWwdGameObject (Init/Setup* out-of-lines)
#include <Gruntz/Sprite.h>           // CSprite (GetFrame @0x15cc30 + the Clamp pair)
#include <Gruntz/ResolveNode.h>      // canonical CResolveNode (3-arg ctor @0x15b2c0)
#include <DDrawMgr/AnimWorkerObj.h>  // AnimWorkerObj (the 0x17c worker; 3-arg ctor @0x15b300)
#include <Gruntz/AniAdvanceCursor.h> // canonical CAniAdvanceCursor (ctor/dtor/Advance)
#include <DDrawMgr/AniAdvance.h>     // CAniRenderCtx/CAniDesc/CAniBlitTrigger satellites
#include <Gruntz/AniElement.h>       // CAniElement (the descriptor playlist full def)
#include <Gruntz/SerialArchive.h>    // the shared CSerialArchive stream (Read/Write)
#include <Wwd/WwdFactoryObject.h>    // CWwdFactoryObject/CDDrawRect
#include <Wwd/WwdGameObjCtor.h>      // WwdCtorBase/CWwdGameObjBaseCtor/WwdAnimWorker
#include <Gruntz/LeafCue.h>          // LeafCue (PlayIfElapsed - Advance's sound cue)
#include <Globals.h>

// The engine RNG @0x15cbe0 is the free __cdecl Rng::Next2 (its body stays in
// Random.cpp - a foreign inline-COMDAT exile hole in this obj's span).
namespace Rng {
    i32 Next2();
}

// g_sndEnabled (0x61ab20) is declared in the included <Globals.h> - no per-TU extern.
// g_sndPanScale (0x5eff2c) + g_aniCueItem (0x61ab24, == <Globals.h>'s g_sndCueTag - same
// address, name conflict) still need homing to Globals.h (deferred globals-consolidation).
extern float g_sndPanScale; // 0x5eff2c
extern i32 g_aniCueItem;    // 0x61ab24 (== g_sndCueTag)

// onto the real CAniElement (<Gruntz/AniElement.h>): its "+0x0c elements/+0x10
// count" are m_records.m_pData/m_nSize and its +0x20 the same float m_scale.)
#include <Gruntz/AniElement.h>

// The +0x2c label sub-object IS the real CDDrawSubMgrLeaf (<DDrawMgr/DDrawSubMgrLeaf.h>):
// its GetLabel method is PROVEN to be ?KeyOfValue_152d30@CDDrawSubMgrLeaf@@ (the 0x152d30
// mangled name, returns CString from a CObject* value), and its +0x10 label map is that
// class's own CMapStringToPtr m_10 (Deserialize_15ca70's Lookup is `call 0x1b8438` =
// CMapStringToPtr - disasm-confirmed; the ex-CDDrawBlitLabelSource view mistyped it as
// CMapStringToOb @0x1b8008, a WRONG reloc that this fold corrects).
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the +0x0c owner: the canonical CDDrawSurfaceMgr

// The manager held at the cursor's +0x0c owner slot (CLoadable::m_0c, a generic i32 owner
// handle) IS the canonical CDDrawSurfaceMgr: the +0x2c label sub-object the blit-param
// serialize reaches is CDDrawSurfaceMgr::m_animRegistry (a CDDrawSubMgrLeaf, KeyOfValue_152d30 /
// +0x10 map). The m_animRegistry@+0x2c layout is unique to the CDDrawSurfaceMgr family, and fold #1
// this wave typed CDDrawSurfaceMgr::m_animRegistry as CDDrawSubMgrLeaf*, so the former per-TU
// to the real class, exactly as WwdObjMgr / WwdGameObjectRender already do.

// ---------------------------------------------------------------------------
// 0x15b2c0 - the parameterized CResolveNode ctor (the factory base sub-object).
// @early-stop
// sentinel-seed ctor store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// identical instruction multiset, but cl floats the m_08 (edx=arg3) store and the
// m_38 (-1) store to different positions than retail; 3 field-order spellings all
// ~60%. Source steers which arg lands in edx, not the store schedule. Logic complete.
RVA(0x0015b2c0, 0x3d)
CResolveNode::CResolveNode(i32 owner, i32 field04, i32 field08) {
    m_04 = field04;
    m_flags = field08;
    m_0c = owner;
    m_20 = static_cast<i32>(0x80000000);
    m_38 = -1;
    m_screenX = static_cast<i32>(0x80000000);
    m_clip.left = static_cast<i32>(0x80000000);
    m_3c = 0;
    m_stateFlags = 0;
}

// ---------------------------------------------------------------------------
// 0x15b300 - AnimWorkerObj's out-of-line 3-arg seed constructor (the body the
// 0x150eb0 factory / CreateWorker24 inlines). The arg-store order (b,c,a into
// m_04/m_08/m_0c) is load-bearing. (Was the WorkerFull view - folded onto the
// canonical AnimWorkerObj, whose ??_7 @0x1efb80 this ctor stamps.)
// @early-stop
// vptr-last wall: retail stamps the vptr AFTER m_04/m_08/m_0c, but a real-virtual
// class forces cl's implicit vptr-first store at ctor entry. Field-store order
// preserved; only the vptr position diverges (mandate: convert anyway).
RVA(0x0015b300, 0x40)
AnimWorkerObj::AnimWorkerObj(i32 a, i32 b, i32 c) {
    m_04 = b;
    m_08 = c;
    m_0c = reinterpret_cast<CDDrawSurfaceMgr*>(a); // (mangling-pinned i32 arg; a IS the mgr)
    m_notify = 0;
    m_14 = 0;
    m_logic = 0;
    m_170 = 0;
    m_1c = 0;
    m_174 = 0;
    m_178 = 0;
}

// ---------------------------------------------------------------------------
// AnimWorkerObj::Consume (0x15b340, __thiscall; was CLogicRecord::). Draw `amount` from the
// remaining-count at m_20: returns 1 with the balance debited while it covers the
// request, else clamps to 0 and returns 0.
RVA(0x0015b340, 0x2b)
i32 AnimWorkerObj::Consume(i32 amount) {
    i32 remaining = m_20;
    if (remaining == 0) {
        return remaining; // eax already holds 0
    }
    if (static_cast<u32>(amount) >= static_cast<u32>(remaining)) {
        m_20 = 0;
        return 0;
    }
    m_20 = remaining - amount;
    return 1;
}

// ---------------------------------------------------------------------------
// CWwdGameObjBaseCtor::Construct (0x15b390) - the shared CWwdGameObject base-object
// ctor the wide-object factories call (CreateObject_1598d0/166640; also
// WwdFile::ReadPlaneObjects 0x162af0). A REAL /GX ctor: the CResolveNode base
// subobject stamps ??_7CResolveNode (0x5efbc0) + its +0x04..+0xd8 fields, then the
// CString label member (+0xdc) constructs, then the derived body final-stamps the
// wide-object vtable, `new`-allocates the +0x7c anim worker (AnimWorkerObj), and
// publishes g_wwdObjIdCounter.
// @early-stop
// eh-member-state wall (59.7%): the real /GX ctor is byte-faithful store-for-store,
// but MSVC5 declines to emit the retail member-construction EH-state machine
// (state cookies around the CString ctor + the worker op-new). Not source-steerable:
// `::operator new`+placement REGRESSED (57.2%), a declared worker dtor is neutral,
// out-of-lining the worker ctor would mismatch retail's inline stores.
// docs/patterns/eh-state-numbering-base.md + throwing-operator-new-eh-state-transition.md.
RVA(0x0015b390, 0x128)
CWwdGameObjBaseCtor::CWwdGameObjBaseCtor(int a, int b, int c) : WwdCtorBase(a, b, c) {
    // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
    m_screenX = static_cast<int>(0x80000000);
    m_78 = 0;
    m_7c = new AnimWorkerObj(a, b);
    m_98 = 0;
    m_80 = 0;
    m_88 = 0;
    m_90 = 0;
    m_188 = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}

// ---------------------------------------------------------------------------
// 0x154a50 - ~CResolveNode: disarm the live dirty-rect sentinels; ~CLoadable
// (m_04/m_08/m_0c reset) + the CObject grand-base restamp fold in. Defined in
// THIS TU so the family dtors below fold its content (retail ~E/~A/~F/~C tails);
// the ResolveNode.cpp pocket's ??_G calls it (retail 0x154a30 -> 0x154a50).
// @early-stop
// sentinel-seed store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// byte-identical EXCEPT the single immediate vptr restamp store position.
RVA(0x00154a50, 0x23)
CResolveNode::~CResolveNode() {
    m_screenX = static_cast<i32>(0x80000000);
    m_20 = static_cast<i32>(0x80000000);
    m_38 = -1;
}

// ---------------------------------------------------------------------------
// 0x15b4f0 - ~CGameObject (vtable 0x5f0020): { Unload(); } - the inline E
// release pass folds, then the CString member dtor, then the inline
// ~CResolveNode + ~CLoadable + CObject-grand-base restamp fold in (retail tail).
// @early-stop
// zero-register-pinning regalloc wall (docs/patterns/zero-register-pinning.md):
// logic + /GX trylevel chain (3->2) byte-exact, residual is the callee-saved
// zero/0x80000000/-1 register coloring (edi/ebx/ebp vs retail ebp/edi/ebx).
RVA(0x0015b4f0, 0xde)
CGameObject::~CGameObject() {
    Unload(); // devirtualized in the dtor -> the inline E pass
}

// ---------------------------------------------------------------------------
// 0x15b650: per-tick notify. When flag bit 0x8 is set, subtract `p`'s +0x120
// budget from this+0x128 and, if non-positive, latch error 0x1c on the +0x7c
// worker. Otherwise hand `p` to the +0x80 notifier's +0x10 cdecl callback (with
// the owner), after recording `p` at +0x84. __thiscall, 1 arg (ret 0x4).
// @early-stop
// 84% - structure/CFG/offsets/stores byte-exact; the residual is two instruction-
// selection coin-flips MSVC5 won't flip from source: the flag test (`movb;testb`
// vs retail `testb mem`) and the budget subtract (mem-operand vs reg-load first).
// Entropy-tail / zero-register-pinning wall.
RVA(0x0015b650, 0x4d)
void CGameObject::Notify_15b650(void* p) {
    char* o = reinterpret_cast<char*>(this);
    if (*reinterpret_cast<unsigned char*>((o + 0x8)) & 0x8) {
        i32 d = *reinterpret_cast<i32*>(o + 0x128) - *reinterpret_cast<i32*>((reinterpret_cast<char*>(p) + 0x120));
        *reinterpret_cast<i32*>((o + 0x128)) = d;
        if (d <= 0) {
            *reinterpret_cast<i32*>((*reinterpret_cast<char**>(o + 0x7c) + 0x1c)) = 0x1c;
        }
    } else {
        AnimWorkerObj* h = *reinterpret_cast<AnimWorkerObj**>((o + 0x80));
        if (h != 0) {
            *reinterpret_cast<void**>((o + 0x84)) = p;
            h->m_notify(reinterpret_cast<CGameObject*>(this));
        }
    }
}

// ---------------------------------------------------------------------------
// 0x15b6d0 - CAniAdvanceCursor::~CAniAdvanceCursor (the out-of-line base ??1): stamp the
// derived ??_7CAniAdvanceCursor (0x5f0128, bound), run the CLoadable slot-7 Unload/Reset
// (0x15c2c0; the (CDDrawBlitParam*) cast reloc-masks CAniAdvanceCursor's own Reset there),
// reset the CLoadable header (m_04/m_08/m_0c), then fold the grand base (??_7CObject
// onto the real class so the vptr stamp binds to ??_7CAniAdvanceCursor@@6B@ (was RELOC_VTBL).
RVA(0x0015b6d0, 0x5b)
CAniAdvanceCursor::~CAniAdvanceCursor() {
    Unload(); // devirtualized in the dtor -> direct call to 0x15c2c0
    m_04 = -1;
    m_flags = 0;
    m_0c = 0;
}

// cl auto-stamps the ??_7CAniAdvanceCursor vptr @+0, seeds the three CLoadable
// header fields (m_0c=owner, m_04=field04, m_08=field08) then zeroes m_10/m_14/m_18.
// Retail FUSES the base seed into this ctor (no `call 0x156cb0`), so the three
// stores are spelled here over the default base ctor - the 3-arg CLoadable ctor
// is out-of-line at 0x156cb0 (its big-caller sites call it; retail's inline copy
// here came from the same source cl chose to inline) and chaining it would
// inject a call retail does not have.
// @early-stop
// vptr-stamp position (97.6%): one 2-instruction swap - retail sinks the ??_7
// stamp BELOW the m_08 store (the chained-inline-base-ctor schedule); a plain
// ctor body stamps before it. The 100% spelling needs the base ctor inline
// (`: CLoadable(...)`), which contradicts its proven out-of-line 0x156cb0 body.
RVA(0x0015b730, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(i32 owner, i32 field04, i32 field08) {
    m_04 = field04;
    m_flags = field08;
    m_0c = owner;
    m_10 = 0;
    m_14 = 0;
    m_element = 0;
}

// ---------------------------------------------------------------------------
// 0x15b790 - the complete destructor: a thin derived class (vtable 0x5f00a8) on top
// of Mid, adding the m_18c block + the embedded WwdSubA command object at +0x1a0.
// @early-stop
// zero-register-pinning regalloc wall: three-level fold (A -> WwdSubA member ->
// Mid -> wap-object base) + trylevel chain reproduced; residual is the callee-saved
// const register coloring across the two worker passes.
RVA(0x0015b790, 0x1a6)
CWwdGameObjectA::~CWwdGameObjectA() {
    Unload(); // devirtualized -> the inline A pass (geometry cache + E release)
    // m_1a0 (CAniAdvanceCursor) member-destroys inline (the 0x5f0128 restamp),
    // then ~E folds (second worker pass + ~CString + the CResolveNode tail).
}

// ---------------------------------------------------------------------------
// Init (0x15b940): zero +0x19c, construct the +0x1a0 command map, then Setup.
// ---------------------------------------------------------------------------
RVA(0x0015b940, 0x38)
i32 CWwdGameObjectA::Setup(i32 a1, i32 a2, i32 a3, i32 a4) {
    m_19c = 0;
    m_1a0.Construct(this);
    return CGameObject::Setup(a1, a2, a3, a4);
}

// ---------------------------------------------------------------------------
// 0x15bad0 - the 0x159440-final variant: thin derived class (vtable 0x5f0060) on top
// of Mid. Re-runs the worker pass + groupX, then folds Mid + wap-object base.
// @early-stop
// zero-register-pinning regalloc wall: two-level fold + double worker pass +
// trylevel chain reproduced; residual is callee-saved const register coloring.
RVA(0x0015bad0, 0x153)
CWwdGameObjectF::~CWwdGameObjectF() {
    Unload(); // devirtualized -> the inline F pass (the E release copy)
}

// CWwdGameObject::SetupDeferred (0x15bc30): Setup with a1/a2 zeroed. Out-of-line.
RVA(0x0015bc30, 0x16)
i32 CWwdGameObjectF::SetupDeferred(i32 a3, i32 a4) {
    return CGameObject::Setup(0, 0, a3, a4);
}

// ---------------------------------------------------------------------------
// 0x15bd10 - the CResolveNode-derived variant (extra +0x1dc CObList, leading init
// call 0x166810, trailing base CResolveNode dtor 0x429b): the 4-level polymorphic
// chain CWwdGameObject : WwdBLevel2 : WwdBMid : WwdBResolve (family header).
// @early-stop
// eh-dtor multi-level trylevel wall: the real 4-level polymorphic chain reproduces
// the four cl-emitted vptr restamps + the per-phase field re-clears + the CString/
// WwdSub/CObList member folds; residual is the /GX trylevel numbering across the four
// destruct phases (the same zero-register-pinning const coloring as the A/C/F
// variants) - not source-steerable.
RVA(0x0015bd10, 0x1ef)
CWwdGameObject::~CWwdGameObject() {
    Unload(); // devirtualized -> the inline B pass (Clear_166810 + geometry + E release)
    // m_1dc (CObList) member-destroys, then ~A folds (retail: the A-phase spills
    // to `call 0x15b5d0` and the bottom keeps the 0x5efbc0 stamp + `call 0x429b`).
}

// 0x15bfb0: rect-overlap predicate (RECT a, RECT b): true iff a.left <= b.right,
// a.right >= b.left, a.top <= b.bottom, a.bottom >= b.top. __stdcall, 2 args.
RVA(0x0015bfb0, 0x4a)
i32 __stdcall RectsOverlap_15bfb0(CDDrawRect* a, CDDrawRect* b) {
    if (a->left > b->right) {
        return 0;
    }
    if (a->right < b->left) {
        return 0;
    }
    if (a->top > b->bottom) {
        return 0;
    }
    return a->bottom >= b->top;
}

// ---------------------------------------------------------------------------
// 0x15c070 - the 0x159250-final variant: thin derived class (vtable 0x5effd0) on top
// of Mid; clears the byte flag m_18c, re-runs the worker pass + groupX, then folds
// Mid + wap-object base.
// @early-stop
// zero-register-pinning regalloc wall: two-level fold + byte-flag clear + double
// worker pass + trylevel chain reproduced; residual is callee-saved const coloring.
RVA(0x0015c070, 0x159)
CWwdGameObjectC::~CWwdGameObjectC() {
    Unload(); // devirtualized -> the inline C pass (byte clear + E release)
}

// CWwdGameObject::SetupFlagged (0x15c1d0): stash the dot-color flag byte then Setup.
RVA(0x0015c1d0, 0x26)
i32 CWwdGameObjectC::SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag) {
    m_dotColor = static_cast<u8>(flag); // the C kind's own +0x18c byte - the reinterpret dies
    return CGameObject::Setup(a1, a2, a3, a4);
}

// 0x15c290: blit-param init.
// @early-stop
// 94.75% - structure/offsets/stores byte-exact; retail pins `src` in edx and the
// constant `1` in eax, our cl swaps them (eax<->edx phase shift), a regalloc
// coin-flip with no source lever (docs/patterns/zero-register-pinning.md).
RVA(0x0015c290, 0x2f)
void CAniAdvanceCursor::Construct(void* srcv) {
    // role-dependent src: the owning wide object (game path) or a worker source
    CAniElement* src = static_cast<CAniElement*>(srcv);
    m_10 = reinterpret_cast<CAniRenderCtx*>(src);
    m_28 = 1;
    m_14 = 0;
    m_scale = 1.0f;
    m_24 = 1;
    m_2c = *reinterpret_cast<i32*>((reinterpret_cast<char*>((&src->m_records.ElementAt(0))) + 0x34)) & 0x40;
}

// CAniAdvanceCursor::Unload (0x15c2c0, vtable slot 7; the ex Reset_15c2c0):
// clear the bound source refs. Returns 0 (retail xor eax,eax).
RVA(0x0015c2c0, 0xc)
i32 CAniAdvanceCursor::Unload() {
    m_10 = 0;
    m_14 = 0;
    m_element = 0;
    return 0;
}

// 0x15c2d0: blit-param setup from a worker source.
RVA(0x0015c2d0, 0x45)
void CAniAdvanceCursor::Setup_15c2d0(CAniElement* src) {
    CAniDesc* e;
    i32 v;
    m_14 = src;
    if (!src) {
        return;
    }
    m_index = 0;
    if (src->m_records.GetSize() > 0) {
        e = static_cast<CAniDesc*>(src->m_records.GetAt(0));
    } else {
        e = 0;
    }
    m_element = e;
    m_20 = 0;
    m_28 = 0;
    v = e->m_drawValue;
    m_pendingDraw = v;
    m_curDraw = v;
    {
        float f = src->m_scale;
        m_scale = f;
    }
}

// 0x15c320: recompute the blit-param from the already-stored m_srcRef source (the
// Setup twin that keeps m_srcRef, fixes the scale to 1.0f, and only clears m_20
// when `a1` is set).
RVA(0x0015c320, 0x40)
void CAniAdvanceCursor::Recompute_15c320(i32 a1) {
    CAniElement* src = m_14;
    if (src == 0) {
        return;
    }
    m_index = 0;
    CAniDesc* e;
    if (src->m_records.GetSize() > 0) {
        e = static_cast<CAniDesc*>(src->m_records.GetAt(0));
    } else {
        e = 0;
    }
    m_element = e;
    m_28 = 0;
    i32 v = e->m_drawValue;
    m_scale = 1.0f;
    m_pendingDraw = v;
    m_curDraw = v;
    if (a1 != 0) {
        m_20 = 0;
    }
}

// ---------------------------------------------------------------------------
// 0x15c360: advance the animation cursor by `elapsed` ticks. __thiscall, 1 arg
// (ret 4).
// @early-stop
// Zero-register-pinning plateau (1365 B, two jump-table switches): the body is a
// complete, logic-correct reconstruction. Byte-exact: the entry + timer-decrement
// block, both jump-table switches (both emit the retail .rdata table AND match its
// physical case-body order), the pos-mode update, the random blit/sound trigger,
// the float speed scale (fild/fmul/__ftol), the descriptor-advance variants, and
// the buffer-consuming return tail. The residual is purely the documented
// register-pinning wall: (1) switch1's increment/step cases pin the new frame
// index in a callee-saved ebx as a TWIN copy where our cl keeps it single-register
// in eax; (2) the back half re-materializes the zero in ecx vs our reuse of the
// ebp=0 pin, and the pos-mode switch swaps eax<->ecx for the switch value. Same
// values, same stores, same CFG; no source lever flips the homing. Deferred to the
// final sweep per the big-function stop rule. docs/patterns/zero-register-pinning.md
// + linked-list-walk-node-eax-rotation.md (the twin-copy idiom).
RVA(0x0015c360, 0x555)
i32 CAniAdvanceCursor::Advance(u32 elapsed) {
    if (m_14 == 0) {
        return -1;
    }

    // --- per-frame timer decrement --------------------------------------------
    if (m_20 > 0) {
        if (m_24 != 0) {
            if (elapsed >= m_20) {
                m_20 = 0;
                m_curDraw = m_pendingDraw;
            } else {
                m_20 -= elapsed;
                return m_curDraw;
            }
        } else {
            m_20 -= 1;
            return m_curDraw;
        }
    } else {
        m_curDraw = m_pendingDraw;
    }

    if (m_28 == 0) {
        CAniRenderCtx* ctx = m_10;
        CAniDesc* d = m_element;

        // --- step the active frame sequence one step (7-way on d->m_stepMode) --------
        switch (d->m_stepMode - 1) {
            case 0: { // advance + wrap-to-first on overrun
                CAniRenderCtx* c = m_10;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor + 1;
                c->m_frameCursor = idx;
                c->m_curFrame = seq->GetFrame(idx);
                if (c->m_curFrame == 0) {
                    i32 first = c->m_frameSeq->m_minIndex;
                    c->m_frameCursor = first;
                    c->m_curFrame = c->m_frameSeq->GetFrame(first);
                }
                break;
            }
            case 1: { // wrap-to-last when at first, else step back
                CAniRenderCtx* c = m_10;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor;
                if (idx == seq->m_minIndex) {
                    c->m_frameCursor = seq->m_maxIndex;
                } else {
                    c->m_frameCursor = idx - 1;
                }
                c->m_curFrame = seq->GetFrame(c->m_frameCursor);
                break;
            }
            case 2: { // jump to an explicit frame (d->m_param)
                CAniRenderCtx* c = m_10;
                i32 frame = d->m_param;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                c->m_curFrame = seq->GetFrame(frame);
                c->m_frameCursor = frame;
                break;
            }
            case 3: { // reset to first
                CAniRenderCtx* c = m_10;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 first = seq->m_minIndex;
                c->m_frameCursor = first;
                c->m_curFrame = seq->GetFrame(first);
                break;
            }
            case 4: { // reset to last
                CAniRenderCtx* c = m_10;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 last = seq->m_maxIndex;
                c->m_frameCursor = last;
                c->m_curFrame = seq->GetFrame(last);
                break;
            }
            case 5: { // advance by d->m_param, clamp-last on overrun
                CAniRenderCtx* c = m_10;
                i32 step = d->m_param;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor + step;
                c->m_frameCursor = idx;
                c->m_curFrame = seq->GetFrame(idx);
                if (c->m_curFrame == 0) {
                    c->ClampLast_15cc90();
                }
                break;
            }
            case 6: { // retreat by d->m_param, clamp-first on underrun
                CAniRenderCtx* c = m_10;
                i32 step = d->m_param;
                CSprite* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor - step;
                c->m_frameCursor = idx;
                c->m_curFrame = seq->GetFrame(idx);
                if (c->m_curFrame == 0) {
                    c->ClampFirst_15cc50();
                }
                break;
            }
            default:
                break;
        }

        // --- apply the per-frame position delta (3-way on d->m_posMode) ------------
        ctx = m_10;
        ctx->m_posModeX = 0;
        ctx->m_posModeY = 0;
        d = m_element;
        switch (d->m_posMode) {
            case 1:
                m_10->m_posModeX = d->m_posDX;
                m_10->m_posModeY = d->m_posDY;
                break;
            case 2: {
                CAniRenderCtx* c = m_10;
                i32 x = c->m_screenX;
                if (c->m_byteFlags & 0x2) {
                    i32 dy = d->m_posDY;
                    i32 dx = d->m_posDX;
                    c->m_screenX = x - dx;
                    c->m_screenY = c->m_screenY + dy;
                } else {
                    i32 dy = d->m_posDY;
                    i32 dx = d->m_posDX;
                    c->m_screenX = x + dx;
                    c->m_screenY = c->m_screenY + dy;
                }
                break;
            }
            case 3:
                m_10->m_screenX = d->m_posDX;
                m_10->m_screenY = d->m_posDY;
                break;
            default:
                break;
        }

        // --- per-frame draw/sound trigger -------------------------------------
        CAniRenderCtx* c = m_10;
        i32 fire = 1;
        if (!(c->m_flags & 0x2000000) && !(m_element->m_flags & 0x8)) {
            if (c->m_anchor == -1) {
                fire = 0;
            }
        }
        if (fire) {
            CAniDesc* dd = m_element;
            if (dd->m_flags & 0x4) {
                i32 cue = c->m_screenX;
                i32* tbl;
                i32 entry;
                if (dd->m_randMod == 0) {
                    entry = 0;
                } else {
                    tbl = dd->m_randTable;
                    entry = tbl[Rng::Next2() % dd->m_randMod];
                }
                if (entry != 0) {
                    (reinterpret_cast<CAniBlitTrigger*>(this))->TriggerBlit_1587f0(cue, 0, 0, 0);
                }
            } else {
                i32* tbl;
                i32 entry;
                if (dd->m_randMod == 0) {
                    entry = 0;
                } else {
                    tbl = dd->m_randTable;
                    entry = tbl[Rng::Next2() % dd->m_randMod];
                }
                if (entry != 0) {
                    (reinterpret_cast<LeafCue*>(entry))->PlayIfElapsed(g_aniCueItem, 0, 0, 0);
                }
            }
        }

        // --- reload the per-frame timer (optionally float-scaled) -------------
        CAniDesc* rd = m_element;
        i32 reload = rd->m_frameTime;
        m_20 = reload;
        m_24 = (~rd->m_flags) & 1;
        if (*reinterpret_cast<i32*>(&m_scale) != 0x3f800000) { // raw-bits compare vs 1.0f (retail int cmp)
            m_20 = static_cast<i32>((static_cast<double>(static_cast<u32>(reload)) * m_scale));
        }

        // --- select the NEXT descriptor (10-way loop-mode on rd->m_loopMode) --------
        // Cases are ordered to reproduce retail's physical case-body layout
        // (9, 8, 7, 1, 2, 3, 4, 0, 5). Cases 2/3/4 test a sequence-position
        // predicate and, on a hit, fall into case 0's shared inline loop-restart
        // body (a goto into the single emitted block).
        i32 modeWord = rd->m_loopMode;
        CAniElement* arr;
        i32 i;
        CAniDesc* nd;
        switch (modeWord & 0xffff) {
            case 9: // pause
                m_28 = 1;
                break;
            case 8: { // reset to the first descriptor and unscaled timing
                if (m_14 != 0) {
                    m_index = 0;
                    m_element = static_cast<CAniDesc*>(m_14->AtChecked_06b270(0));
                    m_28 = 0;
                    m_scale = 1.0f;
                    m_pendingDraw = m_element->m_drawValue;
                    m_curDraw = m_element->m_drawValue;
                }
                break;
            }
            case 7: { // hold on the first two descriptors (m_index = 1 then 0)
                m_index = 1;
                m_element = static_cast<CAniDesc*>(m_14->AtChecked_06b270(1));
                if (m_element == 0) {
                    m_index = 0;
                    m_element = static_cast<CAniDesc*>(m_14->AtChecked_06b270(0));
                }
                if (m_element != 0) {
                    m_28 = 0;
                    m_20 = 0;
                    m_curDraw = m_pendingDraw;
                    m_pendingDraw = m_element->m_drawValue;
                }
                break;
            }
            case 1: { // advance only when the cursor's frame reached the descriptor param
                CAniRenderCtx* c2 = m_10;
                if (c2->m_frameCursor == m_element->m_param) {
                    if (modeWord != 9) {
                        CAniElement* a = m_14;
                        i32 j = m_index + 1;
                        m_index = j;
                        m_element = static_cast<CAniDesc*>(a->AtChecked_06b270(j));
                        if (m_element == 0) {
                            m_index = 0;
                            m_element = static_cast<CAniDesc*>(a->AtChecked_06b270(0));
                        }
                        if (m_element != 0) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case 2: { // advance only when the cursor reached the seq low frame
                CAniRenderCtx* c2 = m_10;
                CSprite* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_minIndex) {
                    goto loop_restart;
                }
                break;
            }
            case 3: { // advance only when the cursor reached the seq high frame
                CAniRenderCtx* c2 = m_10;
                CSprite* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_maxIndex) {
                    goto loop_restart;
                }
                break;
            }
            case 4: { // advance one past the seq low frame
                CAniRenderCtx* c2 = m_10;
                CSprite* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_minIndex + 1) {
                    goto loop_restart;
                }
                break;
            }
            case 0: // loop the playlist forward, inline bounds-checked fetch
            loop_restart:
                if (modeWord != 9) {
                    arr = m_14;
                    i = m_index + 1;
                    m_index = i;
                    if (i >= 0 && i < arr->m_records.GetSize()) {
                        nd = static_cast<CAniDesc*>(arr->m_records.GetAt(i));
                    } else {
                        nd = 0;
                    }
                    m_element = nd;
                    if (nd == 0) {
                        m_index = 0;
                        m_element = static_cast<CAniDesc*>(arr->AtChecked_06b270(0));
                    }
                    if (m_element != 0) {
                        m_curDraw = m_pendingDraw;
                        m_pendingDraw = m_element->m_drawValue;
                    }
                }
                break;
            case 5: { // advance only when the cursor reached one before the high frame
                CAniRenderCtx* c2 = m_10;
                CSprite* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_maxIndex - 1) {
                    if (modeWord != 9) {
                        CAniElement* a = m_14;
                        i32 j = m_index + 1;
                        m_index = j;
                        CAniDesc* p;
                        if (j >= 0 && j < a->m_records.GetSize()) {
                            p = static_cast<CAniDesc*>(a->m_records.GetAt(j));
                        } else {
                            p = 0;
                        }
                        m_element = p;
                        if (p == 0) {
                            m_index = 0;
                            i32 cnt = a->m_records.GetSize();
                            CAniDesc* first;
                            if (cnt > 0) {
                                first = static_cast<CAniDesc*>(a->m_records.GetAt(0));
                            } else {
                                first = 0;
                            }
                            m_element = first;
                        }
                        if (m_element != 0) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // --- return the per-frame draw value, consuming it when buffer-owned ------
    if (m_2c != 0) {
        if (m_20 != 0) {
            i32 r = m_curDraw;
            m_curDraw = 0;
            return r;
        }
        i32 r = m_pendingDraw;
        m_pendingDraw = 0;
        return r;
    }
    if (m_20 != 0) {
        return m_curDraw;
    }
    return m_pendingDraw;
}

// ---------------------------------------------------------------------------
// 0x15c900: dispatch on `type` - type 4 serializes, type 7 deserializes (both
// via the named sibling), every other type is a no-op that returns 1. A
// serialize/deserialize that returns 0 propagates the 0.
// @early-stop
// 80% - logic exact. Residual is the switch lowering: retail emits a `.rdata`
// jump table, but MSVC5 folds our empty cases into a cmp/je-subtract chain.
// Not source-steerable. docs/patterns/switch-cmpje-tree-vs-jumptable.md.
RVA(0x0015c900, 0x42)
i32 CAniAdvanceCursor::Find(CSerialArchive* ar, i32 type, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    switch (type) {
        case 3:
            break;
        case 4:
            if (Serialize_15c970(ar) == 0) {
                return 0;
            }
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            if (Deserialize_15ca70(ar) == 0) {
                return 0;
            }
            break;
        case 8:
            break;
        default:
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15c970: serialize the blit-param. Writes the eight dwords m_index..m_scale to
// the archive (4 bytes each via slot +0x30), zeroes a 0x80-byte label buffer,
// and if m_srcRef is set, fetches the worker label (a returns-by-value CString
// from the +0x2c sub-object's 0x152d30) and strcpy's it into the buffer; then
// writes the whole 0x80-byte buffer. Returns 1.
// @early-stop
// 99.4% - the eight Writes + the buffer zero + the GetLabel call + the inline
// strcpy all byte-exact; the only residual is the NRVO-temp addressing of the
// returned CString. Entropy tail; no source lever.
RVA(0x0015c970, 0xfe)
i32 CAniAdvanceCursor::Serialize_15c970(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_index, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_28, 4);
    ar->Write(&m_2c, 4);
    ar->Write(&m_pendingDraw, 4);
    ar->Write(&m_curDraw, 4);
    ar->Write(&m_scale, 4);
    char buf[0x80];
    for (i32 i = 0; i < 0x20; ++i) {
        (reinterpret_cast<i32*>(buf))[i] = 0;
    }
    if (m_14 != 0) {
        // the +0x0c owner (CLoadable::m_0c) carries the CDDrawSubMgrLeaf at +0x2c;
        // KeyOfValue_152d30 returns the label for the map VALUE m_14 (CAniElement : CObject).
        CString label = OwnerMgr()->m_animRegistry->KeyOfValue_152d30(m_14);
        strcpy(buf, label);
    }
    ar->Write(buf, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15ca70: deserialize the blit-param (the Serialize_15c970 twin). Reads the
// eight dwords from the archive, reads the 0x80-byte label buffer, and - when the
// label is non-empty - looks it up in the worker sub-object's +0x10 map to recover
// the worker into m_srcRef. Then the Setup_15c2d0-style tail. __thiscall, ret 0x4.
// @early-stop
// 89.9% - every instruction/CFG/offset present and the logic is byte-faithful;
// residual is a register-allocation cascade seeded at the first field read:
// retail keeps &m_30 in callee-saved ebp across the function, our cl spills it
// (frame 0x94) and rotates eax/ebp through the eight reads + the index tail.
// docs/patterns/pin-local-for-callee-saved-reg.md / zero-register-pinning.md.
RVA(0x0015ca70, 0x15b)
i32 CAniAdvanceCursor::Deserialize_15ca70(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_index, 4);
    ar->Read(&m_20, 4);
    ar->Read(&m_24, 4);
    ar->Read(&m_28, 4);
    ar->Read(&m_2c, 4);
    ar->Read(&m_pendingDraw, 4);
    ar->Read(&m_curDraw, 4);
    ar->Read(&m_scale, 4);
    char buf[0x80];
    ar->Read(buf, 0x80);
    if (strlen(buf) == 0) {
        m_14 = 0;
    } else {
        // the leaf's +0x10 map is CMapStringToPtr (Lookup 0x1b8438), value-typed void*
        void* out = 0;
        OwnerMgr()->m_animRegistry->m_10.Lookup(buf, out);
        m_14 = static_cast<CAniElement*>(out);
    }
    CAniElement* w = m_14;
    if (w != 0) {
        CAniDesc* e;
        if (m_index >= 0 && m_index < w->m_records.GetSize()) {
            e = static_cast<CAniDesc*>(w->m_records.GetAt(m_index));
        } else {
            e = 0;
        }
        m_element = e;
        if (e == 0) {
            m_index = 0;
            if (w->m_records.GetSize() > 0) {
                e = static_cast<CAniDesc*>(w->m_records.GetAt(0));
            } else {
                e = 0;
            }
            m_element = e;
        }
        if (m_element != 0) {
            m_20 = 0;
            m_28 = 0;
            m_curDraw = m_pendingDraw;
            m_pendingDraw = m_element->m_drawValue;
        }
    }
    return 1;
}

// CSprite::GetFrame (0x15cc30): the frame handle for index n (in [m_firstFrame,
// m_lastFrame]), or 0. Position-homed exile (a CSprite inline getter kept at this
// obj; the CSprite frame methods live in the S1 obj, WwdGameObject.cpp).
RVA(0x0015cc30, 0x1e)
i32 CSprite::GetFrame(i32 n) {
    if (n >= m_minIndex && n <= m_maxIndex) {
        return reinterpret_cast<i32>(static_cast<CImage*>(m_items.GetAt(n)));
    }
    return 0;
}

// CAniRenderCtx::ClampFirst (0x15cc50) / ClampLast (0x15cc90): clamp the +0x190
// frame cursor to the active sequence's low/high frame and re-resolve +0x198
// through the same bounds-checked fetch GetFrame (0x15cc30) inlines.
// @early-stop
// shrink-wrapped-callee-save-push wall (~62%, docs/patterns/shrink-wrapped-callee-
// save-push.md): retail defers `push esi` past the null guard and emits the
// esi-pop epilogue inline per exit; cl hoists push esi to the prologue and
// tail-merges the exits. Body byte-exact; not source-steerable.
RVA(0x0015cc50, 0x38)
void CAniRenderCtx::ClampFirst_15cc50() {
    CSprite* seq = m_frameSeq;
    if (seq == 0) {
        return;
    }
    i32 n = seq->m_minIndex;
    m_frameCursor = n;
    if (n >= seq->m_minIndex && n <= seq->m_maxIndex) {
        m_curFrame = reinterpret_cast<i32>(static_cast<CImage*>(seq->m_items.GetAt(n)));
    } else {
        m_curFrame = 0;
    }
}

// @early-stop
// shrink-wrapped-callee-save-push wall (~62%); twin of ClampFirst above.
RVA(0x0015cc90, 0x38)
void CAniRenderCtx::ClampLast_15cc90() {
    CSprite* seq = m_frameSeq;
    if (seq == 0) {
        return;
    }
    i32 n = seq->m_maxIndex;
    m_frameCursor = n;
    if (n >= seq->m_minIndex && n <= seq->m_maxIndex) {
        m_curFrame = reinterpret_cast<i32>(static_cast<CImage*>(seq->m_items.GetAt(n)));
    } else {
        m_curFrame = 0;
    }
}
