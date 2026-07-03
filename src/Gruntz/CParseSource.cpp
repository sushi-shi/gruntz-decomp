// CParseSource.cpp - the positioned byte-reader over a ButeMgr parse source
// (trace placeholder tomalla-85, RVAs 0x139ae0 / 0x139af0). SetPos seeks
// the cursor; Read copies up to `len` bytes (clamped to the limit) from whichever
// backing store is live - the mapped source, the inline buffer, or the virtual
// reader. Self-contained except the inlined memcpy (rep movs at /O2 /Oi) and the
// reader vtable dispatch. Names are placeholders, offsets + code bytes are
// load-bearing.
#include <Gruntz/CParseSource.h>

#include <rva.h>
#include <string.h>

// The Rez heap allocator/free (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82, cdecl C).
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

// ===========================================================================
// 0x139800 - GetEntryTag: return the first dword of the keyed-store entry m_entry.
// ===========================================================================
RVA(0x00139800, 0x6)
i32 CParseSource::GetEntryTag() {
    return *(i32*)m_entry;
}

// ===========================================================================
// 0x139960 - BeginParse: resolve the live source pointer for a parse pass. If
// the mapped source is active, return its mapped address adjusted for the stream
// base. Otherwise lazily allocate the +0x38 inline buffer and fill it with one
// virtual Read of the whole limit; on a short read, free + return 0.
// ===========================================================================
RVA(0x00139960, 0x6b)
i32 CParseSource::BeginParse() {
    if (m_mapped->m_mapping != 0) {
        return m_base - m_mapped->m_baseOffset + m_mapped->m_mapping;
    }
    if (m_buffer != 0) {
        return m_buffer;
    }
    if (m_length == 0) {
        return 0;
    }
    m_buffer = (i32)RezAlloc(m_length);
    if (m_buffer == 0) {
        return 0;
    }
    if (m_reader->Read(m_base, 0, m_length, (void*)m_buffer) != (i32)m_length) {
        RezFree((void*)m_buffer);
        m_buffer = 0;
    }
    return m_buffer;
}

// ===========================================================================
// 0x1399d0 - EndParse: release the inline parse buffer; returns 1.
// ===========================================================================
RVA(0x001399d0, 0x21)
i32 CParseSource::EndParse() {
    if (m_buffer != 0) {
        RezFree((void*)m_buffer);
        m_buffer = 0;
    }
    return 1;
}

// ===========================================================================
// 0x139a40 - ReadAt(dst, pos, len): copy `len` bytes from absolute position `pos`
// in whichever backing store is live (mapped source / inline buffer / virtual
// reader), WITHOUT seeking the cursor or clamping to the limit. The two buffer
// paths return 1; the virtual-reader path returns whether the full `len` was read.
// ===========================================================================
RVA(0x00139a40, 0x95)
i32 CParseSource::ReadAt(void* dst, i32 pos, u32 len) {
    ParseMappedSource* sd = m_mapped;
    if (sd->m_mapping != 0) {
        memcpy(dst, (const void*)(m_base - sd->m_baseOffset + pos + sd->m_mapping), len);
        return 1;
    }
    if (m_buffer != 0) {
        memcpy(dst, (const void*)(m_buffer + pos), len);
        return 1;
    }
    return m_reader->Read(m_base, pos, len, dst) == (i32)len;
}

// ===========================================================================
// 0x139ae0 - SetPos(pos): move the read cursor; returns 1.
// ===========================================================================
RVA(0x00139ae0, 0xf)
i32 CParseSource::SetPos(i32 pos) {
    m_cursor = pos;
    return 1;
}

// ===========================================================================
// 0x139af0 - Read(dst, len, seekPos): optionally seek (seekPos != -1), clamp the
// request to the remaining bytes, then copy from the live backing store. Returns
// the byte count, or 0 on empty/short read. The unsigned length math yields the
// `jbe` limit/empty tests.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (docs/patterns/pin-local-for-callee-saved-reg.md +
// reread-member-view-pointer.md): logic + control-flow byte-exact (same 3-way
// dispatch, shared return-0 epilogue, both inline rep movsd/movsb memcpys, the
// vtable call). Residue is allocator/selection only: retail holds the mapped-
// source ptr in edx and folds `sub esi,[edx+0xc]` (1 instr) where cl loads
// sd->m_baseOffset to a reg first, plus a je-vs-jbe 1-byte branch encoding on the empty
// check. Not source-steerable. ~89%; SetPos is 100%.
RVA(0x00139af0, 0xcc)
i32 CParseSource::Read(void* dst, u32 len, i32 seekPos) {
    if (seekPos != -1) {
        SetPos(seekPos);
    }

    u32 pos = (u32)m_cursor;
    u32 want = len;
    if (pos + want > m_length) {
        want = m_length - pos;
    }
    if (want != 0) {
        ParseMappedSource* sd = m_mapped;
        if (sd->m_mapping) {
            const char* base = (const char*)(m_base - sd->m_baseOffset + sd->m_mapping + pos);
            memcpy(dst, base, want);
            m_cursor += want;
            return want;
        }
        if (m_buffer) {
            const char* base = (const char*)(m_buffer + pos);
            memcpy(dst, base, want);
            m_cursor += want;
            return want;
        }
        if (m_reader->Read(m_base, pos, want, dst) == (i32)want) {
            m_cursor += want;
            return want;
        }
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CParseSource);
SIZE_UNKNOWN(ParseMappedSource);
SIZE_UNKNOWN(ParseVReader);
