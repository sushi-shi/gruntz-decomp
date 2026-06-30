// VoldemortObj.cpp - the teardown of an engine object that owns an intrusive list
// at +0x94 (re-homed from src/Stub/VoldemortObj.cpp). The list head stores
// (node + 4), so each head is recovered as (head - 4); a null head stays null.
//
// Free drains the +0x94 list (RemoveNode pops the head each pass, so the field is
// re-read every iteration) then runs the final cleanup. RemoveNode (0x1379d0) /
// Cleanup (0x136690) are external __thiscall helpers, modeled with no body so
// their `call rel32` displacements reloc-mask. Only the +0x94 offset + the
// (head - 4) recovery are load-bearing; names are placeholders.
#include <rva.h>

#include <Ints.h>

struct VoldemortNode;

class VoldemortObj {
public:
    void Free();
    void RemoveNode(VoldemortNode* node); // 0x1379d0  pops node off the +0x94 list
    void Cleanup();                       // 0x136690  final teardown

    char m_pad00[0x94];
    void* m_94; // +0x94  list head (stores node + 4)
};

// 0x137740 - drain the +0x94 list, then run the final cleanup.
RVA(0x00137740, 0x3e)
void VoldemortObj::Free() {
    for (VoldemortNode* p = m_94 ? (VoldemortNode*)((char*)m_94 - 4) : 0; p != 0;
         p = m_94 ? (VoldemortNode*)((char*)m_94 - 4) : 0) {
        RemoveNode(p);
    }
    Cleanup();
}
