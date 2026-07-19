// Hash.cpp - CHash, the WAP32 engine string/int-keyed hash table embedded in
// CSymTab (+0x38 child scopes, +0x40 leaf symbols). The original is a template
// CHash<T> over a non-template key-agnostic base; the two byte-identical
// instantiations are modeled as CHashBase + CHash ("_a") / CHashB ("_b"). See
// include/Bute/Hash.h for the layout + the call-graph evidence. Recovered from
// the trace group "ClassUnknown_13" (callers all in the ButeMgr parser region).
#include <rva.h>
#include <Dsndmgr/SoundVoiceList.h>

#include <Bute/Hash.h>
#include <Bute/SymParser.h>     // CParserObjList - the tail V0 slot below
#include <Bute/SymTab.h>        // CSymRecNode / CSymTabNode - the "_a"-int / "_b" element types
#include <Gruntz/ParseSource.h> // CParseSlotHashNode - the "_a"-string element type

// The CHashSlot ctor/dtor + the CHashBase slot machinery (0x1849d0-0x184b40) moved
// to src/Rez/RezColl.cpp (wave1-E: the 0x1832d0-pocket rez/sym/hash utility obj is
// ONE original TU; this file keeps only the 0x13c2xx CHash/CHashB key-typed lookups
// of the ButeMgr parser region).

// ---------------------------------------------------------------------------
// The ELEMENT-side virtual of each instantiation: CHashElement::Hash(). Retail
// emits each element type's Hash() beside its table's HashStr/Walk - 0x13c340 sits
// between Walk (..0x13c33a) and HashInt (0x13c350), and 0x13c3b0 between FindInt
// (..0x13c3a7) and CHashB::HashStr (0x13c3c0) - i.e. INTERLEAVED with this unit on
// both sides, which is what proves they are this TU's out-of-line member defs
// (docs: interleaved-comdat-methods) rather than RVA-proximity neighbours.
//
// All three are the same 15-byte forwarder, differing only in which instantiation's
// hash they statically call:
//     mov eax,[ecx+0x14] / mov ecx,[ecx+0x0c] / mov edx,[eax] / push edx / call / ret
// The `static_cast<CHash*>(m_owner)` downcast is the original's, not a modelling
// artifact: m_owner is declared CHashBase* on the shared element base, while retail
// calls the CONCRETE instantiation's HashStr/HashInt at three distinct RVAs
// (0x13c240 / 0x13c350 / 0x13c3c0) with a direct `call` - a statically-bound member
// call that only type-checks once the base pointer is narrowed to its instantiation.
// ---------------------------------------------------------------------------

// CParseSlotHashNode::Hash (0x13c230): the parse-slot table's element - string key.
RVA(0x0013c230, 0xf)
u32 CParseSlotHashNode::Hash() {
    return static_cast<CHash*>(m_owner)->HashStr(*static_cast<const char**>(m_record));
}

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
            const char* key = *static_cast<const char**>(e->m_record);
            if (_strcmpi(key, name) == 0) {
                return e->m_record;
            }
            e = FromLink(e->m_link.m_next);
        }
        return 0;
    }
    while (e) {
        const char* key = *static_cast<const char**>(e->m_record);
        if (strcmp(key, name) == 0) {
            return e->m_record;
        }
        e = FromLink(e->m_link.m_next);
    }
    return 0;
}

// CSymRecNode::Hash (0x13c340): the leaf-symbol table's element - INT key, so it
// forwards to HashInt (0x13c350) rather than HashStr. Interleaved between Walk and
// HashInt, which is what homes it here.
RVA(0x0013c340, 0xf)
u32 CSymRecNode::Hash() {
    return static_cast<CHash*>(m_owner)->HashInt(*static_cast<u32*>(m_record));
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
        if (*static_cast<u32*>(e->m_record) == key) {
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

// CSymTabNode::Hash (0x13c3b0): the child-scope table's element - string key on the
// "_b" instantiation. Interleaved between FindInt and CHashB::HashStr.
RVA(0x0013c3b0, 0xf)
u32 CSymTabNode::Hash() {
    return static_cast<CHashB*>(m_owner)->HashStr(*static_cast<const char**>(m_record));
}

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
            const char* key = *static_cast<const char**>(e->m_record);
            if (_strcmpi(key, name) == 0) {
                return e->m_record;
            }
            e = FromLink(e->m_link.m_next);
        }
        return 0;
    }
    while (e) {
        const char* key = *static_cast<const char**>(e->m_record);
        if (strcmp(key, name) == 0) {
            return e->m_record;
        }
        e = FromLink(e->m_link.m_next);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CParserObjList::V0 (0x13c4c0) - the parser object-list's empty CObjList slot-0
// override (??_7CParserObjList @0x1ef75c, [0]).
//
// TU EVIDENCE (weaker than the interleaves above - stated, not overstated): it is
// NOT interleaved, it sits at this unit's contiguous edge (CHashB::Walk ends at
// 0x13c4ba, V0 is the next 0x10-aligned slot). Its 1-byte twin CRezList::V0
// @0x13c4d0 is the immediately-following function but belongs to `rezfile`, and the
// two classes' vtables (0x1ef75c vs 0x1ef7c8) sit in different unit groups - so the
// adjacency is a link-order artifact, not shared ownership, which leaves this unit
// as the only contiguous home. (MSVC5 has no /OPT:ICF, so these two identical
// 1-byte bodies are genuinely distinct functions, not one folded copy.)
RVA(0x0013c4c0, 0x1)
void CParserObjList::V0() {}
