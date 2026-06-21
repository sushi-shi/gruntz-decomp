#include <rva.h>
// CStatusBarMgr.cpp - engine-label stubs for CStatusBarMgr (reloc-correlation).

// CStatusBarItem2 is defined in CStatusBarItem2.cpp (same aggregate TU); its
// helpers (Sub3f0/Inner560/740/880) and m_50 are reused by GetItem.
class CStatusBarItem2;

// Intrusive list link embedded in each item at +0x44.
struct SBLink {
    void* m_0;
    void* m_4;
};

// Traversal node: next at +0, item at +8.
struct SBNode {
    SBNode* m_0; // next
    char m_pad4[4];
    CStatusBarItem2* m_8; // item
};

// Embedded 2-pointer list head (head at +0, tail at +4).
struct SBList {
    SBNode* m_head;
    SBNode* m_tail;
    void Unlink(SBLink*); // 0x1391e0 thiscall
    void Append(SBLink*); // 0x139110 thiscall
};

struct SBMgrOwner {
    char m_pad[0x78];
    int m_78; // gate
};

class CStatusBarMgr {
public:
    class CStatusBarItem2* GetItem();

private:
    char m_pad0[0x10];
    SBMgrOwner* m_10; // [+0x10]
    char m_pad14[0x04];
    int m_18; // [+0x18]
    int m_1c; // [+0x1c]
    int m_20; // [+0x20]
    char m_pad24[0x58 - 0x24];
    SBList m_58; // [+0x58] embedded list head

    CStatusBarItem2* Create(int); // 0x135c20 thiscall
};

// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x135d70, 0x92)
class CStatusBarItem2* CStatusBarMgr::GetItem() {
    if (!m_10->m_78) {
        return 0;
    }
    SBNode* node = m_58.m_head;
    if (node) {
        while (1) {
            if (node->m_8->m_50 && node->m_8->Sub3f0() == 0) {
                break;
            }
            node = node->m_0;
            if (!node) {
                break;
            }
        }
    }
    CStatusBarItem2* found;
    if (!node) {
        found = 0;
    } else {
        found = node->m_8;
    }
    if (found) {
        found->Inner560(m_20);
        found->Inner740(m_1c);
        found->Inner880(m_18);
    }
    if (!found) {
        found = Create(1);
        if (!found) {
            return found;
        }
    }
    m_58.Unlink((SBLink*)((char*)found + 0x44));
    m_58.Append((SBLink*)((char*)found + 0x44));
    return found;
}
