#include <rva.h>
// CFileIO.cpp - engine-label stubs for CFileIO (reloc-correlation).

class CFileIO {
public:
    unsigned int GetLength();
    void SaveGameFile(int);
};
// @confidence: high
// @source: reloc-correlation (5 callers)
// @stub
RVA(0x001bf505, 0x2d)
unsigned int CFileIO::GetLength() {
    return 0;
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000e4b60, 0x158)
void CFileIO::SaveGameFile(int) {}
