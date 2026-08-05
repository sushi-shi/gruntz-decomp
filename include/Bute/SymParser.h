#ifndef SRC_BUTE_SYMPARSER_H
#define SRC_BUTE_SYMPARSER_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Bute/ObjListBase.h>
#include <Bute/SymTab.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezList.h>
#include <Rez/RezTypeTag.h>

class CRezItmBase;

struct CParserObjList : public CObjList {
    virtual void UnusedListHook() OVERRIDE;
    ~CParserObjList() {}
    i32 m_count;
};
SIZE(0x10);

struct CParseSource;

struct CSlotNode : public DSoundLink {
    CParseSource* m_buffer;
};
SIZE(0xc);

struct CParserHash : public CHashBase {

    ~CParserHash() {
        RemoveAll();
    }
};
SIZE(0x8);

#pragma pack(push, 1)
struct SymTabFileHeader {
    u8 m_magic0;
    char m_pad001[0x3f - 0x01];
    u8 m_magic3f;
    char m_pad040[0x7e - 0x40];
    u8 m_magic7e;
    i32 m_version;
    i32 m_rootDataOffset;
    i32 m_rootDataSize;
    i32 m_rootDirTime;
    i32 m_nextWritePos;
    i32 m_archiveTime;
    u32 m_largestKeyArraySize;
    u32 m_longestScopeNameLen;
    u32 m_longestLeafNameLen;
    u32 m_largestCommentSize;
    u8 m_sorted;
};
SIZE(0xa8);

#pragma pack(pop)

class CSymParser {
public:
    virtual i32 UnusedParserQuery(i32 a);
    virtual void UnusedParserAction(i32 a);

    virtual i32 Retry();

    CSymParser();
    CSymParser(void* buf, i32 a2, i32 a3);

    ~CSymParser();

    i32 Clear(i32 final);

    CSymTab* GetRoot();

    i32 ParseBuffer(void* buf, i32 a, i32 b);

    i32 LoadEntry(char* name, i32 flag);

    i32 ParseRecords(void* reader, CSymTab* node, char* path, i32 flag);

    i32 Classify(char* buf);

    u32 PackTag(const char* s);

    i32 ReParse();

    CParseSource* PopParseSlot();

    i32 MakeTimestamp();

    i32 CheckNodes();

    void SetDelims(char* s);

    void SetBucketCounts(i32 valueBuckets, i32 keyBuckets, i32 subTabBuckets, i32 symbolBuckets);

    void* FindQualified(const char* name);
    struct CParseSource* ResolveQualified(const char* name, RezTypeTag arg);
    void* ResolvePath(const char* path);
    void AddNode(void* rec);

    char* m_delims;
    i32 m_sorted;
    i32 m_parseArmed;
    CParserObjList m_list;
    CRezItmBase* m_activeNode;
    i32 m_reserved24;
    i32 m_nextGeneratedFileKey;
    i32 m_maxOpenFiles;
    i32 m_rootDataOffset;
    i32 m_rootDataSize;
    i32 m_rootDirTime;
    i32 m_nextWritePos;
    i32 m_readOnly;
    CSymTab* m_root;
    i32 m_archiveTime;
    i32 m_newArchive;
    i32 m_version;

    u32 m_largestKeyArraySize;
    u32 m_longestScopeNameLen;
    u32 m_longestLeafNameLen;
    u32 m_largestCommentSize;
    char* m_cachedSourceBuffer;
    i32 m_caseSensitive;
    i32 m_useKeyIndex;
    i32 m_valueBucketCount;
    i32 m_keyBucketCount;
    i32 m_subTabBucketCount;
    i32 m_symbolBucketCount;
    CParserHash m_hash;
    DSoundList m_nodes;
    i32 m_parseSlotBlockCount;
};
SIZE(0x94);

#endif // SRC_BUTE_SYMPARSER_H
