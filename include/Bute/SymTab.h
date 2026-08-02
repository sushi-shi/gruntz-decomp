#ifndef SRC_BUTE_SYMTAB_H
#define SRC_BUTE_SYMTAB_H

#include <Ints.h>
#include <rva.h>

#include <Bute/Hash.h>
#include <Rez/RezAlloc.h>

void* operator new(u32 size);

struct CSymTabNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;

    CSymTabNode() {
        m_symTab = 0;
    }
};
SIZE(0x18);

void __stdcall UnpackTag(u32 tag, char* dst);

class CSymParser;

class CSymTab;

struct CSymRecNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;
    CSymRecNode() {
        m_symRec = 0;
    }
};
SIZE(0x18);

class CSymRec {
public:
    CSymRec(i32 key, CSymTab* owner, i32 c, i32 d);
    CSymRec(i32 key, CSymTab* owner, i32 c);

    ~CSymRec();
    void* operator new(u32 n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
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
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
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

    struct CParseSource* ResolveQualified(const char* name, i32 fourcc);

    struct CParseSource* Insert(const char* key, u32 fourcc);

    void* Find(const char* key);

    void* FindQualified(const char* name);

    i32 ReleaseParseBuffers(i32 recurse);

    void* FirstSub();
    void* NextSub(void* rec);
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

extern "C" i32 _stat(const char* path, void* statbuf);

#endif // SRC_BUTE_SYMTAB_H
