// ChatBox.h - a two-row scrolling on-screen text/chat display (C:\Proj\Gruntz).
//
// Trace-discovered as Region_182ab0; the engine identity (LoadChatBoxSprite +
// the per-row scroll state + the message-node list) names it a chat/marquee box.
// Two text rows, each with: a CString font/asset key (m_44 / m_48), a "current
// message node" pointer (m_4c / m_64), and per-row horizontal-scroll counters
// (m_50..m_60 row0, m_68..m_78 row1). Owns a CObList of message nodes at m_24
// (head cached in the node-walk loops as m_28 = list+4), plus a parent/page back
// pointer at m_0 and a queued-node slot at m_40.
//
// Only offsets + code bytes are load-bearing; field names are placeholders
// recovered from the member writes in the ctor/methods.
#ifndef GRUNTZ_CHATBOX_H
#define GRUNTZ_CHATBOX_H

#include <Mfc.h>

// ---------------------------------------------------------------------------
// The message-node class (defined in another TU - its ctor/dtor/accessors at
// 0x1833a0 / 0x183990 / 0x1839d0 / 0x1840a0 / 0x1843f0 / 0x1844d0 / ...). Modeled
// here only enough to name the methods our ChatBox rows dispatch into, so their
// `mov ecx,this+0x40; call rel32` is reloc-masked. __thiscall by default.
// ---------------------------------------------------------------------------
class CChatNode;

// The on-screen "page"/owner object reached through m_0 (its message-text table
// is read in AdvanceRow0/AdvanceRow1 via m_0->m_28). External - opaque view.
struct CChatPage;

// ---------------------------------------------------------------------------
// CChatBox
// ---------------------------------------------------------------------------
class CChatNode;

class CChatBox {
public:
    void Init();                              // 0xa0280 - re-zero the rows (no list teardown)
    void Reset();                             // 0x182b30 - free the node list, re-zero the rows
    void Clear();                             // 0x182b60 - free node payloads, empty the list
    int Find(const char* s);                  // 0x182be0 - find a node by string key
    ~CChatBox();                              // 0x182bxx teardown
    int AddNode(void* node);                  // 0x182ba0
    int AttachNode(void* n);                  // 0x182da0
    int ReplaceNode(void* n);                 // 0x182dd0
    int AdvanceRow0(void* key, int x, int y); // 0x182df0
    int AdvanceRow1(void* key, int x, int y); // 0x182e60
    int Step(int dt);                         // 0x182ed0
    int Draw(int a0, int sprite, int arg2, int arg3); // 0x182f90
    int ScrollRow0();                                 // 0x183030
    int ScrollRow1();                                 // 0x1830b0
    int HitTest0(int x, int y);                       // 0x1831a0
    int HitTest1();                                   // 0x183210
    int HitTest2();                                   // 0x183230

    void* m_0; // +0x00 parent/page back pointer
    int m_4;   // +0x04
    char m_pad8[0x24 - 0x08];
    CPtrList m_24; // +0x24 message-node list (vptr+6 fields = 0x1c, head at +0x28)
    void* m_40;    // +0x40 queued/active node slot
    CString m_44;  // +0x44 row0 font/asset key
    CString m_48;  // +0x48 row1 font/asset key
    void* m_4c;    // +0x4c row0 current node
    int m_50;      // +0x50
    int m_54;      // +0x54
    int m_58;      // +0x58
    int m_5c;      // +0x5c
    int m_60;      // +0x60
    void* m_64;    // +0x64 row1 current node
    int m_68;      // +0x68
    int m_6c;      // +0x6c
    int m_70;      // +0x70
    int m_74;      // +0x74
    int m_78;      // +0x78
};

#endif // GRUNTZ_CHATBOX_H
