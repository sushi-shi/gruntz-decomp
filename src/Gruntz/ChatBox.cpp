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
#include <Gruntz/SoundCueMgr.h>
#include <Image/CImage.h>
#include <rva.h>

#include <Gruntz/ChatBox.h>
// The menu-drive methods (0x182c70..0x183150) forward to the owned menu page
// (m_activeNode, a CMenuPage - the same class the node-walks dispatch into) and
// blit the menu surface set (CDDSurface) hung off the owner.
#include <DDrawMgr/DirectDrawMgr.h>
#include <Gruntz/MenuPage.h>

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// ---------------------------------------------------------------------------

// The message-node class the rows dispatch into IS CMenuPage (MenuPage.h): the node
// accessors (dtor 0x183250, ReleaseAll 0x183990, RestoreFocus 0x1839d0, Click
// 0x1840a0, SelectForward 0x1843f0, SelectBackward 0x1844d0, SelectFwd2 0x184230,
// SelectBack2 0x184310, GetKey 0x1832d0) are the same RVAs. __thiscall.

struct CChatCatalog;
struct CChatRoster;
struct CMenuRenderSet;

// The on-screen page/owner reached through CChatBox::m_page: the menu surface set
// (Post/Pre) at +0x04, the key->node catalog at +0x10, the sprite roster (scroll-
// step) at +0x28.
struct CChatPage {
    char m_pad0[0x4];
    CMenuRenderSet* m_4; // +0x04 -> the menu surface set (Flip/BltFast)
    char m_pad8[0x10 - 0x8];
    CChatCatalog* m_10; // +0x10 -> the key->node catalog
    char m_pad14[0x28 - 0x14];
    CChatRoster* m_28; // +0x28 -> sprite roster (scroll-step)
};

// A view of the CPtrList node layout (CPtrList::CNode is protected): pNext/pPrev/
// data. The teardown walk follows pNext and frees each stored payload.
struct CChatListNode {
    CChatListNode* pNext;
    CChatListNode* pPrev;
    void* data;
};

// The per-row frame drawable each row blits through (0x153790). __thiscall.

// The animation/frame record the row-advance lookups return: a frame table m_14
// indexed by the current frame m_64, plus a clamp range m_64..m_68.
struct CChatAnim {
    void* vptr;
    char pad4[0x14 - 0x4];
    CImage** m_14; // +0x14 frame-drawable table
    char pad18[0x64 - 0x18];
    i32 m_64; // +0x64 current frame index
    i32 m_68; // +0x68 max frame
};

// The key->record map (CMapWordToOb-style Lookup at 0x1b8008 / 0x1b8438). The
// value type differs per instance (CChatAnim for the rows, CChatTimer for scroll),
// so the out-param is generic. __thiscall.
// The key->node map is an MFC CMapStringToOb (Lookup @0x1b8438); cast at each call.
struct CChatMap {};

// The on-screen catalog reached through CChatPage::m_10; the key->node map lives
// at +0x10 inside it (the `add ecx,0x10` in the row-advance lookups).
struct CChatCatalog {
    char pad0[0x10];
    CChatMap m_10map;
    char pad14[0x64 - 0x14];
    i32 m_64; // current frame/index, read straight through the lookup result
};

extern "C" void RezFree(void* p); // 0x1b9b82

// The font/sprite passed into Draw: anchor coords m_44/m_48 (0xeeeeeeee = "use the
// caller's fallback coords") and a virtual Measure() at vtable slot +0x14 (index 5).
struct CChatSprite {
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual i32 Measure(); // slot +0x14
    char pad4[0x44 - 0x4];
    i32 m_44; // +0x44 anchor x
    i32 m_48; // +0x48 anchor y
};

// The horizontal-scroll edge state read by the two scroll-step methods.
DATA(0x0061ab20)
extern i32 g_scrollEnabled; // 0x61ab20
DATA(0x0061ab24)
extern i32 g_scrollDelta; // 0x61ab24
DATA(0x006bf3c0)
extern i32 g_scrollClock; // 0x6bf3c0

// The sprite poke target hung off a scroll-timer's m_10 (0x1360d0). __thiscall.

// The per-row scroll timer record the scroll-step lookups return: a sprite poke
// target m_10, a last-tick m_14, and an interval m_18.
struct CChatTimer {
    char pad0[0x10];
    CSoundCueMgr* m_10; // +0x10 sprite poke target
    i32 m_14;           // +0x14 last tick the row scrolled at
    i32 m_18;           // +0x18 scroll interval
};

// The on-screen sprite roster reached via CChatPage::m_28: a key->timer map at
// +0x10 and a "busy" gate at +0x30.
struct CChatRoster {
    char pad0[0x10];
    CChatMap m_10; // +0x10 key->timer map
    char pad14[0x30 - 0x14];
    i32 m_30; // +0x30 busy gate
};

