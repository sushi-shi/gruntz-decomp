#include <rva.h>
// DirectSoundMgr.cpp - engine-label stub for the remaining DirectSoundMgr member.
// The thin IDirectSoundBuffer / IDirectSound wrapper run, the ctor, and the
// device-init methods have been migrated to the real TU
// (src/Dsndmgr/DirectSoundMgr.cpp). Only LockConvert (the Lock + 16->8-bit
// sample-conversion + Unlock data path) remains unmodeled here.

class DirectSoundMgr {
public:
    void ErrorThunk_135f40(int, int, int);
};

// @confidence: med
// @source: call-xref
// @stub
RVA(0x135f40, 0x169)
void DirectSoundMgr::ErrorThunk_135f40(int, int, int) {}
