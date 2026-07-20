#include <rva.h>
#include <Dsndmgr/SoundVoiceList.h>

#include <Bute/Hash.h>
#include <Bute/SymParser.h>     // CParserObjList - the tail V0 slot below
#include <Bute/SymTab.h>        // CSymRecNode / CSymTabNode - the "_a"-int / "_b" element types
#include <Gruntz/ParseSource.h> // CParseSlotHashNode - the "_a"-string element type

RVA(0x0013c230, 0xf)
u32 CParseSlotHashNode::Hash() {
    return static_cast<CHash*>(m_owner)->HashStr(*static_cast<const char**>(m_record));
}

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

RVA(0x0013c340, 0xf)
u32 CSymRecNode::Hash() {
    return static_cast<CHash*>(m_owner)->HashInt(*static_cast<u32*>(m_record));
}

RVA(0x0013c350, 0xd)
u32 CHash::HashInt(u32 key) {
    return key % m_count;
}

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

RVA(0x0013c3b0, 0xf)
u32 CSymTabNode::Hash() {
    return static_cast<CHashB*>(m_owner)->HashStr(*static_cast<const char**>(m_record));
}

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

RVA(0x0013c4c0, 0x1)
void CParserObjList::V0() {}
