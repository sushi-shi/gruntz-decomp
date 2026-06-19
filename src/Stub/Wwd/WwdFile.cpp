#include "../../rva.h"
// WwdFile.cpp - engine-label stubs for WwdFile.

class WwdFile {
public:
    void ValidateMainBlock();
    void ReadPlaneObjects();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x03b470, 0x13a)
void WwdFile::ValidateMainBlock() {}

// @confidence: high
// @source: rez-trace
// @stub
RVA(0x162af0, 0x806)
void WwdFile::ReadPlaneObjects() {}
