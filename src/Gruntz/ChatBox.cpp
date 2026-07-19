// ChatBox.cpp - a two-row scrolling on-screen text/chat display (C:\Proj\Gruntz).
//
// Trace-discovered as Region_182ab0. Two text rows, each with a CString font key
// (m_row0Key / m_row1Key), a "current message/animation record" pointer
// (m_row0Anim / m_row1Anim) and per-row frame-animation state (current frame
// handle + reload period + countdown timer + draw offset + frame index). Owns a
// CPtrList of message nodes (m_nodeList) whose stored payloads it frees on
// Reset/teardown; a parent/page back pointer (m_page); and a queued/active node
// slot (m_activeNode).
//
// Field names are recovered from the member writes; only offsets + code bytes are
// load-bearing (rename is /O2 name-independent). The message-node accessor methods
// (Configure/scroll-step/etc.) live in another TU and are modeled here as no-body
// externs so their calls are reloc-masked.
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Image/CImage.h>
#include <Image/ImageSet.h> // CImageSet - the per-row animation record (m_page->m_10 map value)
#include <rva.h>

#include <Gruntz/ChatBox.h>
// The menu-drive methods (0x182c70..0x183150) forward to the owned menu page
// (m_activeNode, a CMenuPage - the same class the node-walks dispatch into) and
// blit the menu surface set (CDDSurface) hung off the owner.
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h> // m_page->m_drawTarget: front/back/overlay CDDrawSurfacePair
#include <DDrawMgr/DDrawSurfacePair.h> // each pair's +0x2c CDDSurface + +0x1c src RECT
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_page->m_10: CImageRegistry (CMapStringToOb catalog)
#include <Gruntz/GameRegistry.h>          // CDDrawSurfaceMgr (m_page) + CSndHost + LeafCue
#include <Gruntz/MenuPage.h>

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// ---------------------------------------------------------------------------

// The message-node class the rows dispatch into IS CMenuPage (MenuPage.h): the node
// accessors (dtor 0x183250, ReleaseAll 0x183990, RestoreFocus 0x1839d0, Click
// 0x1840a0, SelectForward 0x1843f0, SelectBackward 0x1844d0, SelectFwd2 0x184230,
// SelectBack2 0x184310, GetKey 0x1832d0) are the same RVAs. __thiscall.
//
// The owner reached through m_page is the canonical CDDrawSurfaceMgr
// (GameRegistry.h). Its three facets this box drives are all real engine classes
// (the ex CChatPage/CMenuRenderSet/CChatCatalog/CChatRoster/CChatAnim/CChatListNode
// views are DISSOLVED, 2026-07-14):
//   +0x04 m_drawTarget : CDDrawSubMgrPages   - the menu surface set (Flip/BltFast in Post).
//                   Its +0x10/+0x14/+0x18 are front/back/overlay CDDrawSurfacePair,
//                   each carrying a CDDSurface @+0x2c and the source's src RECT @+0x1c.
//   +0x10 m_10    : CImageRegistry (== CDDrawWorkerRegistry) - the key->record catalog.
//                   Its +0x10 map is a CMapStringToOb (Lookup 0x1b8008, disasm-proven -
//                   NOT the 0x1b8438 CMapStringToPtr the ex-view guessed), whose values
//                   are CImageSet animation records (m_frames @+0x14, index range
//                   [m_minIndex @+0x64, m_maxIndex @+0x68]).
//   +0x28 m_28    : CSndHost (== CDDrawSubMgrLeafScan) - the scroll-cue roster. Its +0x10
//                   map is a CMapStringToPtr (Lookup 0x1b8438) of LeafCue timers, gated on
//                   the +0x30 emit/busy word; each LeafCue carries the pooled
//                   DSoundCloneInst (m_10), a last-play clock (m_14) and cooldown (m_18).
//
// The CPtrList node list (m_nodeList) is walked with the PUBLIC MFC accessors
// GetHeadPosition()/GetNext(POSITION&) (both inline; GetNext advances the POSITION
// then returns the void* payload) - byte-identical to the raw CNode chain walk and
// with no need to reach the protected CPtrList::CNode.

