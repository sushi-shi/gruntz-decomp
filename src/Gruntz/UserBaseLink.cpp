// UserBaseLink.cpp - the out-of-line engine routines the inline game-object
// ctors chain (see include/Gruntz/UserLogic.h). Kept in their own TU so the leaf
// ctor TUs see only declarations (and assume the link ctor can throw - that is
// what makes MSVC emit the /GX EH frame). NOT MATCHED here: the RVA()/DATA()
// pins pin the symbols so the leaf ctors' calls/loads reloc-resolve on both base
// and target sides.
#include <Bute/ButeTree.h> // the real CVariantSlot (m_errSink->Set on the OOM paths)
#include <Gruntz/UserLogic.h>
#include <rva.h>

#include <ctype.h>  // isspace (0x12f8a0) / isdigit (0x12f840) - as FUNCTION calls
#include <stdlib.h> // malloc (0x120b60) / free (0x120c30)
#include <string.h> // memcpy (0x121960) / strchr (0x120120)

// The parser calls the isspace/isdigit LIBRARY functions (retail does, not the table
// macro), and memcpy out-of-line; force both.
#undef isspace
#undef isdigit
#pragma function(memcpy)

// The engine free (_RezFree, 0x1b9b82) - operator= releases the old band through it
// (the ctor/dtor use CRT free/malloc; operator= uses RezFree, matching retail).
extern "C" void RezFree(void* p); // 0x1b9b82

// The bad-argument / bad-character diagnostic name cell the parser reports through
// (distinct from g_projActName @0x6bf454; this one @0x6bf45c). Reloc-masked.
extern void* g_projActName2; // 0x6bf45c

// The container "name" the Ghidra "EngStr" label really names is a zBitVec (see
// <Wap32/zBitVec.h>). Its default ctor, deep-copy operator=, ~ and the 836B parser
// ctor live here; the leaf ctors' construct/assign/destruct reloc-mask against them.

// zBitVec() default ctor - build the base, size to g_defaultProjActSize, no bit set.
// INLINE so it folds into CUserBaseLink::CUserBaseLink() (0x16d710), the only site
// that default-constructs the link's zBitVec name.
inline zBitVec::zBitVec() : CContainerErr(g_containerName) {
    if (!SetSize(g_defaultProjActSize)) {
        void* cache = g_projActCache;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, (i32)cache, 0xc);
    }
}

// The +0x18 link: a zBitVec name field, default-constructed (0x16d710; can throw ->
// the /GX EH frame every leaf ctor inherits).
RVA(0x0016d710, 0x76)
CUserBaseLink::CUserBaseLink() {}

// zBitVec(const char* tokens, int minSize) - the 836B whitespace/','/'-' number-list
// PARSER ctor (0x16d3a0). Pass 1 validates the token syntax and finds the max index
// (to size the bit-set to max(minSize, maxIndex)); pass 2 re-scans and sets each
// listed bit, expanding "N-M" ranges. Bad arg / bad char fire the container error sink.
// @early-stop
// regalloc wall (zero-register-pinning.md family): the two-pass logic, the
// isspace/isdigit/strchr(" ,-") validation, the max-index sizing, the SBO bit-set and
// the "N-M" range expansion are all byte-faithful, but cl pins the pass-1 cursor in
// edi where retail keeps it in esi (retail: esi=cursor, ebp=value, edi=sawSep flag).
// That swap spills the flag to an extra [esp+0x10] slot and cascades esi/edi through
// every [cursor] access. Splitting the pass-2 cursor into its own var did not move the
// pass-1 assignment; no source lever reaches the allocator here. ~79.8%, logic
// complete; deferred to the final sweep.
RVA(0x0016d3a0, 0x344)
zBitVec::zBitVec(const char* tokens, i32 minSize) : CContainerErr(g_containerName) {
    i32 maxv = 0;
    const char* start;
    const char* q;
    if (tokens == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, (i32)name, 0x16);
        return;
    }
    if (minSize == 0) {
        minSize = g_defaultProjActSize;
    }

    const char* p = tokens;
    if (isspace(*p)) {
        do {
            ++p;
        } while (isspace(*p));
    }
    if (*p == 0) {
        if (!SetSize(minSize)) {
            goto oom;
        }
        return;
    }
    if (!isdigit(*p)) {
        goto badchar;
    }

    // ---- pass 1: validate + find max index ----
    start = p;
    while (*p != 0) {
        i32 v = 0;
        i32 sawSep = 0;
        while (isdigit(*p)) {
            v = v * 10 + (*p - '0');
            ++p;
        }
        if ((u32)v > (u32)maxv) {
            maxv = v;
        }
        while (*p != 0) {
            if (isdigit(*p)) {
                break;
            }
            if (sawSep && *p != ' ') {
                goto badchar;
            }
            if (strchr(" ,-", *p) == 0) {
                goto badchar;
            }
            if (*p != ' ') {
                sawSep = 1;
            }
            ++p;
        }
    }

    if ((u32)minSize > (u32)maxv) {
        maxv = minSize;
    }
    if (!SetSize(maxv)) {
        goto oom;
    }

    // ---- pass 2: set the bits ----
    q = start;
    while (*q != 0) {
        i32 v = 0;
        while (isdigit(*q)) {
            v = v * 10 + (*q - '0');
            ++q;
        }
        {
            u32* band = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
            band[(u32)v >> 5] |= 1u << (v & 0x1f);
        }
        if (*q == 0) {
            break;
        }
        if (isspace(*q)) {
            while (isspace(q[1])) {
                ++q;
            }
        }
        char sep = *q;
        ++q;
        if (isspace(*q)) {
            while (isspace(q[1])) {
                ++q;
            }
        }
        if (sep == '-') {
            i32 v2 = 0;
            while (isdigit(*q)) {
                v2 = v2 * 10 + (*q - '0');
                ++q;
            }
            if (v > v2) {
                i32 t = v;
                v = v2;
                v2 = t;
            }
            for (i32 b = v + 1; b <= v2; ++b) {
                u32* band = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
                band[(u32)b >> 5] |= 1u << (b & 0x1f);
            }
            while (*q != 0 && !isdigit(*q)) {
                ++q;
            }
        }
    }
    return;

