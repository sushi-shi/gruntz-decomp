// QueueDrainHost.cpp - CQueueDrainHost::Drain (0x31250), carved out of the conflated
// DDrawSubMgr.cpp (operation REHOME, package D8). FOREIGN to the DDraw submgr obj it
// was parked in (a queue-drain probe, @orphan - owning class unrecovered); its retail
// .text sits at 0x31250, ~1.2 MB before that block - a separate obj. Interleaver home
// (RVA-neighbour caller unit): src/Gruntz/BattlezMapConfig.cpp; homing there is
// deferred (cross-TU class decl + identity recovery).
//
// Walks the singly-linked list at this+0x68, popping each head node; returns the
// first node-data object whose slot +0x20 probe yields 5. Empty/exhausted -> 0.
// __thiscall, no args. Field names are placeholders; offsets + code bytes load-bearing.
#include <rva.h>
#include <Wap32/Object.h> // CObject (the probed node-data base)

class CQueueProbeData;
class CQueueProbeNode {
public:
    CQueueProbeNode* m_next; // +0x00
    char m_pad04[0x04];      // +0x04
    CQueueProbeData* m_data; // +0x08 -> probed object
};
SIZE_UNKNOWN(CQueueProbeNode);
class CQueueProbeData : public CObject {
public:
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual i32 Probe20(); // +0x20 status probe
};
SIZE_UNKNOWN(CQueueProbeData);
class CQueueDrainHost {
public:
    void* Drain_031250();
    char m_pad00[0x68];      // +0x00 .. +0x67
    CQueueProbeNode* m_head; // +0x68 list head
};
SIZE_UNKNOWN(CQueueDrainHost);

// @early-stop
// loop-top member re-read wall - retail re-loads `mov eax,[edi+0x68]` at the loop
// top (a redundant load the call clobbers) and merges the empty/exhausted zero
// epilogues into one xor-first tail; our cl carries the head pointer across the
// back-edge (CSE) and splits the epilogues. Logic/CFG/offsets exact
// (docs/patterns/reread-member-view-pointer.md).
RVA(0x00031250, 0x33)
void* CQueueDrainHost::Drain_031250() {
    while (m_head != 0) {
        CQueueProbeNode* head = m_head;
        m_head = head->m_next;
        CQueueProbeData* data = head->m_data;
        if (data->Probe20() == 5) {
            return data;
        }
    }
    return 0;
}