// The menu surface set the Post() flip/blit reaches via the owner (m_page->m_4):
// three surface holders (a back buffer to Flip, a target + a source to BltFast),
// each carrying its CDDSurface at +0x2c; the source holder also carries the blit
// RECT at +0x1c. Field names are placeholders (offsets are load-bearing).
struct CMenuSurf {
    char pad0[0x2c];
    CDDSurface* m_2c; // +0x2c owned surface
};
struct CMenuSurfSrc {
    char pad0[0x1c];
    i32 m_1c; // +0x1c blit RECT (4 ints, &m_1c)
    char pad20[0x2c - 0x20];
    CDDSurface* m_2c; // +0x2c source surface
};
struct CMenuRenderSet {
    char pad0[0x10];
    CMenuSurf* m_10;    // +0x10 back buffer (Flip)
    CMenuSurf* m_14;    // +0x14 blit target
    CMenuSurfSrc* m_18; // +0x18 blit source + RECT
};

// ===========================================================================
// CChatBox
// ===========================================================================

// re-zero both rows (no list teardown; Reset minus the Clear()).
RVA(0x000a0280, 0x2b)
void CChatBox::Init() {
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
RVA(0x00182b60, 0x3e)
void CChatBox::Clear() {
    CChatListNode* node = (CChatListNode*)m_nodeList.GetHeadPosition();
    while (node) {
        CChatListNode* cur = node;
        node = node->pNext;
        CMenuPage* payload = (CMenuPage*)cur->data;
        if (payload) {
            payload->~CMenuPage();
            RezFree(payload);
        }
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
// regalloc + stack-slot wall: body byte-exact (advance-first node walk + inline
// strcmp + GetKey-by-value), but MSVC reserves an extra `push ecx` slot and pins
// the node walk in a different callee-saved reg (ebp vs edi) than retail; logic
// is complete. ~74%.
// find the message node whose key matches s (linear scan + strcmp).
RVA(0x00182be0, 0x8d)
i32 CChatBox::Find(const char* s) {
    CChatListNode* node = (CChatListNode*)m_nodeList.GetHeadPosition();
    while (node) {
        CChatListNode* cur = node;
        node = node->pNext;
        CMenuPage* payload = (CMenuPage*)cur->data;
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
// Step (0x182ed0). The dt arg is u32 (Render passes g_645584); the inner Step
// takes i32, so this is the Step(u32) overload that calls Step(i32).
RVA(0x00182c70, 0x38)
i32 CChatBox::Step(u32 dt) {
    if (!m_activeNode) {
        return 0;
    }
    if (!m_activeNode->NotifyAll((void*)dt)) {
        return 0;
    }
    return Step((i32)dt) != 0;
}

// lay out the page using the owner's first surface holder as the ctx.
RVA(0x00182cb0, 0x26)
i32 CChatBox::Pre() {
    if (!m_activeNode) {
        return 0;
    }
    i32 ctx = (i32)m_page->m_4->m_14;
    if (!ctx) {
        return ctx;
    }
    return m_activeNode->Layout(ctx) != 0;
}

// flip the menu back buffer, then blit the source onto the target.
RVA(0x00182ce0, 0x36)
i32 CChatBox::Post() {
    CMenuRenderSet* s = m_page->m_4;
    s->m_10->m_2c->Flip(0);
    s->m_14->m_2c->BltFast(0, 0, s->m_18->m_2c, &s->m_18->m_1c, 0x10);
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
    return AttachNode((void*)Find((const char*)n));
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; the residual
// is only the differently-named Lookup extern (0x1b8008, another TU's CMap). ~95%.
// advance row0 to the message keyed by `key`; cache its frame state.
RVA(0x00182df0, 0x69)
i32 CChatBox::AdvanceRow0(void* key, i32 x, i32 y) {
    if (!m_page) {
        return 0;
    }
    CChatAnim* a = 0;
    ((CMapStringToOb*)&m_page->m_10->m_10map)->Lookup((const char*)key, (CObject*&)a);
    m_row0Anim = a;
    if (!a) {
        return 0;
    }
    m_row0Frame = a->m_14[a->m_64];
    m_row0FrameIdx = a->m_64;
    m_row0Period = x;
    m_row0Timer = x;
    m_row0Offset = y;
    return 1;
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; residual is
// only the differently-named Lookup extern (0x1b8008, another TU's CMap). ~95%.
// advance row1 to the message keyed by `key`; cache its frame state.
RVA(0x00182e60, 0x69)
i32 CChatBox::AdvanceRow1(void* key, i32 x, i32 y) {
    if (!m_page) {
        return 0;
    }
    CChatAnim* a = 0;
    ((CMapStringToOb*)&m_page->m_10->m_10map)->Lookup((const char*)key, (CObject*&)a);
    m_row1Anim = a;
    if (!a) {
        return 0;
    }
    m_row1Frame = a->m_14[a->m_64];
    m_row1FrameIdx = a->m_64;
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
    CChatAnim* a = m_row0Anim;
    if (a) {
        if ((u32)m_row0Timer > (u32)delta) {
            m_row0Timer -= delta;
        } else {
            m_row0Timer = m_row0Period;
            i32 f = m_row0FrameIdx + 1;
            m_row0FrameIdx = f;
            CImage* v;
            if (f >= a->m_64 && f <= a->m_68) {
                v = a->m_14[f];
            } else {
                v = 0;
            }
            m_row0Frame = v;
            if (v == 0) {
                m_row0Frame = a->m_14[a->m_64];
                m_row0FrameIdx = a->m_64;
            }
        }
    }
    CChatAnim* b = m_row1Anim;
    if (b) {
        if ((u32)m_row1Timer > (u32)delta) {
            m_row1Timer -= delta;
            return 1;
        }
        m_row1Timer = m_row1Period;
        i32 f = m_row1FrameIdx + 1;
        m_row1FrameIdx = f;
        CImage* v;
        if (f >= b->m_64 && f <= b->m_68) {
            v = b->m_14[f];
        } else {
            v = 0;
        }
        m_row1Frame = v;
        if (v == 0) {
            m_row1Frame = b->m_14[b->m_64];
            m_row1FrameIdx = b->m_64;
        }
    }
    return 1;
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; residual is
// only the differently-named Blit extern (0x153790) + the virtual Measure slot.
// ~95%.
// blit both rows' current frames, centered under the sprite anchor.
RVA(0x00182f90, 0x92)
i32 CChatBox::Draw(i32 a0, i32 sprite_, i32 arg2, i32 arg3) {
    CChatSprite* sprite = (CChatSprite*)sprite_;
    if (!sprite) {
        return 0;
    }
    i32 anchorX, anchorY;
    if (sprite->m_44 != (i32)0xeeeeeeee) {
        anchorY = sprite->m_48;
        anchorX = sprite->m_44;
    } else {
        anchorY = arg3;
        anchorX = arg2;
    }
    if (m_row0Frame) {
        i32 x = -(sprite->Measure() / 2) - m_row0Offset + anchorX;
        m_row0Frame->RenderFrame((void*)arg2, (void*)x, (void*)anchorY, (void*)0);
    }
    if (m_row1Frame) {
        i32 x = sprite->Measure() / 2 + m_row1Offset + anchorX;
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
    CChatRoster* roster = m_page->m_28;
    if (roster->m_30) {
        return 0;
    }
    CChatTimer* t = 0;
    ((CMapStringToOb*)&roster->m_10)->Lookup((const char*)m_row0Key, (CObject*&)t);
    if (!t) {
        return 0;
    }
    if (!g_scrollEnabled) {
        return 0;
    }
    i32 delta = g_scrollDelta;
    i32 clock = g_scrollClock;
    u32 elapsed = (u32)clock - (u32)t->m_14;
    if (elapsed < (u32)t->m_18) {
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
    CChatRoster* roster = m_page->m_28;
    if (roster->m_30) {
        return 0;
    }
    CChatTimer* t = 0;
    ((CMapStringToOb*)&roster->m_10)->Lookup((const char*)m_row1Key, (CObject*&)t);
    if (!t) {
        return 0;
    }
    if (!g_scrollEnabled) {
        return 0;
    }
    i32 delta = g_scrollDelta;
    i32 clock = g_scrollClock;
    u32 elapsed = (u32)clock - (u32)t->m_14;
    if (elapsed < (u32)t->m_18) {
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

// SIZE metadata for the .cpp-local engine views (CChatBox lives in ChatBox.h).
SIZE_UNKNOWN(CChatAnim);
SIZE_UNKNOWN(CChatCatalog);
SIZE_UNKNOWN(CChatListNode);
SIZE_UNKNOWN(CChatMap);
SIZE_UNKNOWN(CChatPage);
SIZE_UNKNOWN(CChatPoker);
SIZE_UNKNOWN(CChatRoster);
SIZE_UNKNOWN(CChatSprite);
SIZE_UNKNOWN(CChatTimer);
SIZE_UNKNOWN(CMenuRenderSet);
SIZE_UNKNOWN(CMenuSurf);
SIZE_UNKNOWN(CMenuSurfSrc);

// --- vtable catalog ---
