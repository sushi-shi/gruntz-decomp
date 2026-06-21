#include <rva.h>
// CFileImage.cpp - engine-label stubs for CFileImage (reloc-correlation).

class CFileImage {
public:
    void* DecodeBmp(char*, void*, unsigned int);
    void* DecodePcx(char*, void*, unsigned int);
    int DecodePcxData(void*, int, int, int, int);
    void* DecodePid(char*, void*, unsigned int, void*);
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x143fc0, 0x142)
void* CFileImage::DecodeBmp(char*, void*, unsigned int) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x144ee0, 0x225)
void* CFileImage::DecodePcx(char*, void*, unsigned int) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x1457a0, 0x22c)
int CFileImage::DecodePcxData(void*, int, int, int, int) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x145b10, 0x1b5)
void* CFileImage::DecodePid(char*, void*, unsigned int, void*) {
    return 0;
}
