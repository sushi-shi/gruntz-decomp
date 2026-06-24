// ChatBox.cpp - a two-row scrolling on-screen text/chat display (C:\Proj\Gruntz).
//
// Trace-discovered as Region_182ab0. Two text rows, each with a CString font key
// (m_44 / m_48), a "current message node" pointer (m_4c / m_64) and per-row
// horizontal-scroll counters (m_50.. / m_68..). Owns a CPtrList of message nodes
// (m_24, head at m_28) whose stored payloads it frees on Reset/teardown; a
// parent/page back pointer (m_0); and a queued-node slot (m_40).
//
// Only offsets + code bytes are load-bearing; field names are placeholders. The
// message-node accessor methods (Configure/scroll-step/etc.) live in another TU
// and are modeled here as no-body externs so their calls are reloc-masked.
#include <rva.h>

#include <Gruntz/ChatBox.h>

// ---------------------------------------------------------------------------
// External engine callees / globals (modeled with no body -> reloc-masked).
// ---------------------------------------------------------------------------

// The message-node class: the rows dispatch into it (ctor/dtor/accessors at
// 0x1833a0 / 0x183990 / 0x1839d0 / 0x1840a0 / 0x1843f0 / 0x1844d0 / 0x1832d0,
// node-walk at 0x183250). __thiscall.
struct CChatNode {
    void* vptr; // +0x00 vtable (measure at slot +0x14)
    char m_pad4[0x8 - 0x4];
    void* m_8;
    char m_padc[0x10 - 0xc];
    void* m_10; // page/context used by the lookup helpers
    char m_pad14[0x24 - 0x14];
    int m_24; // node kind (1/2 are the matchable rows)
    char m_pad28[0x64 - 0x28];
    int m_64;
    char m_pad68[0x6c - 0x68];

    ~CChatNode();               // 0x183250 - external dtor (CString members)
    CString GetKey();           // 0x1832d0 - returns the node key by value (= m_c)
    void Detach();              // 0x183990
    int Rebuild();              // 0x1839d0
    int HitTest1(int x, int y); // 0x1840a0
    int HitTest2();             // 0x1843f0
    int HitTest3();             // 0x1844d0
    int Measure();              // virtual, vtable slot +0x14
};

struct CChatCatalog;

// The on-screen page/owner reached through CChatBox::m_0. Its key->node catalog
// hangs off m_10; the scroll-step helpers reach a sprite roster via m_0->m_28.
struct CChatPage {
    char m_pad0[0x10 - 0x0];
    CChatCatalog* m_10; // -> the key->node catalog
    char m_pad14[0x28 - 0x14];
    void* m_28; // -> sprite roster (scroll-step)
};

// A view of the CPtrList node layout (CPtrList::CNode is protected): pNext/pPrev/
// data. The teardown walk follows pNext and frees each stored payload.
struct CChatListNode {
    CChatListNode* pNext;
    CChatListNode* pPrev;
    void* data;
};

// The animation/frame record the row-advance lookups return: a frame table m_14
// indexed by the current frame m_64, plus a clamp range m_64..m_68.
struct CChatAnim {
    void* vptr;
    char pad4[0x14 - 0x4];
    int* m_14; // +0x14 frame table
    char pad18[0x64 - 0x18];
    int m_64; // +0x64 current frame index
    int m_68; // +0x68 max frame
};

// The key->record map (CMapWordToOb-style Lookup at 0x1b8008 / 0x1b8438). The
// value type differs per instance (CChatAnim for the rows, CChatTimer for scroll),
// so the out-param is generic. __thiscall.
struct CChatMap {
    int Lookup(void* key, void** out); // BOOL Lookup(key, value&)
};

// The on-screen catalog reached through CChatPage::m_10; the key->node map lives
// at +0x10 inside it (the `add ecx,0x10` in the row-advance lookups).
struct CChatCatalog {
    char pad0[0x10];
    CChatMap m_10map;
    char pad14[0x64 - 0x14];
    int m_64; // current frame/index, read straight through the lookup result
};

extern "C" void RezFree(void* p); // 0x1b9b82

