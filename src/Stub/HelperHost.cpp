#include <rva.h>
// HelperHost.cpp - engine-label stubs for HelperHost (reloc-correlation).

class HelperHost {
public:
    int Helper_164790(int, int);
    void Helper_166040(int, int);
};
// @confidence: high
// @source: reloc-correlation (3 callers)
// @stub
RVA(0x00164790, 0x41)
int HelperHost::Helper_164790(int, int) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00166040, 0x66)
void HelperHost::Helper_166040(int, int) {}
