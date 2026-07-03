// StreamFeeder.cpp - the streaming feeder/pump (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6f0). Embedded at
// StreamVoice+0x6c; the trace tagged it "Timer_1380d0" after its Tick pump
// (0x1380d0). It arms a data window over the source (SeedWindow/FeederStart),
// then each Tick copies that window into a streaming DirectSound secondary
// buffer (CopyWindow / FillBuffer), wrapping + silence-padding the tail.
//
// Field names are placeholders; offsets + emitted bytes are load-bearing.
#include <Dsndmgr/SoundDevice.h> // SoundDevice::CreateBuffer / RemoveBuffer; DirectSoundMgr (via it)
#include <Dsndmgr/StreamFeeder.h>
#include <Win32.h>
#include <rva.h>
#include <string.h> // memset (inlined to rep stos)

// ALL-VTABLES phase: StreamFeeder is a REAL polymorphic base (see StreamFeeder.h).
// The slot dispatches use the declared virtuals directly (slot 0 Feed, slot 1
// FeedData, slot 2 OnDrain) - `this->Feed(...)` lowers to the same `mov eax,[this];
// call [eax+N]` the old pmf-via-union did. cl auto-emits ??_7StreamFeeder@@6B@
// (0x5ef6f0) and stamps the vptr in the ctor.

// ---------------------------------------------------------------------------
// StreamFeeder::SeedWindow (__thiscall, 3 args). Arm the data window
// (source + offset + length) over the stream, then prime via Tick(-1).
RVA(0x00137340, 0x33)
i32 StreamFeeder::SeedWindow(CParseSource* src, u32 off, u32 len) {
    if (src == 0) {
        return 0;
    }
    m_source = src;
    m_windowLength = len;
    m_windowStart = off;
    m_sourceOffset = off;
    m_windowEnd = off + len;
    TickPump(-1);
    return 1;
}

