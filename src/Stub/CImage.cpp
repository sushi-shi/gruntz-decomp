#include <rva.h>
// CImage.cpp - engine-label stubs for CImage (reloc-correlation).

class CImage {
public:
    int LoadBmp(char *, void *, void *);
    int LoadDefault(char *, void *, void *);
    int LoadPcx(char *, void *, void *);
    int LoadPid(char *, void *, void *);
    int LoadRid(char *, void *, void *);
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x175e40, 0x1b3)
int CImage::LoadBmp(char *, void *, void *) { return 0; }
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x1767d0, 0x64)
int CImage::LoadDefault(char *, void *, void *) { return 0; }
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x176190, 0x126)
int CImage::LoadPcx(char *, void *, void *) { return 0; }
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x1766a0, 0x126)
int CImage::LoadPid(char *, void *, void *) { return 0; }
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x176310, 0x126)
int CImage::LoadRid(char *, void *, void *) { return 0; }
