// MgrListFind.cpp - a g_mgrSettings list scan (0xf0db0): walk the manager's
// +0x68->+0x4 node list for a payload whose (m_5c==0, m_54, m_58) matches the two
// query coordinates. Self-contained; g_mgrSettings is a reloc-masked DATA load.
#include <Ints.h>
#include <rva.h>

// The payload a node holds; only the three queried words matter.
struct MgrFindPayload {
    char m_pad0[0x54];
    i32 m_54; // +0x54
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
};

// A singly-linked node: +0x0 next, +0x8 payload.
struct MgrFindNode {
    MgrFindNode* m_next; // +0x00
    char m_pad4[4];
    MgrFindPayload* m_8; // +0x08
};

// The +0x68 sub-object holds the list head at +0x4.
struct MgrFindList {
    char m_pad0[4];
    MgrFindNode* m_4; // +0x04
};

// The settings manager (only its +0x68 list sub-object is touched here).
struct MgrFindRoot {
    char m_pad0[0x68];
    MgrFindList* m_68; // +0x68
};

extern MgrFindRoot* g_mgrSettings;
DATA(0x0024556c)
extern MgrFindRoot* g_mgrSettings; // ?g_mgrSettings (VA 0x64556c)

// ---------------------------------------------------------------------------
// 0xf0db0 (__cdecl) - true if a live (m_5c==0) node matches (a1,a2).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc-cascade wall (73%): the loop structure (cur=node; node=node->next;
// payload; nested m_5c==0 then v54/v58 compares) is byte-faithful, but retail
// reuses the dead payload register (eax) and the m_5c temp (edx) for the v54/v58
// loads, needing only esi/edi callee-saved; cl spends a 3rd callee-saved reg (ebx
// for v54), so the whole push/pop frame + the v54 compare diverge. A
// register-pressure coin-flip (linked-list-walk-node-eax-rotation.md family); not
// source-steerable. Logic 100% correct; deferred.
RVA(0x000f0db0, 0x48)
i32 MgrListFind(i32 a1, i32 a2) {
    MgrFindNode* node = g_mgrSettings->m_68->m_4;
    while (node) {
        MgrFindNode* cur = node;
        node = node->m_next;
        MgrFindPayload* p = cur->m_8;
        if (p->m_5c == 0) {
            i32 v54 = p->m_54;
            i32 v58 = p->m_58;
            if (v54 == a1 && v58 == a2) {
                return 1;
            }
        }
    }
    return 0;
}

SIZE_UNKNOWN(MgrFindList);
SIZE_UNKNOWN(MgrFindNode);
SIZE_UNKNOWN(MgrFindPayload);
SIZE_UNKNOWN(MgrFindRoot);
