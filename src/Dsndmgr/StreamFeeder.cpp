// StreamFeeder.cpp - the streaming feeder/pump (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6f0). Embedded at
// StreamVoice+0x6c; the trace tagged it "Timer_1380d0" after its Tick pump
// (0x1380d0). It arms a data window over the source (SeedWindow/FeederStart),
// then each Tick copies that window into a streaming DirectSound secondary
// buffer (CopyWindow / FillBuffer), wrapping + silence-padding the tail.
//
// Field names are placeholders; offsets + emitted bytes are load-bearing.
#include <Dsndmgr/StreamFeeder.h>
#include <Win32.h>
#include <rva.h>
#include <string.h> // memset (inlined to rep stos)

// The feeder's virtuals (slot 0 dtor 0x11fec0, slot 1 FeedData 0x137e10, slot 2
// OnDrain 0x137e20) live in other TUs. Dispatch them through pointer-to-member
// types so the calls are __thiscall (this in ecx) - a single-inheritance pmf is
// just a 4-byte code pointer, so reinterpreting the raw vtable slot is exact.
typedef i32 (StreamFeeder::*FeederVFn)();
union VSlot {
    void* p;
    FeederVFn fn;
};

inline FeederVFn vslot(void* vtbl, i32 slot) {
    VSlot v;
    v.p = ((void**)vtbl)[slot];
    return v.fn;
}

// Slot 0 (0x11fec0) is the feed-into-two-regions virtual: (p1, n1, &got1, p2,
// n2, &got2) -> int. Same pmf-via-union trick for the __thiscall convention.
typedef i32 (StreamFeeder::*FeedRegionsFn)(void*, u32, u32*, void*, u32, u32*);
union VSlot0 {
    void* p;
    FeedRegionsFn fn;
};

inline FeedRegionsFn vslot0(void* vtbl) {
    VSlot0 v;
    v.p = ((void**)vtbl)[0];
    return v.fn;
}

// The feeder's retail vftable (0x5ef6f0), restamped by the ctor + dtor - a
// transitional reloc-masked DIR32 store while its virtuals (0x11fec0 / 0x137e10
// / 0x137e20) live in other TUs, so the class stays non-polymorphic.
DATA(0x001ef6f0)
extern void* const g_StreamFeederVtbl[];

// ---------------------------------------------------------------------------
// StreamFeeder::SeedWindow (0x137340, __thiscall, 3 args). Arm the data window
// (source + offset + length) over the stream, then prime via Tick(-1).
RVA(0x00137340, 0x33)
i32 StreamFeeder::SeedWindow(void* src, u32 off, u32 len) {
    if (src == 0) {
        return 0;
    }
    m_source = (u32)src;
    m_windowLength = len;
    m_windowStart = off;
    m_sourceOffset = off;
    m_windowEnd = off + len;
    TickPump(-1);
    return 1;
}

