#include <rva.h>
// CRegSink.cpp - engine-label stubs for CRegSink (reloc-correlation).

class CRegSink {
public:
    void Post(int, int);
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00003616, 0x5)
void CRegSink::Post(int, int) {}
