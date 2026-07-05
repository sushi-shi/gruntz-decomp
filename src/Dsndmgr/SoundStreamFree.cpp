// SoundStreamFree.cpp - SoundStream::Free (0x137740): drain the owned per-stream
// voice list, then run the base device Shutdown. SoundStream is the Dsndmgr
// streaming-DirectSound class (retail vftable 0x5ef6ec, derives SoundDevice); see
// <Dsndmgr/SoundStream.h> for the layout. Its voice list is the typed DSoundList
// m_voices at +0x94; each StreamVoice threads through its DSoundLink m_link
// (biased +4 MFC-POSITION-style, recovered with elemOf<>).
//
// The list HEAD is the inherited SoundDevice::m_instanceHead (+0x94); only the tail
// lives in the derived class. Free reaps the list (DestroyVoice pops the head each
// pass, so m_instanceHead is re-read every iteration) then runs the inherited
// SoundDevice::Shutdown. Both callees are reloc-masked real named symbols
// (SoundStream::DestroyVoice 0x1379d0, SoundDevice::Shutdown 0x136690).
#include <Dsndmgr/SoundStream.h>
#include <rva.h>

// 0x137740 - drain the voice list, then shut the device down.
RVA(0x00137740, 0x3e)
void SoundStream::Free() {
    for (StreamVoice* p = elemOf<StreamVoice>(m_instanceHead); p != 0;
         p = elemOf<StreamVoice>(m_instanceHead)) {
        DestroyVoice(p);
    }
    Shutdown();
}
