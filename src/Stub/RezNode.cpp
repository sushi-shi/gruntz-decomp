#include <rva.h>
// RezNode.cpp - engine-label stubs for RezNode (reloc-correlation).

struct RezNode {
public:
    struct RezNode * Next();
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x1848b0, 0x47)
struct RezNode * RezNode::Next() { return 0; }
