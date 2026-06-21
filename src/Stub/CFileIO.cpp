#include <rva.h>
// CFileIO.cpp - engine-label stubs for CFileIO (reloc-correlation).

class CFileIO {
public:
    unsigned int GetLength();
};
// @confidence: high
// @source: reloc-correlation (5 callers)
// @stub
RVA(0x1bf505, 0x2d)
unsigned int CFileIO::GetLength() {
    return 0;
}
