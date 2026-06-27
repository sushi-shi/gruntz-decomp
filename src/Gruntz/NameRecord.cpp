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

    void* m_vptr;              // +0x00 (untouched here)
    i32 m_04;                  // +0x04 (head of the zeroed body)
    i32 m_08;                  // +0x08 set to 1 on success
    char m_body0c[0x14 - 0x0c]; // +0x0c..+0x13
    char m_name[0x36 - 0x14];  // +0x14..+0x35  primary name (<=16 chars)
    char m_name2[0xd8 - 0x36]; // +0x36..+0xd7  secondary string
};

// ===========================================================================
// 0x118040 - validate + store the record's names. Returns 0 on any rejection,
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
    if (name == 0)
        return 0;
    if ((i32)strlen(name) > 16)
        return 0;
    if (name2 != 0 && (i32)strlen(name) > 64)
        return 0;
    memset(&m_04, 0, 212);
    strcpy(m_name, name);
    if (name2 != 0)
        strcpy(m_name2, name2);
    m_08 = 1;
    return 1;
}
