#include <rva.h>
// CButeTree.cpp - engine-label stub for CButeTree.
//
// The shared find-by-key store node every CButeMgr getter funnels through
// (outer Find(tag) on m_tree, inner Find(key) on the tag sub-tree). Declared as
// a reloc-masked external no-body call in ButeMgr.cpp; this stub gives its entry
// an address so the getters' `call Find` relocs name the real symbol instead of
// the delinker's FUN_<rva> placeholder.
//
// Address recovered by reloc-correlation (gruntz.analysis.extern_harvest): 10
// matched callers (GetInt/GetIntDef/GetDword/... the getter funnel) all resolve
// their Find call to 0x16d190 - unanimous consensus.

class CButeTree {
public:
    void *Find(const char *key);
    void  Insert(const char *key, void *pNode);
};

// @confidence: high
// @source: reloc-correlation (10 callers, unanimous)
// @stub
RVA(0x16d190, 0x101)
void *CButeTree::Find(const char *) { return 0; }

// @confidence: med
// @source: reloc-correlation
// @stub
RVA(0x16db90, 0x206)
void CButeTree::Insert(const char *, void *) {}
