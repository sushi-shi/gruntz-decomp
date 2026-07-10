// Hash.cpp - CHash, the WAP32 engine string/int-keyed hash table embedded in
// CSymTab (+0x38 child scopes, +0x40 leaf symbols). The original is a template
// CHash<T> over a non-template key-agnostic base; the two byte-identical
// instantiations are modeled as CHashBase + CHash ("_a") / CHashB ("_b"). See
// include/Bute/Hash.h for the layout + the call-graph evidence. Recovered from
// the trace group "ClassUnknown_13" (callers all in the ButeMgr parser region).
#include <rva.h>
#include <Dsndmgr/SoundVoiceList.h>

#include <Bute/Hash.h>

// The CHashSlot ctor/dtor + the CHashBase slot machinery (0x1849d0-0x184b40) moved
// to src/Rez/RezColl.cpp (wave1-E: the 0x1832d0-pocket rez/sym/hash utility obj is
// ONE original TU; this file keeps only the 0x13c2xx CHash/CHashB key-typed lookups
// of the ButeMgr parser region).

// ---------------------------------------------------------------------------
// CHash (instantiation "_a"): HashStr / Walk / HashInt / FindInt.
// ---------------------------------------------------------------------------

// HashStr (0x13c240): string key hash - the key's length modulo the bucket count.
// A hand strlen (peek-ahead loop) then `len % m_count`; null key hashes to 0.
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

// HashInt (0x13c350): int key hash - `key % m_count` (unsigned div, remainder).
RVA(0x0013c350, 0xd)
u32 CHash::HashInt(u32 key) {
    return key % m_count;
}

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

// CHashB::HashStr (0x13c3c0): identical source to CHash::HashStr, distinct RVA.
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
