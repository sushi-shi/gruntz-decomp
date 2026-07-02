// SoundStreamFree.cpp - SoundStream::Free (0x137740): drain the +0x94 streaming-
// voice list, then Shutdown. SoundStream is the Dsndmgr streaming-DirectSound class
// (retail vftable 0x5ef6ec; see include/Dsndmgr/SoundStream.h). It inherits the
// voice-list head at +0x94 from SoundDevice; each StreamVoiceNode is linked via its
// +0x04 word biased +4, so a head is recovered as (head - 4) and a null head stays
// null.
//
// Free drains the +0x94 list (DestroyVoice pops the head each pass, so the field is
// re-read every iteration) then runs Shutdown. DestroyVoice (0x1379d0) and Shutdown
// (0x136690, the inherited SoundDevice::Shutdown) are external __thiscall helpers,
// modeled with no body so their `call rel32` displacements reloc-mask. Only the
// +0x94 offset + the (head - 4) recovery are load-bearing.
#include <rva.h>

#include <Ints.h>

struct StreamVoiceNode;

class SoundStream {
public:
    void Free();
    void DestroyVoice(StreamVoiceNode* voice); // 0x1379d0  unlink+release a voice off +0x94
    void Shutdown();                           // 0x136690  inherited SoundDevice::Shutdown

    char m_pad00[0x94];
    void* m_94; // +0x94  voice list head (stores node + 4)
};

// 0x137740 - drain the +0x94 voice list, then shut the device down.
RVA(0x00137740, 0x3e)
void SoundStream::Free() {
    for (StreamVoiceNode* p = m_94 ? (StreamVoiceNode*)((char*)m_94 - 4) : 0; p != 0;
         p = m_94 ? (StreamVoiceNode*)((char*)m_94 - 4) : 0) {
        DestroyVoice(p);
    }
    Shutdown();
}
