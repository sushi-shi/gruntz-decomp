#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

#include <rva.h>

#include <Bute/ButeStore.h>
#include <Bute/ButeToken.h>
#include <Bute/ButeTree.h>
#include <Bute/ButeValue.h>
#include <Bute/PTreeNode.h>
#include <Crypto/CryptMgr.h>
#include <Gruntz/String.h>
#include <Rez/RezArchiveEntry.h>
#include <Wap32/ZVec.h>

GZ_ENUM_FORWARD(ButeLexAction);

void ButeStoreFreeAdapter(void* p);
struct CBSecStream : zPTree {
    CBSecStream() : zPTree(&ButeStoreFreeAdapter, 2) {}
    virtual ~CBSecStream() OVERRIDE {}
};

#include <stdlib.h>
#include <strstrea.h>

typedef void(__cdecl* ErrCallback)(const char*);

class istream;
class iostream;

class CButeMgr {
public:
    i32 GetInt(const char* tag, const char* key, i32 def);
    i32 GetInt(const char* tag, const char* key);
    DWORD GetDword(const char* tag, const char* key, DWORD def);
    DWORD GetDword(const char* tag, const char* key);
    float GetFloat(const char* tag, const char* key, float def);
    float GetFloat(const char* tag, const char* key);
    double GetDouble(const char* tag, const char* key, double def);
    double GetDouble(const char* tag, const char* key);
    CString* GetString(const char* tag, const char* key, CString* def);
    CString* GetString(const char* tag, const char* key);

    struct ButeIntRect* GetRect(const char* tag, const char* key, struct ButeIntRect* def);
    struct ButeIntPoint* GetPoint(const char* tag, const char* key, struct ButeIntPoint* def);
    CAVector* GetVector(const char* tag, const char* key, CAVector* def);
    CARange* GetRange(const char* tag, const char* key, CARange* def);

    bool Match(ButeToken expectType);
    bool ScanTok();

    bool Parse(CString filename, int streamBase);
    bool Parse(CRezItm* stream, const char* key);

    bool Save();

    void DisplayMessage(const char* fmt, ...);

    CButeMgr();

    void Reset();

    void Term();

    void Init(ErrCallback cb);

    void ConsumeChar();

    i16 CharClass(char c);

    GZ_ENUM_RETURN(ButeLexAction, i16) Action(i16 state, char c);
    i16 NextState(i16 state, char c);
    void LookupCodes(i16 state, char c);

    bool Statement();
    bool StatementList();
    bool Tag();
    bool TagList();

    void SetPoint(const char* tag, const char* key, struct ButeIntPoint* val);

    void SetInt(const char* tag, const char* key, i32 val);
    void SetDword(const char* tag, const char* key, DWORD val);
    void SetFloat(const char* tag, const char* key, float val);
    void SetDouble(const char* tag, const char* key, double val);
    void SetString(const char* tag, const char* key, const CString& val);
    void SetRect(const char* tag, const char* key, struct ButeIntRect* val);
    void SetVector(const char* tag, const char* key, CAVector* val);
    void SetRange(const char* tag, const char* key, CARange* val);

    bool Exist(const char* tag, const char* key);

    DWORD GetChecksum() {
        return m_checksum;
    }

    RVA(0x000213c0, 0x14c)
    ~CButeMgr() {}

    CBSecStream* Tags() {
        return &m_tagTab;
    }

    CBSecStream* ModifiedTags() {
        return &m_auxTagTab;
    }

    DWORD m_decryptCode;
    DWORD m_checksum;
    i32 m_lineNumber;
    bool m_bLineCounterFlag;

    bool m_bErrorFlag;
    char m_pad0e[0x10 - 0xe];
    CString m_sErrorString;
    ErrCallback m_pDisplayFunc;
    CBSecStream m_tagTab;

    zPTree* m_pCurrTabOfItems;
    CBSecStream m_auxTagTab;
    CBSecStream m_newTagTab;
    istream* m_pData;

    iostream* m_pSaveData;
    char m_currentChar;
    char m_pada9;
    GZ_ENUM_STORAGE(ButeToken, i16) m_token;
    i16 m_tokenMinor;
    char m_szTokenString[0x100 - 0xae];
    CString m_sTagName;
    CString m_sAttribute;
    CString m_sAttributeFilename;
    bool m_bPutChar;
    bool m_writeMode;

    bool m_bCrypt;
    CCryptMgr m_cryptMgr;

    ButeIntRect* GetRect(const char* tag, const char* key);
    ButeIntPoint* GetPoint(const char* tag, const char* key);
    CAVector* GetVector(const char* tag, const char* key);
    CARange* GetRange(const char* tag, const char* key);
};

inline bool CButeMgr::Parse(CRezItm* stream, const char* key) {
    if (stream == NULL) {
        return false;
    }

    m_bCrypt = 1;
    u8* encoded = stream->Load();
    i32 length = stream->GetSize();
    istrstream* input = new istrstream(static_cast<char*>(static_cast<void*>(encoded)), length);
    m_cryptMgr.SetKey(key);
    char* decoded = new char[length];
    ostrstream* output = new ostrstream(decoded, length, 2);
    m_cryptMgr.Decrypt(*input, *output);
    m_pData = new istrstream(decoded, output->pcount());
    delete input;
    delete output;
    stream->UnLoad();

    Reset();
    m_tagTab.Reset();
    m_auxTagTab.Reset();
    m_newTagTab.Reset();
    bool result = true;
    if (!TagList()) {
        m_bErrorFlag = 1;
        result = false;
    }
    delete m_pData;
    delete[] decoded;
    return result;
}

extern CButeMgr g_buteMgr;

void ButeGroup_Apply(char* key, void* value, void* ctx);
void ButeTag_Apply(char* key, void* value, void* ctx);
#include <stdio.h>

#endif // SRC_BUTE_BUTEMGR_H
