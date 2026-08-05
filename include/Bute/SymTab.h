#ifndef SRC_BUTE_SYMTAB_H
#define SRC_BUTE_SYMTAB_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

void* operator new(u32 size);

struct CSymTabNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;

    CSymTabNode() {
        m_symTab = NULL;
    }
};
SIZE(0x18);

void __stdcall UnpackTag(RezTypeTag tag, char* dst);

class CSymParser;

class CSymTab;

struct CSymRecNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;
    CSymRecNode() {
        m_symRec = NULL;
    }
};
SIZE(0x18);

class CSymRec {
public:
    CSymRec(i32 key, CSymTab* owner, i32 c, i32 d);
    CSymRec(i32 key, CSymTab* owner, i32 c);

    ~CSymRec();
    void* operator new(u32 n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_key;
    CSymRecNode m_symNode;
    CHash m_keyTable;
    CHash m_valTable;
    CSymTab* m_scope;
};
SIZE(0x30);

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

    void* operator new(u32 n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    CSymTab* CreateSub(const char* name);

    CSymRec* FindOrAddSym(i32 key);

    struct CParseSource* AddNamedValue(void* unused, void* name, i32 key);

    i32 ApplyRecursive(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates);

    i32 ApplyRange(CRezItmBase* stream, i32 dataOff, i32 dataSize, i32 mergeDuplicates);

    CParseSource* AddNodeEntry(u32 key, const char* name, CSymRec* rec, CRezItmBase* stream);

    i32 AddNodeSubEntry(void* rec, void* found);

    void* FindSub(const char* name);

    void* ResolvePath(const char* path);

    struct CParseSource* ResolveQualified(const char* name, RezTypeTag fourcc);

    struct CParseSource* Insert(const char* key, RezTypeTag fourcc);

    void* Find(const char* key);

    void* FindQualified(const char* name);

    i32 ReleaseParseBuffers(i32 recurse);

    void* FirstSub();
    void* NextSub(void* rec);
    void* FindSymKey(u32 key);
    void* FirstSym();
    void* NextSym(void* rec);
    void* NextSym2(void* rec);
    void* NextSym3(void* rec);

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
    CHash m_symbols;
    char* m_mappedBuf;
};
SIZE(0x4c);

extern "C" const char g_sepSlash[];

#endif // SRC_BUTE_SYMTAB_H
