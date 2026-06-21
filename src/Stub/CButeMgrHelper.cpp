#include <rva.h>
// CButeMgrHelper.cpp - engine-label stubs for CButeMgrHelper (reloc-correlation).

class CButeMgrHelper {
public:
    void FuncA();
    void FuncB();
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x169be0, 0x13)
void CButeMgrHelper::FuncA() {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x169d70, 0x5a)
void CButeMgrHelper::FuncB() {}
