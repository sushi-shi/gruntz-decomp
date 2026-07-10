#include <rva.h>
// NameRecord.cpp - the two-string name record setter (0x118040). A __thiscall
// validator/copier: it rejects an empty or over-long primary name (>16 chars),
// optionally rejects when a secondary string is present and the primary is over
// 64 chars (the retail re-measures the SAME primary name - reproduced verbatim),
// zero-fills the record body, and inline-copies the primary into +0x14 and the
// optional secondary into +0x36. Pure /O2 /Oi inline CRT (repne scasb / rep stos
// / rep movs), no relocations. Names are placeholders; the offsets + emitted
// bytes are load-bearing.
#include <string.h>

class CNameRecord {
public:
    i32 SetNames(char* name, char* name2, i32 unused); // 0x118040
    i32 CopyBody(char* body);                          // 0x118130
    // Sibling predicate (0x1182f0, == the m_08==1 success check, reloc-masked near
    // call): the CPred1182f0::IsOne view in BoundaryLowerThunks.cpp is this same field.
    i32 IsOne(); // 0x1182f0

    char _vft0[4];              // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_04;                   // +0x04 (head of the zeroed body)
    i32 m_08;                   // +0x08 set to 1 on success
    char m_body0c[0x14 - 0x0c]; // +0x0c..+0x13
    char m_name[0x36 - 0x14];   // +0x14..+0x35  primary name (<=16 chars)
    char m_name2[0xd8 - 0x36];  // +0x36..+0xd7  secondary string
};

// ===========================================================================
// validate + store the record's names. Returns 0 on any rejection,
// 1 on success. memset(&m_04,0,212) clears +0x04..+0xd7 before the copies.
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
i32 CNameRecord::SetNames(char* name, char* name2, i32 unused) {
    if (name == 0) {
        return 0;
    }
    if ((i32)strlen(name) > 16) {
        return 0;
    }
    if (name2 != 0 && (i32)strlen(name) > 64) {
        return 0;
    }
    memset(&m_04, 0, 212);
    strcpy(m_name, name);
    if (name2 != 0) {
        strcpy(m_name2, name2);
    }
    m_08 = 1;
    return 1;
}

// ===========================================================================
// CNameRecord::CopyBody (0x118130) - copy a 212-byte record body into m_04..m_d7
// (the whole record body) from an external source buffer, gated on the source's
// embedded name (at body+0x10, i.e. the m_name field position within the body)
// being 1..15 chars; then run the m_08==1 predicate (side-effect call, discarded)
// and return 1. Rejects a null source or an out-of-range name (returns 0). Inline
// /Oi CRT (repnz scasb strlen + rep movsd), no relocations except the near IsOne call.
// ===========================================================================
RVA(0x00118130, 0x44)
i32 CNameRecord::CopyBody(char* body) {
    if (body != 0) {
        i32 len = (i32)strlen(body + 0x10);
        if (len > 0 && len < 16) {
            memcpy(&m_04, body, 212);
            IsOne();
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// 0x1181d0 - a spatially-adjacent bounds-grow object (NOT CNameRecord: its box is
// at +0xb8, past this record's body). Reject when the new (+0x04,+0x08) pair does
// not exceed the +0xb8 box; else store it, notify (0x3661) and stash +0xd4.
// __thiscall(3). Placeholder identity. Re-homed from src/Stub/BoundaryLowerMethods.cpp.
// ===========================================================================
struct CBox118 {
    void* m_0;
    u32 m_4;
    u32 m_8;
};
struct C1181d0 {
    char pad0[0xb8];
    CBox118 m_bounds; // +0xb8
    char padd4[0xd4 - 0xb8 - 0xc];
    i32 m_d4; // +0xd4
    i32 Update(i32 a1, i32 a2, i32 a3);
};
extern "C" void Func3661(CBox118* p); // 0x3661
RVA(0x001181d0, 0x70)
i32 C1181d0::Update(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    CBox118* b = &m_bounds;
    if (b == 0) {
        return 0;
    }
    if (b->m_4 > a1) {
        return 0;
    }
    if (b->m_4 == a1 && b->m_8 < a2) {
        return 0;
    }
    b->m_4 = a1;
    b->m_8 = a2;
    Func3661(b);
    m_d4 = a3;
    return 1;
}

SIZE_UNKNOWN(CNameRecord);
SIZE_UNKNOWN(CBox118);
SIZE_UNKNOWN(C1181d0);
