#include <rva.h>
// HelperHost.cpp - engine-label stubs for HelperHost (reloc-correlation).

class HelperHost {
public:
    i32 Helper_164790(i32, i32);
    void Helper_166040(i32, i32);
};
// @confidence: high
// @source: reloc-correlation (3 callers)
// @stub
RVA(0x00164790, 0x41)
i32 HelperHost::Helper_164790(i32, i32) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00166040, 0x66)
void HelperHost::Helper_166040(i32, i32) {}
