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

class CHashElement : public IntrusiveLink {
public:
    virtual u32 Hash();

    CHashElement* Next();

    CHashElement* Prev();

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

class CRezEntryIdHash : public CHashBase {
public:
    CRezEntryIdHash();

    CRezEntryIdHash(i32 bucketCount) : CHashBase(bucketCount) {}

    RVA(0x00139e80, 0x5)
    ~CRezEntryIdHash() {
        RemoveAll();
    }
};

class CRezDirectoryNameHash : public CHashBase {
public:
    CRezDirectoryNameHash(i32 bucketCount) : CHashBase(bucketCount) {}
    RVA(0x0013a0d0, 0x5)
    ~CRezDirectoryNameHash() {
        RemoveAll();
    }

    u32 HashStr(const char* text);
    CRezArchiveDir* FindByName(const char* name, i32 caseInsensitive);
};

class CRezEntryNameHash : public CHashBase {
public:
    CRezEntryNameHash(i32 bucketCount) : CHashBase(bucketCount) {}
    RVA(0x00139fe0, 0x5)
    ~CRezEntryNameHash() {
        RemoveAll();
    }

    u32 HashStr(const char* text);
    CRezArchiveEntry* FindByName(const char* name, i32 caseInsensitive);
};

class CRezTypeTagHash : public CHashBase {
public:
    CRezTypeTagHash(i32 bucketCount) : CHashBase(bucketCount) {}
    RVA(0x0013a0e0, 0x5)
    ~CRezTypeTagHash() {
        RemoveAll();
    }

    u32 HashTypeTag(u32 typeTag);
    CRezArchiveType* FindTypeByTag(u32 typeTag);
};

#endif // SRC_BUTE_HASH_H
