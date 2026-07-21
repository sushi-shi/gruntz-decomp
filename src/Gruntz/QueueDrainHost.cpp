#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h> // the real collection class (ex the CQueueDrainHost view)
#include <Gruntz/UserLogic.h>         // ::CGameObject (the payload; GetClassId is vtable slot 8)

// @early-stop
// loop-top member re-read wall - retail re-loads `mov eax,[edi+0x68]` at the loop
// top (a redundant load the call clobbers) and merges the empty/exhausted zero
// epilogues into one xor-first tail; our cl carries the head pointer across the
// back-edge (CSE) and splits the epilogues. Logic/CFG/offsets exact
// (docs/patterns/reread-member-view-pointer.md).
RVA(0x00031250, 0x33)
CGameObject* CDDrawChildGroup::Drain_031250() {
    while (m_scanCursor != 0) {
        CDDrawGroupNode* head = m_scanCursor;
        m_scanCursor = head->m_next;
        CGameObject* data = head->m_obj;
        if (data->GetClassId() == CLASSID_SERIALREF) {
            return data;
        }
    }
    return 0;
}