// ---------------------------------------------------------------------------
// StreamVoiceFeeder::Feed (was CopyWindow) - the derived voice-feeder's slot-0
// override (__thiscall, 6 args - two (dst, n, *got) triples). Stream-copy up to `n`
// bytes into each destination region from the running window cursor (m_sourceOffset),
// reporting the byte count read in *got, and looping back to the window start
// (m_windowStart) at the end when the loop flag (m_loop) is set. Each chunk is read
// through CParseSource::Read (0x139af0) at the source back-pointer (m_source).
RVA(0x00137380, 0x10e)
i32 StreamVoiceFeeder::Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2) {
    if (dst1 != 0 && n1 > 0) {
        u32 want = n1;
        if (m_sourceOffset + n1 > m_windowEnd) {
            want = m_windowEnd - m_sourceOffset;
        }
        *got1 = m_source->Read(dst1, want, m_sourceOffset);
        m_sourceOffset += *got1;
        while (*got1 < n1 && m_loop != 0) {
            m_sourceOffset = m_windowStart;
            want = n1;
            if (m_windowStart + n1 > m_windowEnd) {
                want = m_windowEnd - m_windowStart;
            }
            *got1 = m_source->Read(dst1, want, m_sourceOffset);
            m_sourceOffset += *got1;
        }
    }
    if (dst2 != 0 && n2 > 0) {
        u32 want = n2;
        if (m_sourceOffset + n2 > m_windowEnd) {
            want = m_windowEnd - m_sourceOffset;
        }
        *got2 = m_source->Read(dst2, want, m_sourceOffset);
        m_sourceOffset += *got2;
        while (*got2 < n2 && m_loop != 0) {
            m_sourceOffset = m_windowStart;
            want = n2;
            if (m_windowStart + n2 > m_windowEnd) {
                want = m_windowEnd - m_windowStart;
            }
            *got2 = m_source->Read(dst2, want, m_sourceOffset);
            m_sourceOffset += *got2;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// StreamFeeder::StreamFeeder (__thiscall). Stamp the vptr, zero the
// buffer/cursor/flag fields.
RVA(0x00137cd0, 0x1a)
StreamFeeder::StreamFeeder() {
    // cl auto-stamps ??_7StreamFeeder@@6B@ (0x5ef6f0) here.
    m_buffer = 0;
    m_armed = 0;
    m_bufferCursor = 0;
    m_drained = 0;
    m_lastTickMs = 0;
}

// ---------------------------------------------------------------------------
// StreamFeeder::Cleanup (__thiscall - the dtor body). Tear down the armed buffer,
// clear m_buffer.
// @early-stop
// vptr-restamp drop: Cleanup no longer emits the leading vptr reset (a plain method
// can't reference the cl-emitted ??_7StreamFeeder); the teardown body is unchanged.
RVA(0x00137cf0, 0x20)
void StreamFeeder::Cleanup() {
    if (m_armed != 0) {
        FeederReset(1);
    }
    m_buffer = 0;
}

// ---------------------------------------------------------------------------
// StreamFeeder::FeederReset (__thiscall, 1 arg). If armed, drain
// (Pause if drained, then the OnDrain virtual), optionally reap the buffer from
// the owner, and disarm.
RVA(0x00137dc0, 0x43)
void StreamFeeder::FeederReset(i32 doStop) {
    if (m_armed != 0) {
        if (m_drained != 0) {
            Pause();
        }
        OnDrain(); // slot 2 (virtual)
        if (doStop != 0) {
            // FLAG(shared, matcher-6): m_buffer (DirectSoundMgr, method-view) and
            // RemoveBuffer's SoundBuf* (device list-view) are two views of the same
            // buffer-wrapper object; unify SoundBuf == DirectSoundMgr to drop this cast.
            m_owner->RemoveBuffer((SoundBuf*)m_buffer);
        }
        m_buffer = 0;
        m_armed = 0;
    }
}

// ---------------------------------------------------------------------------
// StreamFeeder::Resume (__thiscall). If not already drained, resume
// the buffer (SetField3(1)) and, if it reports playing, mark drained (m_drained=1).
RVA(0x00137ed0, 0x30)
i32 StreamFeeder::Resume() {
    if (m_drained != 0) {
        return 1;
    }
    m_buffer->SetField3(1);
    i32 r = m_buffer->Play();
    if (r != 0) {
        m_drained = 1;
    }
    return r; // fall-through keeps the Play result in eax (no re-zero)
}

// ---------------------------------------------------------------------------
// StreamFeeder::Pause (__thiscall). If drained, stop+rewind the
// buffer and clear the drained flag; else nothing.
RVA(0x00137f00, 0x26)
i32 StreamFeeder::Pause() {
    if (m_drained == 0) {
        return 1;
    }
    i32 r = m_buffer->StopAndRewind();
    if (r != 0) {
        m_drained = 0;
    }
    return r; // fall-through keeps the StopAndRewind result in eax (no re-zero)
}

// ---------------------------------------------------------------------------
// StreamFeeder::FeederStart (__thiscall, 6 args, ret 0x18). Arm the
// feeder: record owner/length/format, derive the silence byte from the PCM bit
// depth, create (or adopt) the streaming buffer, arm, FeedData (slot 1) and
// prime via Tick.
// @early-stop
// regalloc eax/edx wall: retail pins the format-pointer (arg2) and the format-value
// (arg4) into a fixed eax/edx pair; MSVC here assigns the opposite pair. Body +
// control flow byte-exact, only the two-arg register choice differs - a
// register-allocation plateau (docs/patterns/zero-register-pinning.md family).
// Logic complete, deferred to the final sweep.
RVA(0x00137d10, 0xab)
i32 StreamFeeder::FeederStart(
    SoundDevice* owner,
    WaveFormatX* fmt,
    u32 len,
    u32 format,
    DirectSoundMgr* buf,
    i32 tickArg
) {
    m_format = format;
    m_owner = owner;
    m_bufferLength = len;
    m_drained = 0;
    if (fmt->wBitsPerSample > 8) {
        m_silenceByte = 0;
    } else {
        m_silenceByte = 0x80;
    }
    if (buf == 0) {
        m_buffer = owner->CreateBuffer(fmt, len, 0x100e0);
    } else {
        m_buffer = buf;
    }
    if (m_buffer == 0) {
        return 0;
    }
    m_armed = 1;
    if (FeedData() == 0) { // slot 1 (virtual)
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
// StreamFeeder::FillBuffer (__thiscall, 2 args, ret 0x8). Lock the
// streaming secondary buffer at the write window, copy the source window into
// the (possibly wrapped) locked regions, silence-pad the unfilled tail, advance
// the read cursor (m_bufferCursor) with wraparound, and Unlock.
// @early-stop
// local-coalescing / frame-size wall: retail reuses dead local slots for the
// got1/got2 scratch (sub esp,0x10); MSVC here keeps them distinct (sub esp,0x14),
// shifting every Lock out-pointer slot + the [esp+N] local offsets. Instruction
// selection + control flow byte-exact; only the stack-slot assignment differs -
// the documented local-coalescing wall (docs/patterns/stack-buffer-size-drives-frame.md).
// Logic complete, deferred to the final sweep.
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
        if (Feed(p1, n1, &got1, p2, n2, &got2) == 0) { // slot 0 (virtual)
            m_buffer->Unlock(p1, n1, p2, n2);
            return 0;
        }
    } else {
        got1 = 0;
        got2 = 0;
    }
    if (got1 < n1) {
        m_pendingBytes += n1 - got1;
        // language-forced: p1 is a void* DirectSound-locked region; the +got1
        // pointer arithmetic requires a byte-pointer view. Inlines to rep stos.
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
