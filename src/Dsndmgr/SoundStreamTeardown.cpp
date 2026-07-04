// SoundStreamTeardown.cpp - SoundStream::Stop (0x137a80): stop streaming without a
// full free. It walks the owned per-stream voice list (head = the inherited
// SoundDevice::m_instanceHead at +0x94; each StreamVoiceNode chains through its
// DSoundLink m_link, recovered with elemOf<>), Pauses each node's embedded feeder
// sub-object (StreamVoiceFeeder at +0x6c), then runs the inherited
// SoundDevice::StopAll. (Was the "MinervaInner" trace placeholder and shared the
// SoundStream::Free mangled name with 0x137740; renamed Stop.)
//
// Both callees are reloc-masked real named symbols: StreamFeeder::Pause (0x137f00)
// and SoundDevice::StopAll (0x136de0). The wide menu/level teardown path calls this.
#include <Dsndmgr/SoundStream.h>
#include <rva.h>

// 0x137a80 - Pause every voice's feeder down the voice list, then StopAll.
RVA(0x00137a80, 0x3d)
void SoundStream::Stop() {
    StreamVoiceNode* node = elemOf<StreamVoiceNode>(m_instanceHead);
    while (node != 0) {
        node->m_feeder.Pause();
        node = elemOf<StreamVoiceNode>(node->m_link.m_next);
    }
    StopAll();
}
