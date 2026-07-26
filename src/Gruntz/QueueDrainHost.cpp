#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h> // the real collection class (ex the CQueueDrainHost view)
#include <Gruntz/UserLogic.h>         // ::CGameObject (the payload; GetClassId is vtable slot 8)

// @early-stop
// loop-top member re-read wall (88.4; was 37.8): the for(;;)+explicit-if form
// reproduces retail's rotation + merged zero epilogue; the LAST line is the
// entry-path re-read (retail loads m_scanCursor for the test AND again for head;
// cl fuses them BB-locally - permute no-change).
RVA(0x00031250, 0x33)
CGameObject* CDDrawChildGroup::Drain() {
    for (;;) {
        if (m_scanCursor == 0) {
            return 0;
        }
        CDDrawGroupNode* head = m_scanCursor;
        m_scanCursor = head->m_next;
        CGameObject* data = head->m_obj;
        if (data->GetClassId() == CLASSID_SERIALREF) {
            return data;
        }
    }
}
