// Hash.cpp - CHash, the WAP32 engine string/int-keyed hash table embedded in
// CSymTab (+0x38 child scopes, +0x40 leaf symbols). The original is a template
// CHash<T> over a non-template key-agnostic base; the two byte-identical
// instantiations are modeled as CHashBase + CHash ("_a") / CHashB ("_b"). See
// include/Bute/Hash.h for the layout + the call-graph evidence. Recovered from
// the trace group "ClassUnknown_13" (callers all in the ButeMgr parser region).
#include <rva.h>
#include <Dsndmgr/SoundVoiceList.h>

#include <Bute/Hash.h>

// ---------------------------------------------------------------------------
// CHashBase - the shared slot machinery (one physical copy each).
// ---------------------------------------------------------------------------

// RemoveAll (0x184a40): array-delete the bucket array. The count cookie sits one
// word before m_buckets (the RezAlloc header); each 16-byte slot's dtor is a
// no-op; RezFree the cookie. The cookie read is a deliberate alloc-header offset.
RVA(0x00184a40, 0x27)
void CHashBase::RemoveAll() {
    if (m_buckets) {
        u32* cookie = (u32*)((char*)m_buckets - 4);
        Tm_DestroyArray(m_buckets, 0x10, *cookie, &CHashSlot_Dtor);
        RezFree(cookie);
    }
}

// Insert (0x184a70): ask the element for its bucket index (the slot-0 virtual
// hash), stamp the owning table (+0xc) and the bucket (+0x10), then splice the
// chain node (element+4) into the bucket's chain. The `?:` keeps the null-check
// `lea ecx,[esi+4]/xor ecx,ecx` even though the engine never feeds a null node.
// @early-stop
// SIB base/index coin-flip (99.55%): retail `lea [eax+ecx+8]` (idx<<4 as base) vs
// cl `lea [ecx+eax+8]` (m_buckets as base); operand-typing/reorder do not flip it.
RVA(0x00184a70, 0x34)
void CHashBase::Insert(CHashElement* node) {
    node->m_owner = this;
    u32 idx = node->Hash();
    node->m_bucket = idx;
    CHashLink* biased = node ? &node->m_link : 0;
    ((DSoundList*)&m_buckets[idx].m_chain)->InsertHead((DSoundLink*)biased);
}

// Remove (0x184ab0): unlink `entry` (its chain node = &entry->m_link) from the
// owning bucket's intrusive {head,tail} chain.
RVA(0x00184ab0, 0x25)
void CHashBase::Remove(CHashElement* entry) {
    CHashLink* node = entry ? &entry->m_link : 0;
    ((DSoundList*)&m_buckets[entry->m_bucket].m_chain)->Unlink((DSoundLink*)node);
}

// Lookup (0x184b40): chain head for bucket `idx`, biased back to the element, or 0.
RVA(0x00184b40, 0x1d)
CHashElement* CHashBase::Lookup(u32 idx) {
    return FromLink(m_buckets[idx].m_chain.m_head);
}

// Last (0x184b10): reverse iteration - scan the bucket array from the highest index
// down, return the tail element of the first non-empty bucket (or 0 when empty).
// Re-homed from src/Stub/GapFunctions.cpp (matcher-2; CHashBase-owned - bracketed by
// Remove/Lookup, scans the 16-byte CHashSlot[] reading m_chain.m_tail @+0x0c).
// @early-stop
// SAME SIB base/index coin-flip wall as CHashBase::Insert (above): retail folds the
// bucket address into `mov ecx,[ecx+4]; lea ecx,[ecx+eax+0xc]` (m_buckets reuses the
// `this` register), while cl loads m_buckets into eax -> `push esi; add eax,esi; lea
// ecx,[eax+0xc]`. The loop body is byte-identical; the base-register pick is not
// source-steerable (operand-typing/reorder do not flip it, per the Insert note).
RVA(0x00184b10, 0x29)
CHashElement* CHashBase::Last() {
    u32 i = m_count - 1;
    CHashSlot* p = &m_buckets[i];
    CHashElement* e;
    for (;;) {
        e = FromLink(p->m_chain.m_tail);
        if (i == 0) {
            break;
        }
        --i;
        --p;
        if (e != 0) {
            break;
        }
    }
    return e;
}

// ---------------------------------------------------------------------------
// CHash (instantiation "_a"): HashStr / Walk / HashInt / FindInt.
// ---------------------------------------------------------------------------

// CHash::HashStr (0x0013c240) is now an inline member in the header.


// Walk (0x13c270): hash the key, look up the bucket chain, scan for a matching
// record. `ci` selects _strcmpi over the record key ([record+0]) vs the inline
// byte-loop strcmp; on a match returns the record ([element+0x14]). The record's
// first word is its key (a genuinely heterogeneous payload, so read via void*).
RVA(0x0013c270, 0xca)
void* CHash::Walk(const char* name, i32 ci) {
    if (!name) {
        return 0;
    }
    CHashElement* e = Lookup(HashStr(name));
    if (ci) {
        while (e) {
            const char* key = *(const char**)e->m_record;
            if (_strcmpi(key, name) == 0) {
                return e->m_record;
            }
            e = FromLink(e->m_link.m_next);
        }
        return 0;
    }
    while (e) {
        const char* key = *(const char**)e->m_record;
        if (strcmp(key, name) == 0) {
            return e->m_record;
        }
        e = FromLink(e->m_link.m_next);
    }
    return 0;
}

// CHash::HashInt (0x0013c350) is now an inline member in the header.


// FindInt (0x13c360): hash the int key, look up the chain, return the first
// record whose key int ([record+0]) equals `key`.
RVA(0x0013c360, 0x47)
void* CHash::FindInt(u32 key) {
    CHashElement* e = Lookup(HashInt(key));
    while (e) {
        if (*(u32*)e->m_record == key) {
            return e->m_record;
        }
        e = FromLink(e->m_link.m_next);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CHashB (instantiation "_b"): the child-scope table's HashStr / Walk (identical
// source; distinct RVAs).
// ---------------------------------------------------------------------------

// CHashB::HashStr (0x0013c3c0) is now an inline member in the header.


// Walk (0x13c3f0): identical to CHash::Walk.
RVA(0x0013c3f0, 0xca)
void* CHashB::Walk(const char* name, i32 ci) {
    if (!name) {
        return 0;
    }
    CHashElement* e = Lookup(HashStr(name));
    if (ci) {
        while (e) {
            const char* key = *(const char**)e->m_record;
            if (_strcmpi(key, name) == 0) {
                return e->m_record;
            }
            e = FromLink(e->m_link.m_next);
        }
        return 0;
    }
    while (e) {
        const char* key = *(const char**)e->m_record;
        if (strcmp(key, name) == 0) {
            return e->m_record;
        }
        e = FromLink(e->m_link.m_next);
    }
    return 0;
}
