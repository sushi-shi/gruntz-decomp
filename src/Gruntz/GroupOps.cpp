// GroupOps.cpp - two selection/group operations that hang off the g_gameReg
// singleton (?g_gameReg @0x64556c):
//
//   * (CenterOnGroup 0x7cf40 is merged into src/Gruntz/TriggerMgr.cpp per
//     bounding box of the selected gruntz over the map grid (+0x1c), centre the
//     view on the box midpoint, and (single selection) latch the picked cell.
//   * Broadcast (0x112080) - iterate a 0-terminated 24-entry key array (+0x2c),
//     look each key up in the +0x24 map, and for each matching member of the
//     resolved node's inner list run its destroy slot.
//
// Field names are placeholders; only the OFFSETS, the grid-hash (x*15 + y), the
// vtable SLOT offsets and the diagnostic ids are load-bearing.  The deeper engine
// leaves (map lookups, the centre helper, the diagnostics sink) are external /
// reloc-masked.

//     dossier 10b - it abuts CTriggerMgr::CenterSelectionGroup in the one
//     0x77f80 TU; only Broadcast remains here.)
#include <Gruntz/TileTriggerLogic.h> // CTileTriggerLogic::FindIndexByKey (the 0x9c family)
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>

// ===========================================================================
// Broadcast (0x112080)
// ===========================================================================
// The singleton itself (0x64556c) - the "g_gameRegDiag diagnostics sink" was a second
// NAME for it (and a C++-mangled one that nothing defines: an undefined-data defect on
// top of the phantom method). One object, one symbol: the extern "C" g_gameReg.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg; // 0x64556c

// A resolved map node (FOREIGN engine object): only vtable slot 3 (+0x0c, Prepare)
// is dispatched; slots 0/1/2 are unreconstructed engine code, declared structurally
// so Prepare lands at slot 3. Real polymorphic model - `node->Prepare()` lowers to
// the same `mov ecx,node; mov eax,[node]; call [eax+0xc]` virtual dispatch.
struct CFindNode {
    virtual void Vslot00(); // slot 0  +0x00
    virtual void Vslot01(); // slot 1  +0x04
    virtual void Vslot02(); // slot 2  +0x08
    virtual void Prepare(); // slot 3  +0x0c
    char m_pad04[0x10 - 0x04];
    i32 m_10; // +0x10 key
    i32 m_14; // +0x14 flag
};
// An inner-list member (FOREIGN): virtual Destroy at slot 0, plus a non-virtual
// Match (0x1fa5). Real polymorphic model (implicit vptr at +0x00).
struct CBcastMember {
    virtual void Destroy(); // slot 0  +0x00
    i32 Match(i32 key);     // 0x1fa5 (non-virtual)
};
struct CBcastListNode {
    CBcastListNode* m_next; // +0x00
    void* m_pad04;
    CBcastMember* m_8; // +0x08
};

struct CGroupBroadcast {
    char m_pad00[0x10];
    i32 m_10; // +0x10  compared with each node's key
    char m_pad14[0x24 - 0x14];
    CTileTriggerSwitchLogic* m_24; // +0x24
    char m_pad28[0x2c - 0x28];
    i32 m_2c[0x18];  // +0x2c  0-terminated key array
    i32 Broadcast(); // 0x112080
    void Init();     // 0x2e0f
};

// @early-stop
// 84% - regalloc wall: the 0-terminated key-array walk, per-key map Find, the
// inner match/destroy list loop and both diagnostic exits are byte-faithful; the
// residual is loop-induction / counter register colouring.  No EH frame.
RVA(0x00112080, 0x138)
i32 CGroupBroadcast::Broadcast() {
    Init();
    i32 counter = 0;
    i32* p = &m_2c[0];
    i32 i = 0;
    i32 done = 0;
    do {
        if (i >= 0x18) {
            return 1;
        }
        CFindNode* node = (CFindNode*)m_24->FindChild(*p, 4);
        if (node == 0) {
            g_gameReg->ReportError(0x80dd, 0x44f);
            return 0;
        }
        if (node->m_10 != m_10 && node->m_14 != 0) {
            node->Prepare();
            i32 any = 0;
            for (CBcastListNode* it = (CBcastListNode*)m_24->m_20; it != 0; it = it->m_next) {
                CBcastMember* o = it->m_8;
                if (o != 0 && ((CTileTriggerLogic*)o)->FindIndexByKey(node->m_10)) {
                    o->Destroy();
                    counter++;
                    any = 1;
                }
            }
            if (any == 0) {
                g_gameReg->ReportError(0x80de, 0x450);
                return 0;
            }
        }
        i32 next = p[1];
        p++;
        i++;
        if (next == 0) {
            done = 1;
        }
    } while (!done);
    return 1;
}

SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CFindNode);
SIZE_UNKNOWN(CBcastMember);
SIZE_UNKNOWN(CBcastListNode);
SIZE_UNKNOWN(CBcastMap);
SIZE_UNKNOWN(CGroupBroadcast);

// --- vtable catalog ---
