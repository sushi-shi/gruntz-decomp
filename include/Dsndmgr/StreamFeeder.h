// StreamFeeder.h - the streaming feeder/pump sub-object embedded at
// StreamVoice+0x6c (Dsndmgr module, C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable
// 0x5ef6f0). The trace tagged this class "Timer_1380d0" after its Tick pump
// (0x1380d0), but it is the StreamFeeder referenced by SoundStream::OpenStream
// (feeder->FeederStart / FeederReset / window seed) - see SoundStream.h.
//
// The feeder owns a per-stream DirectSound buffer wrapper (m_8, a
// DirectSoundMgr-derived voice it Lock/Unlock-fills) and is armed with a data
// window (+0x2c..+0x40) over the source. Each Tick it copies the window into the
// secondary buffer (the circular Lock-fill at FillBuffer 0x137f30), wrapping +
// padding the tail with the silence byte (+0x24). Field names are placeholders;
// only OFFSETS + the emitted code bytes are load-bearing.
#ifndef DSNDMGR_STREAMFEEDER_H
#define DSNDMGR_STREAMFEEDER_H

// The streaming source reader the feeder pulls window bytes from (the same
// polymorphic reader SoundStream.h models as StreamSource; declared here so this
// TU is self-contained - SoundStream.h's wider view stays in its own TU). Read
// (0x139af0) returns the byte count actually read at `at` (-1 = current cursor).
struct FeederSource {
    int Read(void* buf, int n, int at); // 0x139af0
};

// The feeder's owner (m_4): a SoundDevice/SoundStream-family object that creates
// the streaming DirectSound buffer (0x1366f0) and reaps it (RemoveBuffer
// 0x136d80). Opaque here - its methods are reloc-masked __thiscall calls.
struct FeederOwner {
    // 0x1366f0 - build a streaming secondary buffer from the PCM format + size,
    // return the buffer wrapper (or 0). Caller-side: ecx=owner, (fmt, bytes, flags).
    void* CreateStreamBuf(void* fmt, unsigned long bytes, unsigned long flags);
    // 0x136d80 - reap voices + release the COM buffer + unlink one wrapper.
    void RemoveBuffer(void* buf);
};

// The feeder's per-stream buffer wrapper (m_8): a DirectSoundMgr-derived voice
// the feeder Lock/Unlock-fills + Stop/Pause/Resume-drives. Opaque; reloc-masked
// __thiscall calls. Offsets/sizes match DirectSoundMgr but the feeder only ever
// calls these methods, so a thin view is matching-neutral.
struct FeederBuf {
    int Lock(
        unsigned long off,
        unsigned long bytes,
        void** p1,
        unsigned long* n1,
        void** p2,
        unsigned long* n2,
        unsigned long flags
    );                                                                  // 0x136370
    int Unlock(void* p1, unsigned long n1, void* p2, unsigned long n2); // 0x1359c0
    int StopAndRewind();                                                // 0x135380
    int IsPlaying();      // 0x136270  (returns play-state)
    int Resume(int flag); // 0x135510  (resume/restart playback)
    int Tick(int now);    // 0x135a70  ... actually SetCurrentPosition pump
};

// The streaming feeder. Its own vftable (0x5ef6f0) is restamped by the ctor +
// dtor (a transitional reloc-masked DIR32 store: slot 0 = scalar-deleting dtor
// 0x11fec0, slot 1 = FeedData 0x137e10, slot 2 = OnDrain 0x137e20 - all external,
// so the class stays non-polymorphic and the compiler emits no vtable).
struct StreamFeeder {
    void* m_vtbl;       // +0x00  (retail vtable 0x5ef6f0; virtuals external)
    FeederOwner* m_4;   // +0x04  owner (SoundStream/SoundDevice)
    FeederBuf* m_8;     // +0x08  per-stream DirectSound buffer wrapper
    unsigned long m_c;  // +0x0c  read cursor into the buffer (write phase)
    unsigned long m_10; // +0x10  loop/buffer length
    unsigned long m_14; // +0x14  format / flags
    unsigned long m_18; // +0x18  armed flag
    unsigned long m_1c; // +0x1c  drained flag
    unsigned long m_20; // +0x20  pending-bytes accumulator
    unsigned char m_24; // +0x24  silence byte (0x80 for 8-bit PCM, else 0)
    char m_pad25[0x28 - 0x25];
    unsigned long m_28; // +0x28  last Tick time
    unsigned long m_2c; // +0x2c  source back-pointer (window)
    unsigned long m_30; // +0x30  loop flag
    unsigned long m_34; // +0x34  running source offset
    unsigned long m_38; // +0x38  window start offset
    unsigned long m_3c; // +0x3c  window length
    unsigned long m_40; // +0x40  window end (start+length)

    int SeedWindow(void* src, unsigned long off, unsigned long len); // 0x137340
    int CopyWindow(
        void* dst1,
        unsigned long n1,
        unsigned long* got1,
        void* dst2,
        unsigned long n2,
        unsigned long* got2
    );              // 0x137380 (slot-0 feed)
    StreamFeeder(); // 0x137cd0
    void Cleanup(); // 0x137cf0  (dtor body)
    int FeederStart(
        FeederOwner* owner,
        int arg2,
        unsigned long len,
        void* fmt,
        void* buf,
        int tickArg
    );                                                           // 0x137d10
    void FeederReset(int doStop);                                // 0x137dc0
    int Resume();                                                // 0x137ed0
    int Pause();                                                 // 0x137f00
    int FillBuffer(unsigned long writePos, unsigned long bytes); // 0x137f30

    // Tick (0x1380d0): sibling pump, external to this TU - reloc-masked.
    int TickPump(int now);
};

#endif // DSNDMGR_STREAMFEEDER_H
