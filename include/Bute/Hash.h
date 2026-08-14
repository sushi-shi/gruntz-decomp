#ifndef SRC_BUTE_HASH_H
#define SRC_BUTE_HASH_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <string.h>

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

class CHashBase {
public:
    CHashBase() {}
    CHashBase(i32 count);

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

// FOUR typed heads over the one CHashBase. Each carries its own inline
// `~X() { RemoveAll(); }`, so cl emits four 5-byte `jmp CHashBase::RemoveAll`
// COMDATs - 0x139c70, 0x139dd0, 0x139ec0 and 0x139ed0, all interleaved inside
// symtab.obj's contribution. One class can only produce ONE such COMDAT per TU,
// so four addresses prove four classes. The unwind funclets bind each address to
// a member, and the hash/lookup bodies split the same way:
//
//   0x139c70  CHash   CSymRec::m_keyTable   (this+0x1c)  ctor 0x184950
//   0x139dd0  CHashC  CSymRec::m_valTable   (this+0x24)  HashStr 0x13c240 Walk 0x13c270
//                     CSymParser::m_hash    (this+0x80)
//   0x139ec0  CHashB  CSymTab::m_subTabs    (this+0x38)  HashStr 0x13c3c0 Walk 0x13c3f0
//   0x139ed0  CHashD  CSymTab::m_symbols    (this+0x40)  HashInt 0x13c350 FindInt 0x13c360
//
// The retail NAMES are unrecoverable (each is an inline whose only image trace is
// the dtor COMDAT); the letters follow the CMapArrayA/CMapArrayB house convention.

// CSymRec::m_keyTable. Only the out-of-line default ctor and the inline dtor are
// reachable in the image - the key index is populated only when
// CSymParser::m_useKeyIndex is set, which nothing in retail does.
class CHash : public CHashBase {
public:
    CHash();

    CHash(i32 n) : CHashBase(n) {}

    RVA(0x00139c70, 0x5)
    ~CHash() {
        RemoveAll();
    }
};

// CSymTab::m_subTabs - CSymTab elements chained through CSymTabNode, keyed by name.
class CHashB : public CHashBase {
public:
    CHashB(i32 n) : CHashBase(n) {}
    RVA(0x00139ec0, 0x5)
    ~CHashB() {
        RemoveAll();
    }

    u32 HashStr(const char* s);
    void* Walk(const char* name, i32 ci);
};

// CSymRec::m_valTable and CSymParser::m_hash - CParseSource elements chained
// through CParseSlotHashNode, keyed by name.
class CHashC : public CHashBase {
public:
    CHashC(i32 n) : CHashBase(n) {}
    RVA(0x00139dd0, 0x5)
    ~CHashC() {
        RemoveAll();
    }

    u32 HashStr(const char* s);
    void* Walk(const char* name, i32 ci);
};

// CSymTab::m_symbols - CSymRec elements chained through CSymRecNode, keyed by the
// integer symbol key.
class CHashD : public CHashBase {
public:
    CHashD(i32 n) : CHashBase(n) {}
    RVA(0x00139ed0, 0x5)
    ~CHashD() {
        RemoveAll();
    }

    u32 HashInt(u32 key);
    // Generic integer-keyed lookup: one caller passes a REZ fourcc, another an
    // arbitrary symbol key, so the parameter is NOT a single domain.
    void* FindInt(u32 key);
};

#endif // SRC_BUTE_HASH_H
