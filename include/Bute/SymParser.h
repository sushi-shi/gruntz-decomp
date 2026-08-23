#ifndef SRC_BUTE_SYMPARSER_H
#define SRC_BUTE_SYMPARSER_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Bute/ObjListBase.h>
#include <Bute/SymTab.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezList.h>
#include <Rez/RezTypeTag.h>

class CRezItmBase;

struct CParserObjList : public CObjList {
    virtual void UnusedListHook() OVERRIDE;
    RVA(0x0013aaf0, 0x7)
    ~CParserObjList() {}
    // Unsigned: CRezDirNode::Load compares it with `ja`, not `jg`
    // (`sema disasm 0x0013a0f0 --branches --diff`).
    u32 m_count;
};

struct CParseSource;

struct CSlotNode : public DSoundLink {
    CParseSource* m_buffer;
};

// CSymParser::m_nodes (this+0x88). Its own empty destructor COMDAT - the lone
// `c3` at 0x13abb0, fenced by nop fill on both sides and reached from the unwind
// funclets of ??0CSymParser(char*,i32,i32) and ~CSymParser - so it is a distinct
// class from DSoundList, whose own `~DSoundList()` lives elsewhere.
struct CSlotNodeList : public DSoundList {
    RVA(0x0013abb0, 0x1)
    ~CSlotNodeList() {}
};

#pragma pack(push, 1)
GZ_ENUM_CONST_BEGIN(SymTabFileMagic)
    SYMTAB_MAGIC_CR = '\r',
    SYMTAB_MAGIC_LF = '\n',
    SYMTAB_MAGIC_EOF = 0x1a
GZ_ENUM_CONST_END(SymTabFileMagic)

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

#pragma pack(pop)

class CSymParser {
public:
    virtual i32 UnusedParserQuery(i32 a);
    virtual void UnusedParserAction(i32 a);

    virtual i32 Retry();

    CSymParser();
    CSymParser(char* buf, i32 readOnly, i32 createNew);

    ~CSymParser();

    i32 Clear(i32 final);

    CSymTab* GetRoot();

    i32 ParseBuffer(char* buf, i32 a, i32 b);

    i32 LoadEntry(char* name, i32 flag);

    i32 ParseRecords(CRezItmBase* reader, CSymTab* node, char* path, i32 flag);

    i32 Classify(char* buf);

    RezTypeTag PackTag(const char* s);

    void UnpackTag(RezTypeTag tag, char* dst);

    i32 ReParse();

    CParseSource* PopParseSlot();

    i32 MakeTimestamp();

    i32 CheckNodes();

    void SetDelims(char* s);

    void SetBucketCounts(i32 valueBuckets, i32 keyBuckets, i32 subTabBuckets, i32 symbolBuckets);

    CParseSource* FindQualified(const char* name);
    struct CParseSource* ResolveQualified(const char* name, RezTypeTag arg);
    CSymTab* ResolvePath(const char* path);
    void AddNode(CParseSource* rec);

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
    CHashC m_hash;
    CSlotNodeList m_nodes;
    i32 m_parseSlotBlockCount;
};

#endif // SRC_BUTE_SYMPARSER_H