oom: {
    void* cache = g_projActCache;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, (i32)cache, 0xc);
    return;
}
badchar: {
    void* name = g_projActName2;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, (i32)name, 0x16);
    return;
}
}

// zBitVec::operator= (0x16d2f0) - deep-copy the DWORD band. If the capacities differ,
// release this band (RezFree) and reallocate to the source's word count (or OOM), then
// memcpy m_capacity/8 bytes from the source (heap band or inline SBO word).
// @early-stop
// regalloc wall: 1 instruction of 172 differs - `m_capacity = that.m_capacity` on the
// capacities-differ path picks ecx in cl vs eax in retail (retail reuses the freed
// malloc-result register). 99.84%, no source lever for the scratch register.
RVA(0x0016d2f0, 0xac)
zBitVec& zBitVec::operator=(const zBitVec& that) {
    if (this != &that) {
        if (m_capacity != that.m_capacity) {
            if ((u32)m_capacity > 0x20) {
                RezFree(m_words);
            }
            if ((u32)that.m_capacity > 0x20) {
                m_words = (u32*)malloc(((u32)that.m_capacity >> 5) * 4);
                if (!m_words) {
                    void* cache = g_projActCache;
                    g_retAddrBreadcrumb = GetCallerRetAddr();
                    m_errSink->Set(this, (i32)cache, 0xc);
                    m_capacity = 0x20;
                    return *this;
                }
            }
            m_capacity = that.m_capacity;
        }
        const u32* src = ((u32)that.m_capacity > 0x20) ? that.m_words : (const u32*)&that.m_words;
        u32* dst = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
        memcpy(dst, src, (u32)m_capacity >> 3);
    }
    return *this;
}

// zBitVec::~zBitVec() (0x16d2a0) - free the heap band when out of SBO range, then
// chain ~CContainerErr (implicit).
RVA(0x0016d2a0, 0x26)
zBitVec::~zBitVec() {
    if ((u32)m_capacity > 0x20) {
        free(m_words);
    }
}

// operator new the engine uses (0x1b9b46, __cdecl); reloc-masked rel32.
extern "C" void* RezAlloc(u32 n); // 0x1b9b46

// Stamp the foreign worker vtable (0x5efb80) by address - the vptr store the
// inline worker ctor emits; never a C++ ctor (that would emit a divergent vtable).
static inline void StampWorkerVtbl(CAnimWorker* w) {
    *(void**)w = &g_animWorkerVtbl;
}

