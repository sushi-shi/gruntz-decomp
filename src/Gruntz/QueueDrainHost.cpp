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
        if (data->GetClassId() == 5) {
            return data;
        }
    }
    return 0;
}
