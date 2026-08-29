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

#include <stdlib.h>
#include <strstrea.h>

typedef void(__cdecl* ErrCallback)(const char*);

class istream;
class iostream;

class CButeMgr {
public:
    GZ_ENUM_BEGIN(SymTypes)
        INT_TYPE = 0,
        DWORD_TYPE = 1,
        DOUBLE_TYPE = 2,
        FLOAT_TYPE = 3,
        STRING_TYPE = 4,
        RECT_TYPE = 5,
        POINT_TYPE = 6,
        VECTOR_TYPE = 7,
        RANGE_TYPE = 8
    GZ_ENUM_END(SymTypes)

    class CSymTabItem {
    public:
        SymTypes SymType;

        CSymTabItem() {}

        CSymTabItem(SymTypes t, ButeIntPoint* src) {
            SymType = t;
            data.point = new ButeIntPoint(*src);
        }
        CSymTabItem(SymTypes t, i32 val) {
            SymType = t;
            data.i = new i32(val);
        }
        CSymTabItem(SymTypes t, DWORD val) {
            SymType = t;
            data.dw = new DWORD(val);
        }
        CSymTabItem(SymTypes t, float val) {
            SymType = t;
            data.f = new float(val);
        }
        CSymTabItem(SymTypes t, double val) {
            SymType = t;
            data.d = new double(val);
        }
        CSymTabItem(SymTypes t, const CString& val) {
            SymType = t;
            data.s = new CString(val);
        }
        CSymTabItem(SymTypes t, ButeIntRect* src) {
            SymType = t;
            data.r = new ButeIntRect(*src);
        }
        CSymTabItem(SymTypes t, CAVector* src) {
            SymType = t;
            data.v = new CAVector(*src);
        }
        CSymTabItem(SymTypes t, CARange* src) {
            SymType = t;
            data.range = new CARange(*src);
        }

        ~CSymTabItem();
        const CSymTabItem& operator=(const CSymTabItem& item);

        union {
            i32* i;
            DWORD* dw;
            double* d;
            float* f;
            CString* s;
            ButeIntRect* r;
            ButeIntPoint* point;
            CAVector* v;
            CARange* range;
        } data;
    };

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

private:
    typedef zSymTab<CSymTabItem> TableOfItems;
    typedef zSymTab<TableOfItems> TableOfTags;

    TableOfTags* Tags() {
        return &m_tagTab;
    }

    TableOfTags* ModifiedTags() {
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
    TableOfTags m_tagTab;

    TableOfItems* m_pCurrTabOfItems;
    TableOfTags m_auxTagTab;
    TableOfTags m_newTagTab;

    static void AuxTabItemsSave(const char* key, CSymTabItem* value, void* ctx);
    static void NewTabsSave(const char* key, TableOfItems* value, void* ctx);

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

public:
    ButeIntRect* GetRect(const char* tag, const char* key);
    ButeIntPoint* GetPoint(const char* tag, const char* key);
    CAVector* GetVector(const char* tag, const char* key);
    CARange* GetRange(const char* tag, const char* key);
};

RVA(0x00172040, 0x120)
inline const CButeMgr::CSymTabItem&
CButeMgr::CSymTabItem::operator=(const CButeMgr::CSymTabItem& item) {
    switch (SymType) {
        case INT_TYPE:
            *data.i = *item.data.i;
            break;
        case DWORD_TYPE:
            *data.dw = *item.data.dw;
            break;
        case DOUBLE_TYPE:
            *data.d = *item.data.d;
            break;
        case FLOAT_TYPE:
            *data.f = *item.data.f;
            break;
        case STRING_TYPE:
            *data.s = *item.data.s;
            break;
        case RECT_TYPE:
            *data.r = *item.data.r;
            break;
        case POINT_TYPE:
            *data.point = *item.data.point;
            break;
        case VECTOR_TYPE:
            *data.v = *item.data.v;
            break;
        case RANGE_TYPE:
            *data.range = *item.data.range;
            break;
    }
    return *this;
}

inline CButeMgr::CSymTabItem::~CSymTabItem() {
    switch (SymType) {
        case INT_TYPE:
            delete data.i;
            break;
        case DWORD_TYPE:
            delete data.dw;
            break;
        case DOUBLE_TYPE:
            delete data.d;
            break;
        case FLOAT_TYPE:
            delete data.f;
            break;
        case STRING_TYPE:
            delete data.s;
            break;
        case RECT_TYPE:
            delete data.r;
            break;
        case POINT_TYPE:
            delete data.point;
            break;
        case VECTOR_TYPE:
            delete data.v;
            break;
        case RANGE_TYPE:
            delete data.range;
            break;
    }
}

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
    m_tagTab.clear();
    m_auxTagTab.clear();
    m_newTagTab.clear();
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
#include <stdio.h>

#endif // SRC_BUTE_BUTEMGR_H