// CGameObject::EnsureWorker80 (0x150eb0): the +0x80 worker variant of
// EnsureWorker88/90 - same lazy build/reuse/feed, but it RETURNS the slot-9 result
// (or 0 on the null guards). Called by AddLogicHit (0x150f50).
// @early-stop
// Expected to share the zero-register-pinning wall of EnsureWorker88/90 (this/0 in
// esi<->edi). Logic byte-exact; a pure allocator coin-flip, not source-steerable.
RVA(0x00150eb0, 0x98)
i32 CGameObject::EnsureWorker80(CGameObject* src) {
    if (src == 0) {
        return 0;
    }
    if (m_80 != 0) {
        m_80->Slot07();
    } else {
        CAnimWorker* w = (CAnimWorker*)RezAlloc(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_80 = w;
    }
    if (m_80 == 0) {
        return 0;
    }
    return m_80->Slot09(src->m_10, 0);
}

// CGameObject::EnsureWorker88 (0x150f90): lazily build the +0x88 worker - if one
// already exists, just re-run its slot-7 reuse hook; otherwise operator new a
// fresh 0x17c-byte worker (seeded m_04=this->m_4, m_08=0, m_0c=this->m_c, the
// foreign vtable, all other fields 0), stow it at +0x88, then feed src->m_10
// through slot 9. The worker is built MANUALLY (the inline ctor), not via a C++
// ctor, so the vptr store stamps the retail 0x5efb80 by address.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the whole
// build sequence + both dispatches are byte-identical, but retail pins this->edi
// and 0->esi while cl pins this->esi and 0->edi, and lowers the `arg==0` guard as
// an early `xor eax,eax;ret` block where cl shares the epilogue - the swap cascades
// every esi/edi. Logic exact; a pure allocator coin-flip, not source-steerable.
RVA(0x00150f90, 0x98)
void CGameObject::EnsureWorker88(CGameObject* src) {
    if (src == 0) {
        return;
    }
    if (m_88 != 0) {
        m_88->Slot07();
    } else {
        CAnimWorker* w = (CAnimWorker*)RezAlloc(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_88 = w;
    }
    if (m_88 == 0) {
        return;
    }
    m_88->Slot09(src->m_10, 0);
}

// CGameObject::EnsureWorker90 (0x151070): identical to EnsureWorker88 but for the
// +0x90 worker slot.
// @early-stop
// same zero-register-pinning wall as EnsureWorker88 (this/0 in esi<->edi).
RVA(0x00151070, 0x98)
void CGameObject::EnsureWorker90(CGameObject* src) {
    if (src == 0) {
        return;
    }
    if (m_collideWorker != 0) {
        m_collideWorker->Slot07();
    } else {
        CAnimWorker* w = (CAnimWorker*)RezAlloc(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_collideWorker = w;
    }
    if (m_collideWorker == 0) {
        return;
    }
    m_collideWorker->Slot09(src->m_10, 0);
}

// CGameObject's three built-in logic-handler registrars: look the logic-name key
// up in the world's CMapStringToOb (m_0c -> +0x14 -> +0x10), then feed the found
// handler through the matching lazy worker slot (Hit -> 80, Attack -> 88, Bump -> 90).
// m_0c is the family's generically-typed world/context slot (i32); reached by
// documented offset here.
// @early-stop
// scheduling coin-flip: body byte-exact EXCEPT the `handler = 0` slot-init lands one
// push early (push &out; STORE; push key) where retail schedules it after both pushes
// (push &out; push key; STORE). Same slot, independent store; MSVC5's scheduler places
// it between the arg pushes. No source ordering of the init reproduces the late slot.
RVA(0x00150f50, 0x33)
void CGameObject::AddLogicHit(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    ((CMapStringToPtr*)map)->Lookup(key, (void*&)handler);
    EnsureWorker80(handler);
}

// @early-stop
// same `handler = 0` scheduling coin-flip as AddLogicHit.
RVA(0x00151030, 0x33)
void CGameObject::AddLogicAttack(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    ((CMapStringToPtr*)map)->Lookup(key, (void*&)handler);
    EnsureWorker88(handler);
}

// @early-stop
// same `handler = 0` scheduling coin-flip as AddLogicHit.
RVA(0x00151110, 0x33)
void CGameObject::AddLogicBump(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    ((CMapStringToPtr*)map)->Lookup(key, (void*&)handler);
    EnsureWorker90(handler);
}

// g_logicTypesRegistered (RVA 0x2bf674, VA 0x6bf674): the one-shot logic-type
// guard. g_emptyString (RVA 0x2293f4) is already labeled by netmgrerror, so it
// is only DECLARED in the header, never re-defined here.
DATA(0x002bf674)
i32 g_logicTypesRegistered;
