#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

#include <rva.h>

#include <Bute/ButeStore.h>
#include <Bute/ButeTail.h>
#include <Bute/ButeToken.h>
#include <Bute/ButeTree.h>
#include <Bute/ButeValue.h>
#include <Bute/PTreeNode.h>
#include <Gruntz/String.h>
#include <Wap32/ZVec.h>

GZ_ENUM_FORWARD(ButeLexAction);

void ButeStoreFreeAdapter(void* p);
struct CBSecStream : zPTree {
    CBSecStream() : zPTree(&ButeStoreFreeAdapter, 2) {}
    virtual ~CBSecStream() OVERRIDE {}
};

#include <stdlib.h>

typedef void(__cdecl* ErrCallback)(const char*);

class istream;
class iostream;

class CButeMgr {
public:
    i32 GetIntDef(const char* tag, const char* key, i32 def);
    i32 GetInt(const char* tag, const char* key);
    DWORD GetDwordDef(const char* tag, const char* key, DWORD def);
    DWORD GetDword(const char* tag, const char* key);
    float GetFloatDef(const char* tag, const char* key, float def);
    float GetFloat(const char* tag, const char* key);
    double GetDoubleDef(const char* tag, const char* key, double def);
    double GetDouble(const char* tag, const char* key);
    CString* GetStringDef(const char* tag, const char* key, CString* def);
    CString* GetString(const char* tag, const char* key);

    struct ButeIntRect* GetRect(const char* tag, const char* key, struct ButeIntRect* def);
    struct ButeIntPoint* GetPoint(const char* tag, const char* key, struct ButeIntPoint* def);
    struct ButeDoubleVector*
    GetVector(const char* tag, const char* key, struct ButeDoubleVector* def);
    struct ButeDoubleRange* GetRange(const char* tag, const char* key, struct ButeDoubleRange* def);

    bool ScanToken(ButeToken expectType);
    bool ParseTagLine();
    bool Parse();

    bool Parse(CString filename, int streamBase);

    bool Save();

    void ReportError(const char* fmt, ...);

    CButeMgr();

    void Init();

    void Term();

    void SetErrCallback(ErrCallback cb);

    void NextChar();

    i16 CharClass(char c);

    GZ_ENUM_RETURN(ButeLexAction, i16) GetLexAction(i16 state, char c);
    i16 GetTransitionTarget(i16 state, char c);
    void AcceptTransition(i16 state, char c);

    bool SkipToTag();

    bool ParseGroup();

    void SetPoint(const char* tag, const char* key, struct ButeIntPoint* val);

    void SetInt(const char* tag, const char* key, i32 val);
    void SetDword(const char* tag, const char* key, DWORD val);
    void SetFloat(const char* tag, const char* key, float val);
    void SetDouble(const char* tag, const char* key, double val);
    void SetString(const char* tag, const char* key, const CString& val);
    void SetRect(const char* tag, const char* key, struct ButeIntRect* val);
    void SetVector(const char* tag, const char* key, struct ButeDoubleVector* val);
    void SetRange(const char* tag, const char* key, struct ButeDoubleRange* val);

    bool Exists(const char* tag, const char* key);

    RVA(0x000213d0, 0x14c)
    ~CButeMgr() {}

    CBSecStream* Tags() {
        return &m_tags;
    }

    CBSecStream* ModifiedTags() {
        return &m_modifiedTags;
    }

    i32 m_streamBase;
    i32 m_pos;
    i32 m_lineNo;
    bool m_countLine;

    char m_parseFailed;
    char m_pad0e[0x10 - 0xe];
    CString m_errStr;
    ErrCallback m_errCallback;
    CBSecStream m_tags;

    zPTree* m_currentTag;
    CBSecStream m_modifiedTags;
    CBSecStream m_addedTags;
    istream* m_stream;

    iostream* m_pText;
    char m_curChar;
    char m_pada9;
    GZ_ENUM_STORAGE(ButeToken, i16) m_tokType;
    i16 m_lexState;
    char m_token[0x100 - 0xae];
    CString m_tagName;
    CString m_attributeName;
    CString m_filename;
    char m_captureText;
    char m_writeMode;

    char m_encrypted;
    CButeTail m_crypt;

    ButeIntRect* GetRect(const char* tag, const char* key);
    ButeIntPoint* GetPoint(const char* tag, const char* key);
    ButeDoubleVector* GetVector(const char* tag, const char* key);
    ButeDoubleRange* GetRange(const char* tag, const char* key);
};

class ButeMgr : public CButeMgr {
public:
    bool ParseAttributeFile();
};

extern CButeMgr g_buteMgr;

void ButeGroup_Apply(char* key, void* value, void* ctx);
void ButeTag_Apply(char* key, void* value, void* ctx);
#include <stdio.h>

extern i16 g_charClass[];
extern i16 g_transTable[97][49][3];
#endif // SRC_BUTE_BUTEMGR_H
