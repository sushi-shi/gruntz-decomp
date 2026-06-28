// Hash.cpp - CHash, the WAP32 engine string/int-keyed hash table embedded in
// CSymTab (+0x38 child scopes, +0x40 leaf symbols). The original is a template
// CHash<T> over a non-template key-agnostic base; the two byte-identical
// instantiations are modeled as CHashBase + CHash ("_a") / CHashB ("_b"). See
// include/Bute/Hash.h for the layout + the call-graph evidence. Recovered from
// the trace group "ClassUnknown_13" (callers all in the Remus parser region).
#include <rva.h>

#include <Bute/Hash.h>

// ---------------------------------------------------------------------------
// CHashBase - the shared slot machinery (one physical copy each).
// ---------------------------------------------------------------------------

// RemoveAll (0x184a40): array-delete the bucket array. The count cookie sits one
// word before m_buckets; each 16-byte slot's dtor is a no-op; RezFree the cookie.
RVA(0x00184a40, 0x27)
void CHashBase::RemoveAll() {
    if (m_buckets) {
        u32* cookie = (u32*)((char*)m_buckets - 4);
        Tm_DestroyArray(m_buckets, 0x10, (i32)*cookie, (void*)&CHashSlot_Dtor);
        RezFree(cookie);
    }
}

// Insert (0x184a70): ask the entry for its bucket index (the slot-0 virtual
// hash), stamp the owning table (+0xc) and the bucket (+0x10) into the entry,
// then splice the biased node (entry+4) into the bucket's chain. The `?:` keeps
// the null-check `lea ecx,[esi+4]/xor ecx,ecx` even though the engine never feeds
// a null entry here.
RVA(0x00184a70, 0x34)
void CHashBase::Insert(CHashInsertNode* node) {
    node->m_ownerTable = this;
    u32 idx = node->Hash();
    node->m_bucket = idx;
    void* biased = node ? (void*)((char*)node + 4) : 0;
    CHashSlotList* slot = (CHashSlotList*)((idx << 4) + (char*)m_buckets + 8);
    slot->Link(biased);
}

// Remove (0x184ab0): unlink `entry` (its biased node = entry+4) from the owning
// bucket's intrusive {head,tail} chain.
RVA(0x00184ab0, 0x25)
void CHashBase::Remove(CHashEntry* entry) {
    void* node = entry ? (void*)((char*)entry + 4) : 0;
    CHashSlotList* slot = (CHashSlotList*)((entry->m_bucket << 4) + (char*)m_buckets + 8);
    slot->Unlink(node);
}

// Lookup (0x184b40): chain head for bucket `idx`, biased back to the entry, or 0.
RVA(0x00184b40, 0x1d)
CHashEntry* CHashBase::Lookup(u32 idx) {
    void* head = m_buckets[idx].m_head;
    if (head) {
        return (CHashEntry*)((char*)head - 4);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CHash (instantiation "_a"): HashStr / Walk / HashInt / FindInt.
// ---------------------------------------------------------------------------

// HashStr (0x13c240): inline-strlen the key (the byte loop, pre-load + do-while),
// then len % m_count. Null key -> 0.
RVA(0x0013c240, 0x29)
u32 CHash::HashStr(const char* s) {
    if (!s) {
        return 0;
    }
    u32 len = 0;
    while (*s) {
        ++len;
        ++s;
    }
    return len % m_count;
}

// Walk (0x13c270): hash the key, look up the bucket chain, scan for a matching
// record. `ci` selects _strcmpi over the record key ([rec+0]) vs the inline
// byte-loop strcmp; on a match returns the record ([entry+0x14]).
RVA(0x0013c270, 0xca)
void* CHash::Walk(const char* name, i32 ci) {
    if (!name) {
        return 0;
    }
    CHashEntry* e = Lookup(HashStr(name));
    if (ci) {
        while (e) {
            const char* key = *(const char**)e->m_rec;
            if (_strcmpi(key, name) == 0) {
                return e->m_rec;
            }
            CHashEntry* n = (CHashEntry*)e->m_next;
            e = n ? (CHashEntry*)((char*)n - 4) : n;
        }
        return 0;
    }
    while (e) {
        const char* key = *(const char**)e->m_rec;
        if (strcmp(key, name) == 0) {
            return e->m_rec;
        }
        CHashEntry* n = (CHashEntry*)e->m_next;
        e = n ? (CHashEntry*)((char*)n - 4) : n;
    }
    return 0;
}

// HashInt (0x13c350): key % m_count.
RVA(0x0013c350, 0xd)
u32 CHash::HashInt(u32 key) {
    return key % m_count;
}

// FindInt (0x13c360): hash the int key, look up the chain, return the first
// record whose key int ([rec+0]) equals `key`.
RVA(0x0013c360, 0x47)
void* CHash::FindInt(u32 key) {
    CHashEntry* e = Lookup(HashInt(key));
    while (e) {
        if (*(u32*)e->m_rec == key) {
            return e->m_rec;
        }
        CHashEntry* n = (CHashEntry*)e->m_next;
        e = n ? (CHashEntry*)((char*)n - 4) : n;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CHashB (instantiation "_b"): the child-scope table's HashStr / Walk (identical
// source; distinct RVAs).
// ---------------------------------------------------------------------------

// HashStr (0x13c3c0): identical to CHash::HashStr.
RVA(0x0013c3c0, 0x29)
u32 CHashB::HashStr(const char* s) {
    if (!s) {
        return 0;
    }
    u32 len = 0;
    while (*s) {
        ++len;
        ++s;
    }
    return len % m_count;
}

// Walk (0x13c3f0): identical to CHash::Walk.
RVA(0x0013c3f0, 0xca)
void* CHashB::Walk(const char* name, i32 ci) {
    if (!name) {
        return 0;
    }
    CHashEntry* e = Lookup(HashStr(name));
    if (ci) {
        while (e) {
            const char* key = *(const char**)e->m_rec;
            if (_strcmpi(key, name) == 0) {
                return e->m_rec;
            }
            CHashEntry* n = (CHashEntry*)e->m_next;
            e = n ? (CHashEntry*)((char*)n - 4) : n;
        }
        return 0;
    }
    while (e) {
        const char* key = *(const char**)e->m_rec;
        if (strcmp(key, name) == 0) {
            return e->m_rec;
        }
        CHashEntry* n = (CHashEntry*)e->m_next;
        e = n ? (CHashEntry*)((char*)n - 4) : n;
    }
    return 0;
}
