#include <rva.h>
#include <string.h>
#include <Gruntz/GameInfo.h> // canonical CGameInfo / CGameInfoTime + BuildGameDate decl

// ===========================================================================
// CGameInfo::SetNames (0x118040) - validate + store the record's name (m_14) and its
// optional secondary/Location string (m_36). Rejects an empty or over-long (>16 char)
// primary name; rejects a secondary-present + primary >64 chars (retail re-measures the
// SAME primary name - reproduced verbatim); memset(&m_04, 0, 212) clears the whole body
// (+0x04..+0xd7) before the copies. Returns 0 on any rejection, 1 on success.
// ===========================================================================
// @early-stop
// Unsigned-compare scheduling micro-idiom (~96.6%): retail lowers each
// `strlen(name) > N` as `dec ecx; js Lfail; cmp ecx,N; jle Lok`, reusing the
// inline-strlen `dec ecx` flags for a free `len<0` (= unsigned-huge) half before
// the signed `cmp;jle`. Body is otherwise byte-exact; the `(i32)`-cast spelling
// (closest) emits the signed `cmp;jle` WITHOUT the free `js` - `(u32)` emits a
// single `jbe` (worse), an explicit `n<0||n>N` reschedules (much worse). The `js`
// is a cl scheduling artifact off the live dec flags, not steerable from C.
RVA(0x00118040, 0xb6)
i32 CGameInfo::SetNames(char* name, char* name2, i32 unused) {
    if (name == 0) {
        return 0;
    }
    if (static_cast<i32>(strlen(name)) > 16) {
        return 0;
    }
    if (name2 != 0 && static_cast<i32>(strlen(name)) > 64) {
        return 0;
    }
    memset(&m_04, 0, 212);
    strcpy(m_14, name);
    if (name2 != 0) {
        strcpy(m_36, name2);
    }
    m_8 = 1;
    return 1;
}

RVA(0x00118130, 0x44)
i32 CGameInfo::CopyBody(char* body) {
    if (body != 0) {
        i32 len = static_cast<i32>(strlen(body + 0x10));
        if (len > 0 && len < 16) {
            memcpy(&m_04, body, 212);
            Check1(); // 0x1182f0 (same +0x08 ready flag)
            return 1;
        }
    }
    return 0;
}

RVA(0x001181d0, 0x70)
i32 CGameInfo::Update(i32 s, i32 timestamp, i32 type) {
    if (s == 0) {
        return 0;
    }
    if (timestamp == 0) {
        return 0;
    }
    CGameInfoTime* b = &m_b8;
    if (b == 0) {
        return 0;
    }
    if (b->m_4 > s) {
        return 0;
    }
    if (b->m_4 == s && b->m_8 < timestamp) {
        return 0;
    }
    b->m_4 = s;
    b->m_8 = timestamp;
    BuildGameDate(b);
    m_d4 = type;
    return 1;
}

// ===========================================================================
// CGameInfo::CopyIfLarger (0x118260) - the RVA-contiguous TWIN of Update above (same
// +0xb8 time box, same +0xd4 Type). Reject a null/absent source, or one that does not
// exceed the current box; else copy the whole 7-dword CGameInfoTime in and stash Type.
// __thiscall(src, type) ret 8. (Was the CBoundsCopy118::CopyIfLarger view, re-homed from
// src/Stub/BoundaryLowerMethods.cpp.)
// @identity-TODO: no direct caller (a bounds-grow updater); reached only via an ILT thunk.
// ===========================================================================
RVA(0x00118260, 0x63)
i32 CGameInfo::CopyIfLarger(CGameInfoTime* src, i32 type) {
    if (src == 0) {
        return 0;
    }
    CGameInfoTime* dst = &m_b8;
    if (dst == 0) {
        return 0;
    }
    if (dst->m_4 > src->m_4) {
        return 0;
    }
    if (dst->m_4 == src->m_4 && dst->m_8 < src->m_8) {
        return 0;
    }
    *dst = *src;
    m_d4 = type;
    return 1;
}