// DISSOLVED (Fable A2, 2026-07-14): the "CChatSprite" arg WAS the canonical
// CMenuItem (<Gruntz/MenuItem.h>, via MenuPage.h): its "+0x44/+0x48 anchor with
// the 0xeeeeeeee sentinel" is exactly m_fixedX/m_fixedY ("use the caller's
// coords" placement override), and the "Measure" virtual at slot 5 (+0x14) is
// GetFrameWidth (0x185520) - Draw centers each row's frame on the item.

// The horizontal-scroll edge state read by the two scroll-step methods.
extern "C" u32 g_killCueClock; // 0x6bf3c0

// ===========================================================================
// CChatBox
// ===========================================================================

// CChatBox::Init (0xa0280) is NOT in this TU's block: it sits inside the menu
// state's contiguous 0x9fe50..0xa0d80 run, so it is homed in
// src/Gruntz/MenuState.cpp (the obj that contributes that byte).

// ---------------------------------------------------------------------------
// The menu-region seeder (0x0182ab0). The three SOURCE-side views are gone: the retail
// call proves the arg chain is entirely canonical classes -
//   arg1 = CMenuState::m_c            -> CDDrawSurfaceMgr (<Gruntz/GameRegistry.h>)
//          holder->m_drawTarget       -> CDDrawSubMgrPages          (<Gruntz/ResMgr.h>)
//          drawTarget->m_10           -> CDDrawSubMgrPages::SurfaceA (its +0x10/+0x14 pixel
//                                        extent is now named there; see the disasm cited
//                                        in ResMgr.h) - the default RECT is (0,0,w-1,h-1).
//   arg2 = m_4->m_gameWnd->m_hwnd     -> the game window's HWND (WAP32::CGameWnd +0x04).
//
// The `this` IS the CChatBox LoadAssets just newed (retail `mov [esi+0x1b4],ecx` right
// before `call 0x182ab0`, ecx unchanged) - now a real CChatBox method. Every store lands
// on a CChatBox member (m_page/m_4/the m_rect8 RECT/m_headGap/m_rowSpacing/m_wrapFlag/m_activeNode);
// the three ex-blockers (m_page CChatPage->CDDrawSurfaceMgr, m_pad8->RECT, m_wrapFlag
// char->i32) are all resolved in ChatBox.h, so the ex MenuRegion view is dissolved.
RVA(0x00182ab0, 0x7b)
i32 CChatBox::InitRegion(CDDrawSurfaceMgr* src, i32 a, RECT* rc, i32 d, i32 e, i32 f) {
    if (!src) {
        return 0;
    }
    m_page = src;
    m_4 = a;
    m_wrapFlag = f;
    m_headGap = d;
    m_rowSpacing = e;
    m_activeNode = 0;
    if (rc) {
        CopyRect(&m_rect8, rc);
        return 1;
    }
    m_rect8.left = 0;
    m_rect8.top = 0;
    m_rect8.right = src->m_drawTarget->m_frontPair->m_width - 1;
    m_rect8.bottom = src->m_drawTarget->m_frontPair->m_height - 1;
    return 1;
}

// destructor lives in ChatBoxDtor.cpp (the /GX EH-frame TU; it is the
// only method here that needs the frame, so the rest stay frameless under base).

// free the node list, re-zero both rows.
RVA(0x00182b30, 0x30)
void CChatBox::Reset() {
    Clear();
    m_page = 0;
    m_4 = 0;
    m_activeNode = 0;
    m_row0Anim = 0;
    m_row1Anim = 0;
    m_row0Frame = 0;
    m_row1Frame = 0;
    m_row0Key.Empty();
    m_row1Key.Empty();
}

