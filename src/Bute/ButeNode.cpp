#include <rva.h>

#include <Bute/ButeStore.h>
#include <Bute/ButeValue.h>
#include <Bute/PTreeNode.h>
#include <Gruntz/String.h>

// @early-stop
RVA_COMPGEN(0x00174d50, 0x1e, ??_GCButeNode@@UAEPAXI@Z)
// @early-stop
// Scoring artifact, not a source defect: the switch's case bodies compile into a
// SECOND symbol next to the jump table, so objdiff pairs only the dispatch prologue
// against retail's whole function (delinker jump-table dup-symbol undercount).
RVA(0x00174df0, 0x7c)
void __cdecl ButeValueTeardown(void* pValue) {
    CButeValue* v = static_cast<CButeValue*>(pValue);
    switch (v->type) {
        case BUTE_STRING:
            delete static_cast<CString*>(v->pValue);
            break;
        case BUTE_DOUBLE:
        case BUTE_POINT:
            delete static_cast<double*>(v->pValue);
            break;
        case BUTE_INT:
        case BUTE_FLOAT:
        case BUTE_VECTOR:
            delete static_cast<i32*>(v->pValue);
            break;
        case BUTE_DWORD:
        case BUTE_RECT:
        case BUTE_RANGE:
            delete static_cast<u32*>(v->pValue);
            break;
    }
}

RVA(0x00174d00, 0x25)
CButeNode::CButeNode(i32 kind) : zPTree(&ButeValueTeardown, kind) {}

RVA(0x00174d70, 0x70)
CButeNode::~CButeNode() {}

RVA(0x00174de0, 0x9)
void ButeStoreFreeAdapter(void* p) {
    (static_cast<CButeNode*>(p))->CButeNode::~CButeNode();
}
