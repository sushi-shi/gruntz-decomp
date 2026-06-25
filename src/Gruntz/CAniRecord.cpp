#include <rva.h>
// CAniRecord.cpp - the 0x34-byte 'ANI' animation FRAME RECORD cataloged by
// CAniElement (src/Gruntz/CAniElement.cpp) into a CObArray of these. The
// orchestrator placeholders ClassUnknown_39 / _40 / _41 are ALL this ONE class
// (consolidated here):
//
//   ClassUnknown_39: 0x168c60 Parse (read a packed 9-short record header off a
//                    binary cursor, then resolve the name string), 0x168e50
//                    GetSize (frame-count -> byte accumulator).
//   ClassUnknown_40: 0x168d00 ResolveIndices (whitespace-split the name string
//                    into tokens, look each up in the owner's CMapStringToPtr, and
//                    store the resolved indices in a RezAlloc'd array). /GX frame.
//   ClassUnknown_41: 0x165dd0 the base-2 (g_albusWorkerVtbl) destructor (frees the
//                    +0x10 buffer-virtual, resets the base subobject to CObject),
//                    0x168ee0 / 0x168fb0 the two buffer (de)allocation virtuals
//                    that go through the owner's pool allocator (owner+0x1c).
//
// The class is multiply-derived: a primary base whose vftable is g_aniRecordVtbl
// @0x5f02c0 (slot1 dtor 0x165780/0x1657a0) and a secondary CObject base whose
// vftable is g_albusWorkerVtbl @0x5f02d8 (= 0x5f02c0+0x18; slot1 dtor 0x165db0/
// 0x165dd0). No RTTI complete-object-locator survives for the most-derived class,
// so both vftables are reloc-masked DIR32 data stamped by address (an incomplete
// polymorphic model would emit a divergent ??_7). Only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine); field names are placeholders. See
// include comment block above the layout for the recovered offsets.
//
// This TU carries a /GX EH frame (flags="eh"): the ResolveIndices parser and the
// base-2 destructor have SEH frames for their destructible locals; the leaves
// (Parse / GetSize / the two buffer virtuals) stay frameless and byte-exact.
// ---------------------------------------------------------------------------
#include <Mfc.h>    // real MFC CStringArray / CMapStringToPtr / CString / CObject
#include <string.h> // strlen (inline repnz scas)

// The two construction vtables (reloc-masked DIR32 data, RVA = VA-0x400000):
// the secondary base (g_albusWorkerVtbl @0x5f02d8) and the CObject grand-base dtor
// vtable (g_remusBaseDtorVtbl @0x5e8cb4) restamped at base-2 dtor exit.
DATA(0x001f02d8)
extern void* g_albusWorkerVtbl; // 0x5f02d8 - the secondary base vftable
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4 - the CObject base dtor vtable

// g_aniParsedNameLen (0x6bf3c4): the parsed name length the catalog builder uses
// to advance the record stream cursor; Parse sets it (strlen of the name).
DATA(0x002bf3c4)
extern i32 g_aniParsedNameLen;

// Global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);
// The buffer is freed via _RezFree (@0x1b9b82, __cdecl).
extern "C" void RezFree(void* p);

// ---------------------------------------------------------------------------
// The pool allocator the buffer virtuals route through: the owner (record+0x0c)
// holds at +0x1c a manager whose Alloc(handle, size) / Free(p) (re)allocate the
// record's +0x10 work buffer from a fixed pool. Modeled as a tiny __thiscall
// helper so `mov ecx,[owner+0x1c]; call` falls out with no stack cleanup.
class CAniRecordPool {
public:
    void* Alloc1_142fc0(i32 handle, i32 size);          // 0x142fc0
    void* Alloc2_142f40(i32 handle, i32 size);          // 0x142f40
    void* Alloc3_1430c0(i32 a, i32 handle, i32 size);   // 0x1430c0
    void Free_142f10(void* p);                          // 0x142f10
};

// The owner node (record+0x0c). Its +0x08 is a flags word the buffer virtuals OR
// a bit into; its +0x1c is the pool above.
class CAniRecordOwner {
public:
    i32 m_00, m_04;        // +0x00..+0x07
    i32 m_08;              // +0x08  flags
    char _pad0c[0x1c - 0x0c];
    CAniRecordPool* m_1c;  // +0x1c  the pool allocator
};

// The freshly-allocated +0x10 buffer's second-stage init (0x1485b0, __thiscall on
// the buffer); reached only on the "took a new buffer" path of the buffer virtuals.
class CAniRecordBuf {
public:
    void Init_1485b0(); // 0x1485b0
};

// The CMapStringToPtr the index resolver looks each token up in lives at owner2
// +0x10 (owner2 = ResolveIndices arg1). Real MFC layout via <Mfc.h>.
struct CAniMapOwner {
    char _pad00[0x10];
    CMapStringToPtr m_map; // +0x10
};

