#include <rva.h>
// CStatusBarMgrGetItem.cpp - CStatusBarMgr::GetItem in its OWN dedicated unit.
//
// Re-homed from src/Stub/CStatusBarMgr.cpp. NOT folded into src/Gruntz/CStatusBarMgr.cpp
// (the byte-exact LoadTabSprites TU): GetItem's CStatusBarMgr view conflicts with that
// TU's - offset 0x10 is a pointer here (m_10->m_78) but a base-x coordinate int there,
// and the +0x58 list head overlaps the +0x48 CPtrList. The two are effectively different
// classes sharing a mangled name, so a single folded layout would regress LoadTabSprites.
// A dedicated unit keeps GetItem's self-contained view isolated -> the conflict never
// arises. flags="eh" (== the engine_label_stubs base+/GX it came from; matching-neutral).

// The item type GetItem manages. Its real class is the DSNDMGR buffer wrapper
// (DirectSoundMgr, src/Dsndmgr/DirectSoundMgr.cpp): Sub3f0/Inner560/740/880 are
// IsPlaying/SetVolume/SetPan/SetFrequency (0x1353f0/0x135560/0x135740/0x135880)
// and m_50 is the play key. Declared minimally here (decls only); the helper
// calls lower to reloc-masked __thiscall calls to those real bodies.
class CStatusBarItem2 {
public:
    char m_pad0[0x50];
    i32 m_50;          // [+0x50]
    i32 Sub3f0();      // 0x1353f0 thiscall
    i32 Inner560(i32); // 0x135560 thiscall
    i32 Inner740(i32); // 0x135740 thiscall
    i32 Inner880(i32); // 0x135880 thiscall
};

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
    i32 m_78; // gate
};

class CStatusBarMgr {
public:
    class CStatusBarItem2* GetItem();

private:
    char m_pad0[0x10];
    SBMgrOwner* m_10; // [+0x10]
    char m_pad14[0x04];
    i32 m_18; // [+0x18]
    i32 m_1c; // [+0x1c]
    i32 m_20; // [+0x20]
    char m_pad24[0x58 - 0x24];
    SBList m_58; // [+0x58] embedded list head

    CStatusBarItem2* Create(i32); // 0x135c20 thiscall
};

// @confidence: med
// @source: reloc-correlation (1 caller)
// @early-stop
// shrink-wrapped callee-save push wall (~90%): logic + offsets + externs byte-exact.
// Retail saves only edi at entry and defers `push esi`/`push ebx` past the m_78 null
// guard (the early-out restores just edi); cl pushes all three upfront. Not source-
// steerable; docs/patterns/shrink-wrapped-callee-save-push.md. Final sweep.
RVA(0x00135d70, 0x92)
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
