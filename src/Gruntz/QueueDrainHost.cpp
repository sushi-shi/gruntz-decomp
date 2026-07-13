// QueueDrainHost.cpp - CQueueDrainHost::Drain (0x31250), carved out of the conflated
// DDrawSubMgr.cpp (operation REHOME, package D8). FOREIGN to the DDraw submgr obj it
// was parked in; its retail .text sits at 0x31250, ~1.2 MB before that block - a
// separate obj. Interleaver home (RVA-neighbour caller unit): BattlezMapConfig.cpp.
//
// The class is the LEVEL's game-object collection - see <Gruntz/QueueDrainHost.h>,
// which is now the ONE shared shape (the three .cpp-local views here - CQueueDrainHost
// / CQueueProbeNode / CQueueProbeData - were the same object BattlezMapConfig.cpp
// re-modeled as SceneColl / SceneNode / SceneObj; all dissolved). The payload is a
// real ::CGameObject, so the "+0x20 probe" is its slot-8 GetTypeId() virtual.
//
// Pops each head node off the m_scan cursor; returns the first node payload whose
// GetTypeId() yields 5. Empty/exhausted -> 0. __thiscall, no args.
#include <rva.h>
#include <Gruntz/QueueDrainHost.h>
#include <Gruntz/UserLogic.h> // ::CGameObject (the payload; GetTypeId is vtable slot 8)

// @early-stop
// loop-top member re-read wall - retail re-loads `mov eax,[edi+0x68]` at the loop
// top (a redundant load the call clobbers) and merges the empty/exhausted zero
// epilogues into one xor-first tail; our cl carries the head pointer across the
// back-edge (CSE) and splits the epilogues. Logic/CFG/offsets exact
// (docs/patterns/reread-member-view-pointer.md).
RVA(0x00031250, 0x33)
CGameObject* CQueueDrainHost::Drain_031250() {
    while (m_scan != 0) {
        CQueueProbeNode* head = m_scan;
        m_scan = head->m_next;
        CGameObject* data = head->m_data;
        if (data->GetTypeId() == 5) {
            return data;
        }
    }
    return 0;
}