// ---------------------------------------------------------------------------
// The 0x34-byte record. Layout recovered from Parse + the dtors + CAniElement's
// new-site. Modeled with NO virtuals (manual-vtable, dual-base), so the offsets
// stay exactly where retail puts them.
class CAniRecord {
public:
    i32 Parse_168c60(void* ctx, const i16* src); // 0x168c60
    i32 GetSize_168e50();                        // 0x168e50
    void ResolveIndices_168d00(CAniMapOwner* owner, const char* str); // 0x168d00
    void* Alloc168ee0(i32 size, i32 flag);       // 0x168ee0
    void* Alloc168ea0(i32 size, i32 flag);       // 0x168ea0
    void* Alloc168f60(i32 a, i32 size, i32 flag);// 0x168f60
    void FreeBuf_168fb0();                       // 0x168fb0

    void* m_vptr;          // +0x00
    u16 m_flags;           // +0x04  status word (bit 1 scaled, bit 2 has-name)
    u16 m_06;              // +0x06  (high half of the +0x04 dword)
    i32 m_08;              // +0x08
    CAniRecordOwner* m_0c; // +0x0c  owner node (also seeded 0xffff at alloc)
    i32 m_10;              // +0x10  pool work buffer (i32 here; ptr in the virtuals)
    i32 m_14;              // +0x14
    i32 m_18;              // +0x18  frame count (GetSize)
    i32 m_1c;              // +0x1c
    i32 m_20;              // +0x20
    i32 m_24;              // +0x24
    u16 m_28;              // +0x28
    u16 m_2a;              // +0x2a
    i32 m_count;           // +0x2c  resolved-index array length
    i32* m_indices;        // +0x30  resolved-index array
};

// ---------------------------------------------------------------------------
// 0x165dd0: the base-2 (g_albusWorkerVtbl) destructor. /GX. Stamps the secondary
// base vftable, frees the +0x10 work buffer (FreeBuf_168fb0), then the embedded
// CObject grand-base sub-object's MEMBER destructor resets it (m_04=-1, m_08=0,
// m_0c=0, vptr=g_remusBaseDtorVtbl). Per docs/patterns/eh-dtor-subobject-vptr-
// restore-member.md the trailing CObject reset is the sub-object's member dtor run
// under the /GX frame, NOT body code - so the body is only the albus stamp + the
// FreeBuf call, and m_cobj auto-destructs.
//
// The secondary-base `this` (the record) is modeled here as a tiny class whose
// FIRST value member is the destructible CObject sub-object (its vptr + 3 base
// fields), so the compiler emits the /GX frame + the trylevel-0 write + the
// member-teardown reset in the retail schedule.
struct CAniRecordCObjSub {
    void* m_vptr; // +0x00 (shared with the secondary-base vptr stamp)
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c
    ~CAniRecordCObjSub() {
        m_04 = -1;
        m_08 = 0;
        m_0c = 0;
        m_vptr = &g_remusBaseDtorVtbl;
    }
};

struct CAniRecordBase2 {
    CAniRecordCObjSub m_cobj; // +0x00 (the CObject grand-base sub-object)
    ~CAniRecordBase2();
    void FreeBuf_168fb0() { ((CAniRecord*)this)->FreeBuf_168fb0(); }
};

// @early-stop
// MI base-2 destructor: complete reconstruction modeling the CObject grand-base as
// a destructible value member (eh-dtor-subobject-vptr-restore-member.md). The /GX
// frame + body shape are correct; the residual EH-state-index / vptr-restamp
// schedule is the documented EH-state wall - parked for the final sweep.
RVA(0x00165dd0, 0x5b)
CAniRecordBase2::~CAniRecordBase2() {
    m_cobj.m_vptr = &g_albusWorkerVtbl;
    FreeBuf_168fb0();
    // m_cobj auto-destructs here (the CObject base reset) under the /GX frame.
}