// free every node's owned payload, empty the list, clear the queue slot.
// `delete payload` calls the header-inline ~CMenuPage OUT-OF-LINE: under /GX
// (this TU's flags) MSVC5 declines to fold the EH-stateful teardown into a
// frameless fn (verified: under base flags it folds and Clear craters 100->69),
// so this obj emits + calls the standalone COMDAT copy retail keeps at
// 0x183250 (link position = this obj's COMDAT tail, right after HitTest2
// 0x183230) - pinned here (an inline dtor cannot carry RVA(); MenuPage.cpp no
// longer defines it).
// @rva-symbol: ??1CMenuPage@@QAE@XZ 0x00183250 0x71
RVA(0x00182b60, 0x3e)
void CChatBox::Clear() {
    POSITION pos = m_nodeList.GetHeadPosition();
    while (pos) {
        CMenuPage* payload = reinterpret_cast<CMenuPage*>(m_nodeList.GetNext(pos));
        delete payload; // ~CMenuPage is non-virtual -> direct dtor + ??3 (retail 0x182b79/0x182b7f)
    }
    m_nodeList.RemoveAll();
    m_activeNode = 0;
}

// append a node to the list; first node also becomes the active one.
RVA(0x00182ba0, 0x35)
i32 CChatBox::AddNode(void* node) {
    if (!node) {
        return 0;
    }
    m_nodeList.AddTail(node);
    if (!m_activeNode) {
        AttachNode(node);
    }
    return 1;
}

// @early-stop
// EH-frame contradiction wall (topic:eh topic:wall): retail Find destructs its
// GetKey CString temp with NO fs:0 frame (a non-/GX shape), but this TU MUST be
// /GX for Clear + the ??1CMenuPage COMDAT (see the unit note in units.toml) -
// under /GX cl frames this fn (73.8 -> 47.9). One retail fn per flag choice
// diverges; suspected retail TU split around 0x182be0 (this region is already
// split: InitRegion 0x182ab0 lives in menustateassets). Underneath sits the
// older regalloc residual (node walk pinned ebp-vs-edi + an extra `push ecx`
// slot); logic is complete.
// find the message node whose key matches s (linear scan + strcmp).
RVA(0x00182be0, 0x8d)
i32 CChatBox::Find(const char* s) {
    POSITION pos = m_nodeList.GetHeadPosition();
    while (pos) {
        CMenuPage* payload = reinterpret_cast<CMenuPage*>(m_nodeList.GetNext(pos));
        if (payload) {
            CString key = payload->GetKey();
            if (strcmp(key, s) == 0) {
                return (i32)payload;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// The front-end menu drive (CMenuState::Render fires these on its m_1b4 CChatBox).
// Each guards on the owned menu page (m_activeNode, a CMenuPage) and forwards one
// navigation; the page methods are reloc-masked rel32 callees (MenuPage.cpp).
// ---------------------------------------------------------------------------

// notify the page of the per-frame delta, then run the inner scroll
// Step (0x182ed0). The dt arg is u32 (Render passes g_frameDelta); the inner Step
// takes i32, so this is the Step(u32) overload that calls Step(i32).
RVA(0x00182c70, 0x38)
i32 CChatBox::Step(u32 dt) {
    if (!m_activeNode) {
        return 0;
    }
    if (!m_activeNode->NotifyAll((void*)dt)) {
        return 0;
    }
    return Step(static_cast<i32>(dt)) != 0;
}

// lay out the page using the owner's first surface holder as the ctx.
RVA(0x00182cb0, 0x26)
i32 CChatBox::Pre() {
    if (!m_activeNode) {
        return 0;
    }
    i32 ctx = (i32)m_page->m_drawTarget->m_backPair;
    if (!ctx) {
        return ctx;
    }
    return m_activeNode->Layout(ctx) != 0;
}

// flip the menu back buffer, then blit the source onto the target.
RVA(0x00182ce0, 0x36)
i32 CChatBox::Post() {
    CDDrawSubMgrPages* s = m_page->m_drawTarget;
    s->m_frontPair->m_surface->Flip(0);
    s->m_backPair->m_surface
        ->BltFast(0, 0, s->m_overlayPair->m_surface, &s->m_overlayPair->m_srcRect, 0x10);
    return 1;
}

// entity-flag 0x40000000 scan -> advance the page focus.
RVA(0x00182d20, 0x16)
i32 CChatBox::OnFlag40000000() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusNext() != 0;
}

// entity-flag 0x80000000 scan -> retreat the page focus.
RVA(0x00182d40, 0x16)
i32 CChatBox::OnFlag80000000() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusPrev() != 0;
}

// entity-flag 0x00000003 scan -> activate the focused item.
RVA(0x00182d60, 0x16)
i32 CChatBox::OnFlag00000003() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->Activate() != 0;
}

// entity-flag 0x00000100 scan -> switch the page (refocus).
RVA(0x00182d80, 0x18)
i32 CChatBox::OnFlag00000100() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->Switch(1) != 0;
}

