#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

#include <string>

#include <Ints.h>

#include <Bute/ButeStore.h>
#include <Bute/ButeToken.h>
#include <Bute/ButeValue.h>
#include <Crypto/CryptMgr.h>
#include <Gruntz/String.h>
#include <Rez/RezArchiveEntry.h>
#include <ZTools/Error.h>
#include <ZTools/PTree.h>

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
        SymTypes m_symType;

        CSymTabItem() {}

        CSymTabItem(SymTypes t, ButeIntPoint* src) {
            m_symType = t;
            m_data.m_point = new ButeIntPoint(*src);
        }
        CSymTabItem(SymTypes t, i32 val) {
            m_symType = t;
            m_data.m_i = new i32(val);
        }
        CSymTabItem(SymTypes t, DWORD val) {
            m_symType = t;
            m_data.m_dw = new DWORD(val);
        }
        CSymTabItem(SymTypes t, float val) {
            m_symType = t;
            m_data.m_f = new float(val);
        }
        CSymTabItem(SymTypes t, double val) {
            m_symType = t;
            m_data.m_d = new double(val);
        }
        CSymTabItem(SymTypes t, const std::string& val) {
            m_symType = t;
            m_data.m_s = new std::string(val);
        }
        CSymTabItem(SymTypes t, ButeIntRect* src) {
            m_symType = t;
            m_data.m_r = new ButeIntRect(*src);
        }
        CSymTabItem(SymTypes t, const CAVector& src) {
            m_symType = t;
            m_data.m_v = new CAVector(src);
        }
        CSymTabItem(SymTypes t, const CARange& src) {
            m_symType = t;
            m_data.m_range = new CARange(src);
        }

        ~CSymTabItem();
        const CSymTabItem& operator=(const CSymTabItem& item);

        union {
            i32* m_i;
            DWORD* m_dw;
            double* m_d;
            float* m_f;
            std::string* m_s;
            ButeIntRect* m_r;
            ButeIntPoint* m_point;
            CAVector* m_v;
            CARange* m_range;
        } m_data;
    };

    i32 GetInt(const std::string& tag, const std::string& key, i32 def);
    i32 GetInt(const std::string& tag, const std::string& key);
    DWORD GetDword(const std::string& tag, const std::string& key, DWORD def);
    DWORD GetDword(const std::string& tag, const std::string& key);
    float GetFloat(const std::string& tag, const std::string& key, float def);
    float GetFloat(const std::string& tag, const std::string& key);
    double GetDouble(const std::string& tag, const std::string& key, double def);
    double GetDouble(const std::string& tag, const std::string& key);
    std::string GetString(const std::string& tag, const std::string& key, const std::string& fallback);
    std::string GetString(const std::string& tag, const std::string& key);

    struct ButeIntRect* GetRect(const std::string& tag, const std::string& key, struct ButeIntRect* def);
    struct ButeIntPoint* GetPoint(const std::string& tag, const std::string& key, struct ButeIntPoint* def);
    CAVector& GetVector(const std::string& tag, const std::string& key, CAVector& def);
    CARange& GetRange(const std::string& tag, const std::string& key, CARange& def);

    bool Match(ButeToken expectType);
    bool ScanTok();

    bool Parse(const std::string& filename, int streamBase);
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

    void SetPoint(const std::string& tag, const std::string& key, struct ButeIntPoint* val);

    void SetInt(const std::string& tag, const std::string& key, i32 val);
    void SetDword(const std::string& tag, const std::string& key, DWORD val);
    void SetFloat(const std::string& tag, const std::string& key, float val);
    void SetDouble(const std::string& tag, const std::string& key, double val);
    void SetString(const std::string& tag, const std::string& key, const std::string& val);
    void SetRect(const std::string& tag, const std::string& key, struct ButeIntRect* val);
    void SetVector(const std::string& tag, const std::string& key, const CAVector& val);
    void SetRange(const std::string& tag, const std::string& key, const CARange& val);

    bool HasTag(const std::string& tag);
    bool Exist(const std::string& tag, const std::string& key);

    DWORD GetChecksum() {
        return m_checksum;
    }

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
    std::string m_sErrorString;
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
    GZ_ENUM_STORAGE(ButeToken, i16) m_token;
    i16 m_tokenMinor;
    char m_szTokenString[0x100 - 0xae];
    std::string m_sTagName;
    std::string m_sAttribute;
    std::string m_sAttributeFilename;
    bool m_bPutChar;
    bool m_writeMode;

    bool m_bCrypt;
    CCryptMgr m_cryptMgr;

public:
    ButeIntRect* GetRect(const std::string& tag, const std::string& key);
    ButeIntPoint* GetPoint(const std::string& tag, const std::string& key);
    CAVector& GetVector(const std::string& tag, const std::string& key);
    CARange& GetRange(const std::string& tag, const std::string& key);
};

inline const CButeMgr::CSymTabItem&
CButeMgr::CSymTabItem::operator=(const CButeMgr::CSymTabItem& item) {
    switch (m_symType) {
        case INT_TYPE:
            *m_data.m_i = *item.m_data.m_i;
            break;
        case DWORD_TYPE:
            *m_data.m_dw = *item.m_data.m_dw;
            break;
        case DOUBLE_TYPE:
            *m_data.m_d = *item.m_data.m_d;
            break;
        case FLOAT_TYPE:
            *m_data.m_f = *item.m_data.m_f;
            break;
        case STRING_TYPE:
            *m_data.m_s = *item.m_data.m_s;
            break;
        case RECT_TYPE:
            *m_data.m_r = *item.m_data.m_r;
            break;
        case POINT_TYPE:
            *m_data.m_point = *item.m_data.m_point;
            break;
        case VECTOR_TYPE:
            *m_data.m_v = *item.m_data.m_v;
            break;
        case RANGE_TYPE:
            *m_data.m_range = *item.m_data.m_range;
            break;
    }
    return *this;
}

inline CButeMgr::CSymTabItem::~CSymTabItem() {
    switch (m_symType) {
        case INT_TYPE:
            delete m_data.m_i;
            break;
        case DWORD_TYPE:
            delete m_data.m_dw;
            break;
        case DOUBLE_TYPE:
            delete m_data.m_d;
            break;
        case FLOAT_TYPE:
            delete m_data.m_f;
            break;
        case STRING_TYPE:
            delete m_data.m_s;
            break;
        case RECT_TYPE:
            delete m_data.m_r;
            break;
        case POINT_TYPE:
            delete m_data.m_point;
            break;
        case VECTOR_TYPE:
            delete m_data.m_v;
            break;
        case RANGE_TYPE:
            delete m_data.m_range;
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

#endif
