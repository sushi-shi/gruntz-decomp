// CRemusReadStream.cpp - the positioned byte-reader over a Remus parse source
// (trace placeholder ClassUnknown_85, RVAs 0x139ae0 / 0x139af0). SetPos seeks
// the cursor; Read copies up to `len` bytes (clamped to the limit) from whichever
// backing store is live - the mapped source, the inline buffer, or the virtual
// reader. Self-contained except the inlined memcpy (rep movs at /O2 /Oi) and the
// reader vtable dispatch. Names are placeholders, offsets + code bytes are
// load-bearing.
#include <Gruntz/CRemusReadStream.h>

#include <rva.h>
#include <string.h>

// The Rez heap allocator/free (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82, cdecl C).
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

// ===========================================================================
// 0x139800 - GetEntryTag: return the first dword of the keyed-store entry m_04.
// ===========================================================================
RVA(0x00139800, 0x6)
i32 CRemusReadStream::GetEntryTag() {
    return *(i32*)m_04;
}

// ===========================================================================
// 0x139960 - BeginParse: resolve the live source pointer for a parse pass. If
// the mapped source is active, return its mapped address adjusted for the stream
// base. Otherwise lazily allocate the +0x38 inline buffer and fill it with one
// virtual Read of the whole limit; on a short read, free + return 0.
// ===========================================================================
RVA(0x00139960, 0x6b)
i32 CRemusReadStream::BeginParse() {
    if (m_10->m_48 != 0)
        return m_14 - m_10->m_0c + m_10->m_48;
    if (m_38 != 0)
        return m_38;
    if (m_0c == 0)
        return 0;
    m_38 = (i32)RezAlloc(m_0c);
    if (m_38 == 0)
        return 0;
    if (m_34->Read(m_14, 0, m_0c, (void*)m_38) != (i32)m_0c) {
        RezFree((void*)m_38);
        m_38 = 0;
    }
    return m_38;
}

// ===========================================================================
// 0x1399d0 - EndParse: release the inline parse buffer; returns 1.
// ===========================================================================
RVA(0x001399d0, 0x21)
i32 CRemusReadStream::EndParse() {
    if (m_38 != 0) {
        RezFree((void*)m_38);
        m_38 = 0;
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
i32 CRemusReadStream::ReadAt(void* dst, i32 pos, u32 len) {
    RemusMappedSource* sd = m_10;
    if (sd->m_48 != 0) {
        memcpy(dst, (const void*)(m_14 - sd->m_0c + pos + sd->m_48), len);
        return 1;
    }
    if (m_38 != 0) {
        memcpy(dst, (const void*)(m_38 + pos), len);
        return 1;
    }
    return m_34->Read(m_14, pos, len, dst) == (i32)len;
}

// ===========================================================================
// 0x139ae0 - SetPos(pos): move the read cursor; returns 1.
// ===========================================================================
RVA(0x00139ae0, 0xf)
i32 CRemusReadStream::SetPos(i32 pos) {
    m_18 = pos;
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
// sd->m_0c to a reg first, plus a je-vs-jbe 1-byte branch encoding on the empty
// check. Not source-steerable. ~89%; SetPos is 100%.
RVA(0x00139af0, 0xcc)
i32 CRemusReadStream::Read(void* dst, u32 len, i32 seekPos) {
    if (seekPos != -1)
        SetPos(seekPos);

    u32 pos = (u32)m_18;
    u32 want = len;
    if (pos + want > m_0c)
        want = m_0c - pos;
    if (want != 0) {
        RemusMappedSource* sd = m_10;
        if (sd->m_48) {
            const char* base = (const char*)(m_14 - sd->m_0c + sd->m_48 + pos);
            memcpy(dst, base, want);
            m_18 += want;
            return want;
        }
        if (m_38) {
            const char* base = (const char*)(m_38 + pos);
            memcpy(dst, base, want);
            m_18 += want;
            return want;
        }
        if (m_34->Read(m_14, pos, want, dst) == (i32)want) {
            m_18 += want;
            return want;
        }
    }
    return 0;
}