// entity-flag 0x10000000 scan -> step the focus back N nodes.
RVA(0x00183130, 0x16)
i32 CChatBox::OnFlag10000000() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusBackwardN() != 0;
}

// entity-flag 0x20000000 scan -> step the focus forward N nodes.
RVA(0x00183150, 0x16)
i32 CChatBox::OnFlag20000000() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusForwardN() != 0;
}

// mouse focus+select -> forward (x,y) to the owned menu page's FocusAndSelect.
RVA(0x00183170, 0x24)
i32 CChatBox::FocusSelect(i32 x, i32 y) {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusAndSelect(x, y) != 0;
}

// make `n` the active node (detach + rebuild it).
RVA(0x00182da0, 0x2a)
i32 CChatBox::AttachNode(void* n) {
    if (!n) {
        return 0;
    }
    m_activeNode = (CMenuPage*)n;
    ((CMenuPage*)n)->ReleaseAll();
    m_activeNode->RestoreFocus();
    return 1;
}

// find a node by key and make it active.
RVA(0x00182dd0, 0x19)
i32 CChatBox::ReplaceNode(void* n) {
    return AttachNode((void*)Find(static_cast<const char*>(n)));
}

// @early-stop
// scheduling wall (~95%): body byte-exact and the Lookup reloc now binds correctly to
// CMapStringToOb::Lookup @0x1b8008 (masked in objdiff). Residual is a 1-instruction
// reorder: cl emits the out-param zero-init (`a_ob = 0` -> `mov [slot],0`) BEFORE the
// arg pushes, where retail defers it past them (`mov [esp+0xc],0` after push ecx/edx).
// Independent-store scheduler choice, not steerable from source. Logic complete.
// advance row0 to the message keyed by `key`; cache its frame state. The catalog map
// is a CMapStringToOb (Lookup 0x1b8008); its CObject* value is a CImageSet record.
RVA(0x00182df0, 0x69)
i32 CChatBox::AdvanceRow0(void* key, i32 x, i32 y) {
    if (!m_page) {
        return 0;
    }
    CObject* a_ob = 0;
    m_page->m_imageRegistry->m_10map.Lookup(static_cast<const char*>(key), a_ob);
    CImageSet* a = (CImageSet*)a_ob;
    m_row0Anim = a;
    if (!a) {
        return 0;
    }
    m_row0Frame = (CImage*)a->m_items.GetAt(a->m_minIndex);
    m_row0FrameIdx = a->m_minIndex;
    m_row0Period = x;
    m_row0Timer = x;
    m_row0Offset = y;
    return 1;
}

