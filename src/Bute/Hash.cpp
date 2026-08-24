#include <rva.h>

#include <Bute/Hash.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <Dsndmgr/IntrusiveList.h>
#include <Enums.h>
#include <Gruntz/ParseSource.h>

RVA(0x0013c230, 0xf)
u32 CParseSlotHashNode::Hash() {
    return static_cast<CHashC*>(m_owner)->HashStr(m_parseSource->m_name);
}

RVA(0x0013c240, 0x29)
u32 CHashC::HashStr(const char* s) {
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
CParseSource* CHashC::Walk(const char* name, i32 ci) {
    if (!name) {
        return NULL;
    }
    CHashElement* e = Lookup(HashStr(name));
    if (ci) {
        while (e) {
            const char* key = e->m_parseSource->m_name;
            if (_strcmpi(key, name) == 0) {
                return e->m_parseSource;
            }
            e = FromLink(e->m_link.m_next);
        }
        return NULL;
    }
    while (e) {
        const char* key = e->m_parseSource->m_name;
        if (strcmp(key, name) == 0) {
            return e->m_parseSource;
        }
        e = FromLink(e->m_link.m_next);
    }
    return NULL;
}

RVA(0x0013c340, 0xf)
u32 CSymRecNode::Hash() {
    return static_cast<CHashD*>(m_owner)->HashInt(m_symRec->m_key);
}

RVA(0x0013c350, 0xd)
u32 CHashD::HashInt(u32 key) {
    return key % m_count;
}

RVA(0x0013c360, 0x47)
CSymRec* CHashD::FindInt(u32 key) {
    CHashElement* e = Lookup(HashInt(key));
    while (e) {
        if (static_cast<u32>(e->m_symRec->m_key) == key) {
            return e->m_symRec;
        }
        e = FromLink(e->m_link.m_next);
    }
    return NULL;
}

RVA(0x0013c3b0, 0xf)
u32 CSymTabNode::Hash() {
    return static_cast<CHashB*>(m_owner)->HashStr(m_symTab->m_name);
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
CSymTab* CHashB::Walk(const char* name, i32 ci) {
    if (!name) {
        return NULL;
    }
    CHashElement* e = Lookup(HashStr(name));
    if (ci) {
        while (e) {
            const char* key = e->m_symTab->m_name;
            if (_strcmpi(key, name) == 0) {
                return e->m_symTab;
            }
            e = FromLink(e->m_link.m_next);
        }
        return NULL;
    }
    while (e) {
        const char* key = e->m_symTab->m_name;
        if (strcmp(key, name) == 0) {
            return e->m_symTab;
        }
        e = FromLink(e->m_link.m_next);
    }
    return NULL;
}

RVA(0x0013c4c0, 0x1)
void CParserObjList::UnusedListHook() {}
