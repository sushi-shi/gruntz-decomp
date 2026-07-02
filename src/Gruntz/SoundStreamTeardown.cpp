// SoundStreamTeardown.cpp - SoundStream::Free (0x137a80), which tears down the
// SoundStream's owned voice list at +0x94 (the inherited SoundDevice instance-list
// head; each StreamVoiceNode chains through +0x04 biased +4 MFC-POSITION-style, so a
// node is recovered as (link - 4)). (Was the "MinervaInner" trace placeholder; the
// callees prove the class: 0x136de0 = SoundDevice::StopAll, and each node's +0x6c is
// the StreamVoiceNode feeder - cf. include/Dsndmgr/SoundStream.h.)
//
// Free walks the +0x94 list (following each node's +0x04 next link, NOT
// re-reading the head), destroys each node's embedded feeder sub-object at +0x6c,
// then runs StopAll. The feeder destroy (0x137f00) / StopAll (0x136de0) are external
// __thiscall helpers, modeled with no body so their `call rel32` displacements
// reloc-mask. Only the offsets + the (link - 4) recovery are load-bearing.
#include <rva.h>

#include <Ints.h>

// The per-node embedded sub-object destroyed during the walk (node + 0x6c).
struct StreamFeederView {
    void Destroy(); // 0x137f00
};

struct StreamVoiceNode {
    void* m_0;
    void* m_4; // +0x04  next link (stores node + 4)
    char m_pad8[0x6c - 0x8];
    StreamFeederView m_6c; // +0x6c  embedded sub-object
};

class SoundStream {
public:
    void Free();
    void Cleanup(); // 0x136de0  SoundDevice::StopAll (base)

    char m_pad00[0x94];
    void* m_94; // +0x94  voice-list head (stores node + 4)
};

// destroy every node's +0x6c feeder down the +0x94 voice list, then StopAll.
RVA(0x00137a80, 0x3d)
void SoundStream::Free() {
    StreamVoiceNode* node = m_94 ? (StreamVoiceNode*)((char*)m_94 - 4) : 0;
    while (node != 0) {
        node->m_6c.Destroy();
        void* nx = node->m_4;
        node = nx ? (StreamVoiceNode*)((char*)nx - 4) : 0;
    }
    Cleanup();
}

SIZE_UNKNOWN(StreamVoiceNode);
SIZE_UNKNOWN(StreamFeederView);
