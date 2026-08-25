#ifndef SRC_BUTE_HASH_H
#define SRC_BUTE_HASH_H

#include <rva.h>

#include <Dsndmgr/IntrusiveList.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <string.h>

class CHashBase;
class CRezArchiveType;
class CRezArchiveDir;
struct CRezArchiveEntry;

struct CHashSlot {

    CHashSlot();

    ~CHashSlot();

    char m_pad00[0x8];
    IntrusiveList m_chain;
};

class CHashElement {
public:
    virtual u32 Hash();

    CHashElement* Next();

    CHashElement* Prev();

    IntrusiveLink m_link;
    CHashBase* m_hash;
    u32 m_bucketIndex;
    union {
        CRezArchiveEntry* m_archiveEntry;
        CRezArchiveType* m_archiveType;
        CRezArchiveDir* m_archiveDirectory;
    };
};

class CHashBase {
public:
    CHashBase() {}
    CHashBase(i32 bucketCount);

    CHashElement* First();

    CHashElement* Last();

    CHashElement* Lookup(u32 bucketIndex);

    void Remove(CHashElement* entry);

    void RemoveAll();

    void Insert(CHashElement* node);

    static CHashElement* FromLink(IntrusiveLink* link) {
        return ElementFromLink<CHashElement>(link);
    }

    u32 m_bucketCount;
    CHashSlot* m_buckets;
};

// FOUR typed heads over the one CHashBase. Each carries its own inline
// `~X() { RemoveAll(); }`, so cl emits four 5-byte `jmp CHashBase::RemoveAll`
// COMDATs - 0x139c70, 0x139dd0, 0x139ec0 and 0x139ed0, all interleaved inside
// rezarchive.obj's contribution. One class can only produce ONE such COMDAT per TU,
// so four addresses prove four classes. The unwind funclets bind each address to
// a member, and the hash/lookup bodies split the same way:
//
//   0x139c70  CHash   CRezArchiveType::m_idIndex     (this+0x1c)  ctor 0x184950
//   0x139dd0  CHashC  CRezArchiveType::m_nameIndex   (this+0x24)  HashStr 0x13c240 FindByName 0x13c270
//                     CRezArchive::m_freeEntries     (this+0x80)
//   0x139ec0  CHashB  CRezArchiveDir::m_subdirectories (this+0x38) HashStr 0x13c3c0 FindByName 0x13c3f0
//   0x139ed0  CHashD  CRezArchiveDir::m_types        (this+0x40)  HashTypeTag 0x13c350 FindTypeByTag 0x13c360
//
// The retail NAMES are unrecoverable (each is an inline whose only image trace is
// the dtor COMDAT); the letters follow the CMapArrayA/CMapArrayB house convention.

// CRezArchiveType::m_idIndex. Only the out-of-line default ctor and the inline dtor are
// reachable in the image - the resource-id index is populated only when
// CRezArchive::m_useIdIndex is set, which nothing in retail does.
class CHash : public CHashBase {
public:
    CHash();

    CHash(i32 bucketCount) : CHashBase(bucketCount) {}

    RVA(0x00139c70, 0x5)
    ~CHash() {
        RemoveAll();
    }
};

// CRezArchiveDir::m_subdirectories - archive directories keyed by name.
class CHashB : public CHashBase {
public:
    CHashB(i32 bucketCount) : CHashBase(bucketCount) {}
    RVA(0x00139ec0, 0x5)
    ~CHashB() {
        RemoveAll();
    }

    u32 HashStr(const char* text);
    CRezArchiveDir* FindByName(const char* name, i32 caseInsensitive);
};

// CRezArchiveType::m_nameIndex and CRezArchive::m_freeEntries - archive entries keyed by name.
class CHashC : public CHashBase {
public:
    CHashC(i32 bucketCount) : CHashBase(bucketCount) {}
    RVA(0x00139dd0, 0x5)
    ~CHashC() {
        RemoveAll();
    }

    u32 HashStr(const char* text);
    CRezArchiveEntry* FindByName(const char* name, i32 caseInsensitive);
};

// CRezArchiveDir::m_types - archive resource types keyed by the integer type tag.
class CHashD : public CHashBase {
public:
    CHashD(i32 bucketCount) : CHashBase(bucketCount) {}
    RVA(0x00139ed0, 0x5)
    ~CHashD() {
        RemoveAll();
    }

    u32 HashTypeTag(u32 typeTag);
    CRezArchiveType* FindTypeByTag(u32 typeTag);
};

#endif // SRC_BUTE_HASH_H