// ---------------------------------------------------------------------------
// 0x168c60: Parse a packed record header off the binary cursor `src` (a stream of
// little-endian 16-bit fields): m_flags (WORD), then eight movsx'd i16 -> i32, then
// m_28 (WORD). Zero the index array, clear g_aniParsedNameLen, then if m_flags has
// the "has name" bit (0x2) strlen the trailing name -> g_aniParsedNameLen and
// resolve the indices from it. Returns 1. Frameless leaf.
RVA(0x00168c60, 0xa0)
i32 CAniRecord::Parse_168c60(void* ctx, const i16* src) {
    const i16* p = src;
    m_flags = (u16)*p++;
    m_08 = *p++;
    m_0c = (CAniRecordOwner*)*p++;
    m_10 = *p++;
    m_14 = *p++;
    m_18 = *p++;
    m_1c = *p++;
    m_20 = *p++;
    m_24 = *p++;
    m_28 = (u16)*p++;
    m_indices = 0;
    m_count = 0;
    g_aniParsedNameLen = 0;
    if (m_flags & 0x2) {
        const char* name = (const char*)p;
        g_aniParsedNameLen = (i32)strlen(name) + 1;
        ResolveIndices_168d00((CAniMapOwner*)ctx, name);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x168d00: whitespace-split `str` into tokens, look each token up by name in the
// owner's CMapStringToPtr (owner+0x10), and store the resolved indices in a
// RezAlloc'd array (m_indices, length m_count). /GX frame for the local
// CStringArray + per-token CString temporaries.
// @early-stop
// /GX CStringArray + tokenizer: complete reconstruction at ~97%. The whole body
// (tokenize / SetAtGrow / RezAlloc / per-token GetAt+Lookup+store / EH unwind) is
// byte-correct; the residual is the named-local slot coin-flip in the per-token
// loop (retail puts the Lookup-out at [esp+0x10] and the CString temp at [esp+0x14];
// cl flips them - not decl-order-steerable) + reloc operand names. Documented
// regalloc/slot wall - parked for the final sweep.
RVA(0x00168d00, 0x14c)
void CAniRecord::ResolveIndices_168d00(CAniMapOwner* owner, const char* str) {
    if (owner == 0 || str == 0) {
        return;
    }
    CStringArray tokens;
    char tok[0x80];
    i32 n = 0;
    const char* s = str;
    while (*s != 0) {
        char c = *s;
        if (c > 0x21) {
            tok[n++] = c;
        } else {
            tok[n] = 0;
            if (n > 0) {
                tokens.SetAtGrow(tokens.GetSize(), tok);
            }
            n = 0;
        }
        s++;
    }
    tok[n] = 0;
    if (n > 0) {
        tokens.SetAtGrow(tokens.GetSize(), tok);
    }
    m_count = tokens.GetSize();
    if (m_count > 0) {
        m_indices = (i32*)operator new((u32)(m_count * 4));
        for (i32 i = 0; i < m_count; i++) {
            CString t = tokens.GetAt(i);
            void* v = 0;
            owner->m_map.Lookup(t, v);
            m_indices[i] = (i32)v;
        }
    }
}

// ---------------------------------------------------------------------------
// 0x168e50: byte-size of the resolved frames. If the frame count m_18 is positive,
// return m_18*22 when the "scaled" flag (bit 0x1) is set, else m_18; otherwise the
// 0x16 empty-record constant. Frameless leaf.
// @early-stop
// scheduling: body shape (count guard / scaled lea*22 / plain count / 0x16 default)
// byte-correct; retail pins the count in edx and materializes the `mov eax,0x16`
// default right after the count load, while cl keeps the count in eax and schedules
// the constant on the fall-through ret (~87%). All 4 spellings tried (early-return,
// 0x16-default-temp, n<=0-first, n>0-first) flip the regalloc but none reproduce the
// edx-pin + eager-default. Not source-steerable - documented scheduling wall.
RVA(0x00168e50, 0x1e)
i32 CAniRecord::GetSize_168e50() {
    i32 n = m_18;
    if (n > 0) {
        if (m_flags & 0x1) {
            return n * 22;
        }
        return n;
    }
    return 0x16;
}

// ---------------------------------------------------------------------------
// 0x168ea0: (re)allocate the +0x10 work buffer from the owner's pool with size
// 0x44 (Alloc2_142f40). On success, if the caller's flag bit 0x1 is set, mark the
// owner's +0x08 flags and run the buffer's second-stage init. Returns 1.
// Frameless leaf.
RVA(0x00168ea0, 0x40)
void* CAniRecord::Alloc168ea0(i32 size, i32 flag) {
    CAniRecordBuf* buf = (CAniRecordBuf*)m_0c->m_1c->Alloc2_142f40(size, 0x44);
    m_10 = (i32)buf;
    if (buf == 0) {
        return (void*)0; // tail returns 1 only on the success path below
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->Init_1485b0();
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// 0x168ee0: as 0x168ea0 but through Alloc1_142fc0 (the canonical 0x44 allocator).
// Frameless leaf.
RVA(0x00168ee0, 0x40)
void* CAniRecord::Alloc168ee0(i32 size, i32 flag) {
    CAniRecordBuf* buf = (CAniRecordBuf*)m_0c->m_1c->Alloc1_142fc0(size, 0x44);
    m_10 = (i32)buf;
    if (buf == 0) {
        return (void*)0;
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->Init_1485b0();
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// 0x168f60: the three-arg buffer allocator (Alloc3_1430c0, ret 0xc). Frameless leaf.
RVA(0x00168f60, 0x45)
void* CAniRecord::Alloc168f60(i32 a, i32 size, i32 flag) {
    CAniRecordBuf* buf = (CAniRecordBuf*)m_0c->m_1c->Alloc3_1430c0(a, size, 0x44);
    m_10 = (i32)buf;
    if (buf == 0) {
        return (void*)0;
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->Init_1485b0();
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// 0x168fb0: free the +0x10 work buffer back to the owner's pool (Free_142f10) and
// clear it. Frameless leaf.
RVA(0x00168fb0, 0x1f)
void CAniRecord::FreeBuf_168fb0() {
    i32 buf = m_10;
    if (buf != 0) {
        m_0c->m_1c->Free_142f10((void*)buf);
        m_10 = 0;
    }
}
