#ifndef SRC_BUTE_HASH_H
#define SRC_BUTE_HASH_H

#include <Ints.h>
#include <rva.h>
#include <string.h>

#include <Dsndmgr/SoundVoiceList.h>

class CHashBase;
class CSymRec;
class CSymTab;
class CRezDirNode;
struct CParseSource;

struct CHashSlot {

    CHashSlot();

    ~CHashSlot();

    char m_pad00[0x8];
    DSoundList m_chain;
};
SIZE(0x10);

VTBL_ABSENT(CHashElement);
class CHashElement {
public:
    virtual u32 Hash();

    CHashElement* Next();

    CHashElement* Prev();

    DSoundLink m_link;
    CHashBase* m_owner;
    u32 m_bucket;
    union {
        CParseSource* m_parseSource;
        CSymRec* m_symRec;
        CSymTab* m_symTab;
        CRezDirNode* m_rezDirNode;
    };
};
SIZE(0x18);

class CHashBase {
public:
    CHashBase* Construct(i32 count);

    CHashElement* First();

    CHashElement* Last();

    CHashElement* Lookup(u32 idx);

    void Remove(CHashElement* entry);

    void RemoveAll();

    void Insert(CHashElement* node);

    static CHashElement* FromLink(DSoundLink* link) {
        return elemOf<CHashElement>(link);
    }

    u32 m_count;
    CHashSlot* m_buckets;
};
SIZE(0x8);

class CHash : public CHashBase {
public:
    CHash();

    CHash(i32 n) {
        Construct(n);
    }

    ~CHash() {
        RemoveAll();
    }

    u32 HashStr(const char* s);
    void* Walk(const char* name, i32 ci);
    u32 HashInt(u32 key);
    void* FindInt(u32 key);
};
SIZE(0x8);

class CHashB : public CHashBase {
public:
    CHashB(i32 n) {
        Construct(n);
    }
    ~CHashB() {
        RemoveAll();
    }

    u32 HashStr(const char* s);
    void* Walk(const char* name, i32 ci);
};
SIZE(0x8);

#endif // SRC_BUTE_HASH_H