// ---------------------------------------------------------------------------
// StreamFeeder::CopyWindow (0x137380, __thiscall, 6 args - two
// (dst, n, *got) triples). Stream-copy up to `n` bytes into each destination
// region from the running source offset, reporting the byte count read in *got,
// and looping back to the window start at the end when the loop flag is set. Each
// chunk is read through StreamSource::Read (0x139af0) at the source back-pointer.
// This is the slot-0 feed virtual's body.
RVA(0x00137380, 0x10e)
i32 StreamFeeder::CopyWindow(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2) {
    if (dst1 != 0 && n1 > 0) {
        u32 want = n1;
        if (m_sourceOffset + n1 > m_windowEnd) {
            want = m_windowEnd - m_sourceOffset;
        }
        *got1 = ((FeederSource*)m_source)->Read(dst1, want, m_sourceOffset);
        m_sourceOffset += *got1;
        while (*got1 < n1 && m_loop != 0) {
            m_sourceOffset = m_windowStart;
            want = n1;
            if (m_windowStart + n1 > m_windowEnd) {
                want = m_windowEnd - m_windowStart;
            }
            *got1 = ((FeederSource*)m_source)->Read(dst1, want, m_sourceOffset);
            m_sourceOffset += *got1;
        }
    }
    if (dst2 != 0 && n2 > 0) {
        u32 want = n2;
        if (m_sourceOffset + n2 > m_windowEnd) {
            want = m_windowEnd - m_sourceOffset;
        }
        *got2 = ((FeederSource*)m_source)->Read(dst2, want, m_sourceOffset);
        m_sourceOffset += *got2;
        while (*got2 < n2 && m_loop != 0) {
            m_sourceOffset = m_windowStart;
            want = n2;
            if (m_windowStart + n2 > m_windowEnd) {
                want = m_windowEnd - m_windowStart;
            }
            *got2 = ((FeederSource*)m_source)->Read(dst2, want, m_sourceOffset);
            m_sourceOffset += *got2;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// StreamFeeder::StreamFeeder (0x137cd0, __thiscall). Stamp the vptr, zero the
// buffer/cursor/flag fields.
RVA(0x00137cd0, 0x1a)
StreamFeeder::StreamFeeder() {
    *(void**)this = (void*)g_StreamFeederVtbl;
    m_buffer = 0;
    m_armed = 0;
    m_bufferCursor = 0;
    m_playing = 0;
    m_lastTickTime = 0;
}

// ---------------------------------------------------------------------------
// StreamFeeder::Cleanup (0x137cf0, __thiscall - the dtor body). Restamp the
// vptr, tear down the armed buffer, clear m_buffer.
RVA(0x00137cf0, 0x20)
void StreamFeeder::Cleanup() {
    *(void**)this = (void*)g_StreamFeederVtbl;
    if (m_armed != 0) {
        FeederReset(1);
    }
    m_buffer = 0;
}

// ---------------------------------------------------------------------------
// StreamFeeder::FeederReset (0x137dc0, __thiscall, 1 arg). If armed, drain
// (Pause if drained, then the OnDrain virtual), optionally reap the buffer from
// the owner, and disarm.
RVA(0x00137dc0, 0x43)
void StreamFeeder::FeederReset(i32 doStop) {
    if (m_armed != 0) {
        if (m_playing != 0) {
            Pause();
        }
        (this->*vslot(m_vtbl, 2))(); // OnDrain (slot 2)
        if (doStop != 0) {
            m_owner->RemoveBuffer(m_buffer);
        }
        m_buffer = 0;
        m_armed = 0;
    }
}

// ---------------------------------------------------------------------------
// StreamFeeder::Resume (0x137ed0, __thiscall). If not already playing, resume
// the buffer and, if it reports playing, mark playback active.
RVA(0x00137ed0, 0x30)
i32 StreamFeeder::Resume() {
    if (m_playing != 0) {
        return 1;
    }
    m_buffer->Resume(1);
    i32 r = m_buffer->IsPlaying();
    if (r != 0) {
        m_playing = 1;
    }
    return r; // fall-through keeps the IsPlaying result in eax (no re-zero)
}

// ---------------------------------------------------------------------------
// StreamFeeder::Pause (0x137f00, __thiscall). If playing, stop+rewind the buffer
// and clear the playback flag; else nothing.
RVA(0x00137f00, 0x26)
i32 StreamFeeder::Pause() {
    if (m_playing == 0) {
        return 1;
    }
    i32 r = m_buffer->StopAndRewind();
    if (r != 0) {
        m_playing = 0;
    }
    return r; // fall-through keeps the StopAndRewind result in eax (no re-zero)
}

// ---------------------------------------------------------------------------
// StreamFeeder::FeederStart (0x137d10, __thiscall, 6 args, ret 0x18). Arm the
// feeder: record owner/length/format, derive the silence byte from the PCM bit
// depth, create (or adopt) the streaming buffer, arm, FeedData (slot 1) and
// prime via Tick.
// @early-stop
// regalloc eax/edx wall: retail pins len(arg3)->eax, fmt(arg4)->edx; MSVC here
// assigns the opposite pair (len->edx, fmt->eax). Body + control flow byte-exact,
// only the two-arg register choice differs - 96.8%, a register-allocation
// plateau (docs/patterns/zero-register-pinning.md family). Logic complete,
// deferred to the final sweep.
RVA(0x00137d10, 0xab)
i32 StreamFeeder::FeederStart(
    FeederOwner* owner,
    i32 arg2,
    u32 len,
    void* fmt,
    void* buf,
    i32 tickArg
) {
    m_bufferLength = len;
    m_owner = owner;
    m_format = (u32)fmt;
    m_playing = 0;
    if (*(u16*)((char*)fmt + 0xe) > 8) {
        m_silenceByte = 0;
    } else {
        m_silenceByte = 0x80;
    }
    if (buf == 0) {
        m_buffer = (FeederBuf*)owner->CreateStreamBuf(fmt, len, 0x100e0);
    } else {
        m_buffer = (FeederBuf*)buf;
    }
    if (m_buffer == 0) {
        return 0;
    }
    m_armed = 1;
    if ((this->*vslot(m_vtbl, 1))() == 0) { // FeedData (slot 1)
        FeederReset(1);
        return 0;
    }
    if (TickPump(tickArg) == 0) {
        FeederReset(1);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// StreamFeeder::FillBuffer (0x137f30, __thiscall, 2 args, ret 0x8). Lock the
// streaming secondary buffer at the write window, copy the source window into
// the (possibly wrapped) locked regions, silence-pad the unfilled tail, advance
// the buffer cursor with wraparound, and Unlock.
// @early-stop
// local-coalescing / frame-size wall: retail reuses dead local slots for the
// got1/got2 scratch (sub esp,0x10, 4 dwords); MSVC here keeps them distinct
// (sub esp,0x14, 5 dwords), shifting every Lock out-pointer slot + the [esp+N]
// local offsets. Instruction selection + control flow byte-exact; only the
// stack-slot assignment differs - 88.6%, the documented local-coalescing wall
// (docs/patterns/stack-buffer-size-drives-frame.md). Logic complete, deferred
// to the final sweep.
RVA(0x00137f30, 0x197)
i32 StreamFeeder::FillBuffer(u32 writePos, u32 bytes) {
    void* p1;
    u32 n1;
    void* p2;
    u32 n2;
    if (m_buffer->Lock(writePos, bytes, &p1, &n1, &p2, &n2, 0) == 0) {
        return 0;
    }
    u32 got1 = 0;
    u32 got2 = 0;
    if (m_pendingBytes == 0) {
        if ((this->*vslot0(m_vtbl))(p1, n1, &got1, p2, n2, &got2) == 0) {
            m_buffer->Unlock(p1, n1, p2, n2);
            return 0;
        }
    } else {
        got1 = 0;
        got2 = 0;
    }
    if (got1 < n1) {
        m_pendingBytes += n1 - got1;
        memset((char*)p1 + got1, m_silenceByte, n1 - got1);
    }
    if (got2 < n2) {
        m_pendingBytes += n2 - got2;
        memset((char*)p2 + got2, m_silenceByte, n2 - got2);
    }
    if (m_pendingBytes >= m_bufferLength) {
        Pause();
    }
    if (got2 != 0) {
        m_bufferCursor = got2;
    } else {
        m_bufferCursor = writePos + bytes;
    }
    if (m_bufferCursor >= m_bufferLength) {
        m_bufferCursor = 0;
    }
    m_buffer->Unlock(p1, n1, p2, n2);
    return 1;
}
