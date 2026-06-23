#ifndef GRUNTZ_STUB_CRANDOMAMBIENTSOUND_H
#define GRUNTZ_STUB_CRANDOMAMBIENTSOUND_H
#include <rva.h>
#include <Stub/CAmbientSound.h>
// CRandomAmbientSound : CAmbientSound (RTTI). sizeof 0x58.
class CRandomAmbientSound : public CAmbientSound {
public:
    ~CRandomAmbientSound();
    char m_size_pad[0x18]; // own region over CAmbientSound (0x40)
};
SIZE(CRandomAmbientSound, 0x58);
#endif