// The frame handle each row blits through (0x153790). __thiscall.
struct CChatFrame {
    void Blit(int a, int x, int y, int b); // 0x153790
};

// The font/sprite passed into Draw: anchor coords m_44/m_48 (0xeeeeeeee = "use the
// caller's fallback coords") and a virtual Measure() at vtable slot +0x14 (index 5).
struct CChatSprite {
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual int Measure(); // slot +0x14
    char pad4[0x44 - 0x4];
    int m_44; // +0x44 anchor x
    int m_48; // +0x48 anchor y
};

// The horizontal-scroll edge state read by the two scroll-step methods.
DATA(0x0061ab20)
extern int g_scrollEnabled; // 0x61ab20
DATA(0x0061ab24)
extern int g_scrollDelta; // 0x61ab24
DATA(0x006bf3c0)
extern int g_scrollClock; // 0x6bf3c0

// The sprite poke target hung off a scroll-timer's m_10 (0x1360d0). __thiscall.
struct CChatPoker {
    int Poke(int a, int b, int c, int d); // 0x1360d0
};

// The per-row scroll timer record the scroll-step lookups return: a sprite poke
// target m_10, a last-tick m_14, and an interval m_18.
struct CChatTimer {
    char pad0[0x10];
    CChatPoker* m_10; // +0x10 sprite poke target
    int m_14;         // +0x14 last tick the row scrolled at
    int m_18;         // +0x18 scroll interval
};

// The on-screen sprite roster reached via CChatPage::m_28: a key->timer map at
// +0x10 and a "busy" gate at +0x30.
struct CChatRoster {
    char pad0[0x10];
    CChatMap m_10; // +0x10 key->timer map
    char pad14[0x30 - 0x14];
    int m_30; // +0x30 busy gate
};

// ===========================================================================
// CChatBox
// ===========================================================================

// 0xa0280 - re-zero both rows (no list teardown; Reset minus the Clear()).
RVA(0x000a0280, 0x2b)
void CChatBox::Init() {
    m_0 = 0;
    m_4 = 0;
    m_40 = 0;
    m_4c = 0;
    m_64 = 0;
    m_50 = 0;
    m_68 = 0;
    m_44.Empty();
    m_48.Empty();
}

// 0xa0360 - destructor lives in ChatBoxDtor.cpp (the /GX EH-frame TU; it is the
// only method here that needs the frame, so the rest stay frameless under base).

// 0x182b30 - free the node list, re-zero both rows.
RVA(0x00182b30, 0x30)
void CChatBox::Reset() {
    Clear();
    m_0 = 0;
    m_4 = 0;
    m_40 = 0;
    m_4c = 0;
    m_64 = 0;
    m_50 = 0;
    m_68 = 0;
    m_44.Empty();
    m_48.Empty();
}

// 0x182b60 - free every node's owned payload, empty the list, clear the queue slot.
RVA(0x00182b60, 0x3e)
void CChatBox::Clear() {
    CChatListNode* node = (CChatListNode*)m_24.GetHeadPosition();
    while (node) {
        CChatListNode* cur = node;
        node = node->pNext;
        CChatNode* payload = (CChatNode*)cur->data;
        if (payload) {
            payload->~CChatNode();
            RezFree(payload);
        }
    }
    m_24.RemoveAll();
    m_40 = 0;
}

// 0x182ba0 - append a node to the list; first node also becomes the active one.
RVA(0x00182ba0, 0x35)
int CChatBox::AddNode(void* node) {
    if (!node) {
        return 0;
    }
    m_24.AddTail(node);
    if (!m_40) {
        AttachNode(node);
    }
    return 1;
}

// @early-stop
// regalloc + stack-slot wall: body byte-exact (advance-first node walk + inline
// strcmp + GetKey-by-value), but MSVC reserves an extra `push ecx` slot and pins
// the node walk in a different callee-saved reg (ebp vs edi) than retail; logic
// is complete. ~74%.
// 0x182be0 - find the message node whose key matches s (linear scan + strcmp).
RVA(0x00182be0, 0x8d)
int CChatBox::Find(const char* s) {
    CChatListNode* node = (CChatListNode*)m_24.GetHeadPosition();
    while (node) {
        CChatListNode* cur = node;
        node = node->pNext;
        CChatNode* payload = (CChatNode*)cur->data;
        if (payload) {
            CString key = payload->GetKey();
            if (strcmp(key, s) == 0) {
                return (int)payload;
            }
        }
    }
    return 0;
}

