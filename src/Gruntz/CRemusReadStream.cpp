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
