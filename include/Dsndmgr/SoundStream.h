#ifndef DSNDMGR_SOUNDSTREAM_H
#define DSNDMGR_SOUNDSTREAM_H

#include <rva.h>

#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundVoiceList.h> // DSoundLink (intrusive link) / DSoundList (the voice list)
#include <Dsndmgr/StreamVoice.h>    // the canonical per-stream voice (+ its embedded feeder)
#include <Gruntz/ParseSource.h>     // the RIFF/WAVE byte-reader (ParseWave / OpenStream)

class SoundStream;

class SoundStream : public SoundDevice {
public:
    SoundStream(); // 0x1376d0  base ctor + zero the voice list + stamp 0x5ef6ec
    virtual ~SoundStream()
        OVERRIDE; // 0x137710  reset vptr (0x5ef6ec) then ~SoundDevice; cl auto-emits
                  // the ??_G scalar-deleting dtor at 0x1376f0
    StreamVoice* CreateStreamBuffer(WaveFormatX* fmt, u32 bytes, i32 a, i32 b, i32 c);
    // 0x137780
    StreamVoice* OpenStream(CParseSource* src, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5);
    StreamVoice* PlayStream(CParseSource* src, i32 a2, i32 a3, i32 a4); // 0x137a30 open + resume
    // 0x137900
    void DestroyVoice(StreamVoice* voice); // 0x1379d0 (retire one instance-list voice;
                                           // TickSubManagers calls it directly on `this`)
    // Full free: reap (DestroyVoice) every voice off m_voices, then the base
    // SoundDevice::Shutdown. Called from CDDrawSurfaceMgr::FreeContext.
    void Free(); // 0x137740
    // Stop streaming: Pause each voice's embedded feeder (+0x6c), then the base
    // SoundDevice::StopAll. The wide pause/reset path (menu/level teardown).
    void Stop(); // 0x137a80
    // 0x137720 - play a defaulted sound (3rd flag = 0) bound to a window handle.
    // A real __thiscall SoundStream method (CDDrawSurfaceMgr::PlayDefaultSound
    // dispatches it on m_soundStream); its body ignores `this`.
    i32 PlaySoundDefaulted(void* hWnd, i32 flag); // 0x137720
    // Per-frame stream-voice tick: walk m_instanceHead, pump each embedded feeder,
    // poll IsPlaying, reprime/retire (DestroyVoice) idle voices. A SoundStream method
    // (its `this` calls the SoundStream-only DestroyVoice) defined in SoundStream.cpp.
    i32 TickSubManagers(i32 time); // 0x137ac0
    i32 ParseWave(
        CParseSource* src,
        WaveFormatX* fmtBuf,
        u32* outDataOff,
        u32* outDataLen
    ); // 0x137b70

    // SoundStream's own per-stream voice list {head@+0x94, tail@+0x98}. FLAG(shared):
    // the base SoundDevice ends at +0x94 (its ctor 0x136440 zeroes only through +0x90;
    // it never touches +0x94), so this list is entirely the derived class's.
    DSoundList m_voices; // +0x94  owned per-stream voice list
};
SIZE(0x9c); // 0x94 SoundDevice base + DSoundList m_voices (8 B)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 g_ssLogEnabled; // 0x653c4c -> OutputDebugStringA
extern "C" i32 g_ssMsgBoxEnabled; // 0x653c50 -> MessageBox
extern "C" i32 g_ssBeepEnabled; // 0x653c54 -> startup beep
extern "C" i32 g_ssThirdEnabled; // 0x653c58 -> "any output" gate

#endif // DSNDMGR_SOUNDSTREAM_H