// 0x182da0 - make `n` the active node (detach + rebuild it).
RVA(0x00182da0, 0x2a)
int CChatBox::AttachNode(void* n) {
    if (!n) {
        return 0;
    }
    m_40 = n;
    ((CChatNode*)n)->Detach();
    ((CChatNode*)m_40)->Rebuild();
    return 1;
}

// 0x182dd0 - find a node by key and make it active.
RVA(0x00182dd0, 0x19)
int CChatBox::ReplaceNode(void* n) {
    return AttachNode((void*)Find((const char*)n));
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; the residual
// is only the differently-named Lookup extern (0x1b8008, another TU's CMap). ~95%.
// 0x182df0 - advance row0 to the message keyed by `key`; cache its frame state.
RVA(0x00182df0, 0x69)
int CChatBox::AdvanceRow0(void* key, int x, int y) {
    if (!m_0) {
        return 0;
    }
    CChatAnim* a = 0;
    ((CChatPage*)m_0)->m_10->m_10map.Lookup(key, (void**)&a);
    m_4c = a;
    if (!a) {
        return 0;
    }
    m_50 = a->m_14[a->m_64];
    m_60 = a->m_64;
    m_54 = x;
    m_58 = x;
    m_5c = y;
    return 1;
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; residual is
// only the differently-named Lookup extern (0x1b8008, another TU's CMap). ~95%.
// 0x182e60 - advance row1 to the message keyed by `key`; cache its frame state.
RVA(0x00182e60, 0x69)
int CChatBox::AdvanceRow1(void* key, int x, int y) {
    if (!m_0) {
        return 0;
    }
    CChatAnim* a = 0;
    ((CChatPage*)m_0)->m_10->m_10map.Lookup(key, (void**)&a);
    m_64 = a;
    if (!a) {
        return 0;
    }
    m_68 = a->m_14[a->m_64];
    m_78 = a->m_64;
    m_6c = x;
    m_70 = x;
    m_74 = y;
    return 1;
}

// @early-stop
// regalloc wall: body byte-exact (unsigned counter compare, clamp+wrap of both
// rows' frame indices), but retail holds the row node in eax with the counter in
// edx, whereas MSVC swaps them here; 1-register phase shift only. ~89%.
// 0x182ed0 - per-frame advance of both rows' scroll counters & frame indices.
RVA(0x00182ed0, 0xbc)
int CChatBox::Step(int delta) {
    CChatAnim* a = (CChatAnim*)m_4c;
    if (a) {
        if ((unsigned)m_58 > (unsigned)delta) {
            m_58 -= delta;
        } else {
            m_58 = m_54;
            int f = m_60 + 1;
            m_60 = f;
            int v;
            if (f >= a->m_64 && f <= a->m_68) {
                v = a->m_14[f];
            } else {
                v = 0;
            }
            m_50 = v;
            if (v == 0) {
                m_50 = a->m_14[a->m_64];
                m_60 = a->m_64;
            }
        }
    }
    CChatAnim* b = (CChatAnim*)m_64;
    if (b) {
        if ((unsigned)m_70 > (unsigned)delta) {
            m_70 -= delta;
            return 1;
        }
        m_70 = m_6c;
        int f = m_78 + 1;
        m_78 = f;
        int v;
        if (f >= b->m_64 && f <= b->m_68) {
            v = b->m_14[f];
        } else {
            v = 0;
        }
        m_68 = v;
        if (v == 0) {
            m_68 = b->m_14[b->m_64];
            m_78 = b->m_64;
        }
    }
    return 1;
}

// @early-stop
// reloc-masked plateau: instruction stream byte-identical to retail; residual is
// only the differently-named Blit extern (0x153790) + the virtual Measure slot.
// ~95%.
// 0x182f90 - blit both rows' current frames, centered under the sprite anchor.
RVA(0x00182f90, 0x92)
int CChatBox::Draw(int a0, int sprite_, int arg2, int arg3) {
    CChatSprite* sprite = (CChatSprite*)sprite_;
    if (!sprite) {
        return 0;
    }
    int anchorX, anchorY;
    if (sprite->m_44 != (int)0xeeeeeeee) {
        anchorY = sprite->m_48;
        anchorX = sprite->m_44;
    } else {
        anchorY = arg3;
        anchorX = arg2;
    }
    if (m_50) {
        int x = -(sprite->Measure() / 2) - m_5c + anchorX;
        ((CChatFrame*)m_50)->Blit(arg2, x, anchorY, 0);
    }
    if (m_68) {
        int x = sprite->Measure() / 2 + m_74 + anchorX;
        ((CChatFrame*)m_68)->Blit(arg2, x, anchorY, 0);
    }
    return 1;
}

// @early-stop
// shrink-wrap + scheduling wall: body byte-exact, but retail DEFERS the
// `push edi/esi` past the empty-key guard (and uses 2 callee-saved regs for the
// elapsed-time compare); MSVC saves them in the prologue, shifting the whole BB
// layout. Logic complete. ~63%.
// 0x183030 - scroll row0's sprite one tick if its scroll interval has elapsed.
RVA(0x00183030, 0x7b)
int CChatBox::ScrollRow0() {
    if (m_44.GetLength() == 0) {
        return 0;
    }
    CChatRoster* roster = (CChatRoster*)((CChatPage*)m_0)->m_28;
    if (roster->m_30) {
        return 0;
    }
    CChatTimer* t = 0;
    roster->m_10.Lookup((void*)(const char*)m_44, (void**)&t);
    if (!t) {
        return 0;
    }
    if (!g_scrollEnabled) {
        return 0;
    }
    int delta = g_scrollDelta;
    int clock = g_scrollClock;
    unsigned elapsed = (unsigned)clock - (unsigned)t->m_14;
    if (elapsed < (unsigned)t->m_18) {
        return 0;
    }
    t->m_14 = clock;
    t->m_10->Poke(delta, 0, 0, 0);
    return 0;
}

// @early-stop
// shrink-wrap + scheduling wall (same as ScrollRow0): body byte-exact, retail
// defers the `push edi/esi` past the empty-key guard. Logic complete. ~63%.
// 0x1830b0 - scroll row1's sprite one tick if its scroll interval has elapsed.
RVA(0x001830b0, 0x7b)
int CChatBox::ScrollRow1() {
    if (m_48.GetLength() == 0) {
        return 0;
    }
    CChatRoster* roster = (CChatRoster*)((CChatPage*)m_0)->m_28;
    if (roster->m_30) {
        return 0;
    }
    CChatTimer* t = 0;
    roster->m_10.Lookup((void*)(const char*)m_48, (void**)&t);
    if (!t) {
        return 0;
    }
    if (!g_scrollEnabled) {
        return 0;
    }
    int delta = g_scrollDelta;
    int clock = g_scrollClock;
    unsigned elapsed = (unsigned)clock - (unsigned)t->m_14;
    if (elapsed < (unsigned)t->m_18) {
        return 0;
    }
    t->m_14 = clock;
    t->m_10->Poke(delta, 0, 0, 0);
    return 0;
}

// 0x1831a0 - forward a hit-test to the active node (slot 0x1840a0).
RVA(0x001831a0, 0x24)
int CChatBox::HitTest0(int x, int y) {
    CChatNode* n = (CChatNode*)m_40;
    if (!n) {
        return 0;
    }
    return n->HitTest1(x, y) != 0;
}

// 0x183210 - forward a hit-test to the active node (slot 0x1843f0).
RVA(0x00183210, 0x16)
int CChatBox::HitTest1() {
    CChatNode* n = (CChatNode*)m_40;
    if (!n) {
        return 0;
    }
    return n->HitTest2() != 0;
}

// 0x183230 - forward a hit-test to the active node (slot 0x1844d0).
RVA(0x00183230, 0x16)
int CChatBox::HitTest2() {
    CChatNode* n = (CChatNode*)m_40;
    if (!n) {
        return 0;
    }
    return n->HitTest3() != 0;
}