// @early-stop
// scheduling wall (~95%): same as AdvanceRow0 - body byte-exact, Lookup reloc binds
// CMapStringToOb::Lookup @0x1b8008 (masked), residual is cl emitting the out-param
// zero-init before the arg pushes vs retail deferring it past them. Logic complete.
// advance row1 to the message keyed by `key`; cache its frame state.
RVA(0x00182e60, 0x69)
i32 CChatBox::AdvanceRow1(void* key, i32 x, i32 y) {
    if (!m_page) {
        return 0;
    }
    CObject* a_ob = 0;
    m_page->m_imageRegistry->m_10map.Lookup(static_cast<const char*>(key), a_ob);
    CImageSet* a = (CImageSet*)a_ob;
    m_row1Anim = a;
    if (!a) {
        return 0;
    }
    m_row1Frame = (CImage*)a->m_items.GetAt(a->m_minIndex);
    m_row1FrameIdx = a->m_minIndex;
    m_row1Period = x;
    m_row1Timer = x;
    m_row1Offset = y;
    return 1;
}

// @early-stop
// regalloc wall: body byte-exact (unsigned counter compare, clamp+wrap of both
// rows' frame indices), but retail holds the row node in eax with the counter in
// edx, whereas MSVC swaps them here; 1-register phase shift only. ~89%.
// per-frame advance of both rows' scroll counters & frame indices.
RVA(0x00182ed0, 0xbc)
i32 CChatBox::Step(i32 delta) {
    CImageSet* a = m_row0Anim;
    if (a) {
        if (static_cast<u32>(m_row0Timer) > static_cast<u32>(delta)) {
            m_row0Timer -= delta;
        } else {
            m_row0Timer = m_row0Period;
            i32 f = m_row0FrameIdx + 1;
            m_row0FrameIdx = f;
            CImage* v;
            if (f >= a->m_minIndex && f <= a->m_maxIndex) {
                v = (CImage*)a->m_items.GetAt(f);
            } else {
                v = 0;
            }
            m_row0Frame = v;
            if (v == 0) {
                m_row0Frame = (CImage*)a->m_items.GetAt(a->m_minIndex);
                m_row0FrameIdx = a->m_minIndex;
            }
        }
    }
    CImageSet* b = m_row1Anim;
    if (b) {
        if (static_cast<u32>(m_row1Timer) > static_cast<u32>(delta)) {
            m_row1Timer -= delta;
            return 1;
        }
        m_row1Timer = m_row1Period;
        i32 f = m_row1FrameIdx + 1;
        m_row1FrameIdx = f;
        CImage* v;
        if (f >= b->m_minIndex && f <= b->m_maxIndex) {
            v = (CImage*)b->m_items.GetAt(f);
        } else {
            v = 0;
        }
        m_row1Frame = v;
        if (v == 0) {
            m_row1Frame = (CImage*)b->m_items.GetAt(b->m_minIndex);
            m_row1FrameIdx = b->m_minIndex;
        }
    }
    return 1;
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; residual is
// only the differently-named Blit extern (0x153790) + the GetFrameWidth slot.
// ~95%.
// blit both rows' current frames, centered under the sprite anchor.
RVA(0x00182f90, 0x92)
i32 CChatBox::Draw(i32 a0, i32 sprite_, i32 arg2, i32 arg3) {
    CMenuItem* sprite = (CMenuItem*)sprite_;
    if (!sprite) {
        return 0;
    }
    i32 anchorX, anchorY;
    if (sprite->m_fixedX != static_cast<i32>(0xeeeeeeee)) {
        anchorY = sprite->m_fixedY;
        anchorX = sprite->m_fixedX;
    } else {
        anchorY = arg3;
        anchorX = arg2;
    }
    if (m_row0Frame) {
        i32 x = -(sprite->GetFrameWidth() / 2) - m_row0Offset + anchorX;
        m_row0Frame->RenderFrame((void*)arg2, (void*)x, (void*)anchorY, (void*)0);
    }
    if (m_row1Frame) {
        i32 x = sprite->GetFrameWidth() / 2 + m_row1Offset + anchorX;
        m_row1Frame->RenderFrame((void*)arg2, (void*)x, (void*)anchorY, (void*)0);
    }
    return 1;
}

// @early-stop
// shrink-wrap + scheduling wall: body byte-exact, but retail DEFERS the
// `push edi/esi` past the empty-key guard (and uses 2 callee-saved regs for the
// elapsed-time compare); MSVC saves them in the prologue, shifting the whole BB
// layout. Logic complete. ~63%.
// scroll row0's sprite one tick if its scroll interval has elapsed.
RVA(0x00183030, 0x7b)
i32 CChatBox::ScrollRow0() {
    if (m_row0Key.GetLength() == 0) {
        return 0;
    }
    CSndHost* roster = m_page->m_soundRegistry;
    if (roster->m_emitGate) {
        return 0;
    }
    void* t_ob = 0;
    roster->m_10.Lookup(static_cast<const char*>(m_row0Key), t_ob);
    LeafCue* t = (LeafCue*)t_ob;
    if (!t) {
        return 0;
    }
    if (!g_sndEnabled) {
        return 0;
    }
    i32 delta = g_sndCueTag;
    i32 clock = g_killCueClock;
    u32 elapsed = static_cast<u32>(clock) - static_cast<u32>(t->m_14);
    if (elapsed < static_cast<u32>(t->m_18)) {
        return 0;
    }
    t->m_14 = clock;
    t->m_10->ConfigureItem(delta, 0, 0, 0);
    return 0;
}

// @early-stop
// shrink-wrap + scheduling wall (same as ScrollRow0): body byte-exact, retail
// defers the `push edi/esi` past the empty-key guard. Logic complete. ~63%.
// scroll row1's sprite one tick if its scroll interval has elapsed.
RVA(0x001830b0, 0x7b)
i32 CChatBox::ScrollRow1() {
    if (m_row1Key.GetLength() == 0) {
        return 0;
    }
    CSndHost* roster = m_page->m_soundRegistry;
    if (roster->m_emitGate) {
        return 0;
    }
    void* t_ob = 0;
    roster->m_10.Lookup(static_cast<const char*>(m_row1Key), t_ob);
    LeafCue* t = (LeafCue*)t_ob;
    if (!t) {
        return 0;
    }
    if (!g_sndEnabled) {
        return 0;
    }
    i32 delta = g_sndCueTag;
    i32 clock = g_killCueClock;
    u32 elapsed = static_cast<u32>(clock) - static_cast<u32>(t->m_14);
    if (elapsed < static_cast<u32>(t->m_18)) {
        return 0;
    }
    t->m_14 = clock;
    t->m_10->ConfigureItem(delta, 0, 0, 0);
    return 0;
}

// forward a hit-test to the active node (slot 0x1840a0).
RVA(0x001831a0, 0x24)
i32 CChatBox::HitTest0(i32 x, i32 y) {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->Click(x, y) != 0;
}

// forward a hit-test to the active node (slot 0x1843f0).
RVA(0x00183210, 0x16)
i32 CChatBox::HitTest1() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->SelectForward() != 0;
}

// forward a hit-test to the active node (slot 0x1844d0).
RVA(0x00183230, 0x16)
i32 CChatBox::HitTest2() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->SelectBackward() != 0;
}

// forward a query to the active node (callee 0x184230); bool-normalize.
RVA(0x001831d0, 0x16)
i32 CChatBox::HitTest3() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->SelectFwd2() != 0;
}

// forward a query to the active node (callee 0x184310); bool-normalize.
RVA(0x001831f0, 0x16)
i32 CChatBox::HitTest4() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->SelectBack2() != 0;
}

// All the .cpp-local engine views are DISSOLVED onto their canonical classes
// (CDDrawSurfaceMgr / CDDrawSubMgrPages / CDDrawSurfacePair / CImageRegistry /
// CImageSet / CSndHost / LeafCue), which carry their own SIZE metadata in their
// headers. CChatBox itself lives in ChatBox.h.
