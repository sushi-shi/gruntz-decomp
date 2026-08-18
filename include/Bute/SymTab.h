#ifndef SRC_BUTE_SYMTAB_H
#define SRC_BUTE_SYMTAB_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

struct CSymTabNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;

    CSymTabNode() {
        m_symTab = NULL;
    }
};

void __stdcall UnpackTag(RezTypeTag tag, char* dst);

class CSymParser;

class CSymTab;

struct CSymRecNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;
    CSymRecNode() {
        m_symRec = NULL;
    }
};

class CSymRec {
public:
    CSymRec(i32 key, CSymTab* owner, i32 c, i32 d);
    CSymRec(i32 key, CSymTab* owner, i32 c);

    // No class-level operator new/delete: retail's unwind funclet for
    // `new CSymRec` calls the GLOBAL ??3@YAXPAX@Z, which a class-level
    // forwarder cannot produce (cl emits ??3CSymRec@@SAXPAX@Z and the
    // funclet calls that).
    ~CSymRec();

    i32 m_key;
    CSymRecNode m_symNode;
    CHash m_keyTable;
    CHashC m_valTable;
    CSymTab* m_scope;
};

struct CParseSource;
class CRezItmBase;

class CSymTab {
public:
    CSymTab(
        CSymParser* owner,
        CSymTab* parent,
        const char* name,
        i32 dataOff,
        i32 dataSize,
        i32 dirTime,
        i32 subN,
        i32 symN
    );

    ~CSymTab();

    CSymTab* CreateSub(const char* name);

    CSymRec* FindOrAddSym(i32 key);

    struct CParseSource* AddNamedValue(void* unused, void* name, i32 key);

    i32 ApplyRecursive(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates);

    i32 ApplyRange(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates);

    CParseSource* AddNodeEntry(u32 key, const char* name, CSymRec* rec, CRezItmBase* stream);

    i32 AddNodeSubEntry(void* rec, void* found);

    CSymTab* FindSub(const char* name);

    CSymTab* ResolvePath(const char* path);

    struct CParseSource* ResolveQualified(const char* name, RezTypeTag fourcc);

    struct CParseSource* Insert(const char* key, RezTypeTag fourcc);

    CParseSource* Find(const char* key);

    CParseSource* FindQualified(const char* name);

    i32 ReleaseParseBuffers(i32 recurse);

    CSymTab* FirstSub();
    CSymTab* NextSub(CSymTab* rec);
    CSymRec* FindSymKey(u32 key);
    CSymRec* FirstSym();
    CSymRec* NextSym(CSymRec* rec);
    CParseSource* NextSym2(CSymRec* rec);
    CParseSource* NextSym3(CParseSource* rec);

    char* m_name;

    i32 m_dataOff;
    i32 m_dataSize;
    i32 m_baseOffset;

    i32 m_totalSourceLength;

    i32 m_dirTime;
    CSymParser* m_owner;

    CSymTab* m_parent;

    CSymTabNode m_node20;
    CHashB m_subTabs;
    CHashD m_symbols;
    char* m_mappedBuf;
};

#endif // SRC_BUTE_SYMTAB_H
