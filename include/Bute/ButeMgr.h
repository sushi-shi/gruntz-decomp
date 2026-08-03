#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

#include <rva.h>

#include <Bute/ButeStore.h>
#include <Bute/ButeToken.h>
#include <Bute/ButeTree.h>
#include <Bute/ButeValue.h>
#include <Bute/PTreeNode.h>
#include <Gruntz/String.h>
#include <Wap32/ZVec.h>

void ButeStoreFreeAdapter(void* p);
struct CBSecStream : zPTree {
    CBSecStream() : zPTree(&ButeStoreFreeAdapter, 2) {}
    virtual ~CBSecStream() OVERRIDE {}
};
SIZE(0x2c);

struct CButeTail {
    CButeTail();

    ~CButeTail();

    // Both crypto entry points are __thiscall members of this tag struct: every
    // retail call site (Save 0x171640, ProcessCheatInput 0x205c0, Run 0x83450)
    // loads ecx with a CButeTail lvalue before the call.
    void Decode(class istream* in, class ostream* out);
    void Encode(class istream* src, class ostream* dst);
};
SIZE(0x1);

struct ButeIntRect {
    ButeIntRect() : a(0), b(0), c(0), d(0) {}
    ~ButeIntRect() {}
    DWORD a, b, c, d;
};
SIZE(0x10);
struct ButeIntPoint {
    ButeIntPoint() : a(0), b(0) {}
    ~ButeIntPoint() {}
    DWORD a, b;
};
SIZE(0x8);

struct ButeDoubleVector {
    ButeDoubleVector() : a(0), b(0), c(0), d(0), e(0), f(0) {}
    ~ButeDoubleVector() {}
    union {
        struct {
            DWORD a, b;
        };
        double x;
    };
    union {
        struct {
            DWORD c, d;
        };
        double y;
    };
    union {
        struct {
            DWORD e, f;
        };
        double z;
    };
};
SIZE(0x18);

struct ButeDoubleRange {
    ButeDoubleRange() : a(0), b(0), c(0), d(0) {}
    ~ButeDoubleRange() {}
    union {
        struct {
            DWORD a, b;
        };
        double x;
    };
    union {
        struct {
            DWORD c, d;
        };
        double y;
    };
};
SIZE(0x10);

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
    char* GetString(const char* tag, const char* key);

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

    void* InvokeCallback(void* (*fn)(CButeMgr*));

    CButeMgr();

    void Init();

    void SetErrCallback(ErrCallback cb);

    void NextChar();

    i16 CharClass(char c);

    i16 PeekState(i16 state, char c);
    i16 PeekState2(i16 state, char c);
    void ScanState(i16 state, char c);

    bool SkipToTag();

    bool ParseGroup();

    void SetValue(const char* tag, const char* key, struct CButeValue* val);

    bool Exists(const char* tag, const char* key);

    ~CButeMgr();

    CBSecStream* Tree() {
        return &m_tree;
    }

    CBSecStream* Tree48() {
        return &m_tree48;
    }

    i32 m_streamBase;
    i32 m_pos;
    i32 m_lineNo;
    bool m_countLine;

    char m_parseFailed;
    char m_pad0e[0x10 - 0xe];
    CString m_errStr;
    ErrCallback m_errCallback;
    CBSecStream m_tree;

    zPTree* m_pNode;
    CBSecStream m_tree48;
    CBSecStream m_tree74;
    istream* m_stream;

    iostream* m_pText;
    char m_curChar;
    char m_pada9;
    GZ_ENUM_STORAGE(ButeToken, i16) m_tokType;
    i16 m_lexState;
    char m_token[0x100 - 0xae];
    CString m_tagName;
    CString m_str104;
    CString m_str108;
    char m_captureText;
    char m_writeMode;

    char m_encrypted;
    CButeTail m_crypt; // Blowfish stream codec (Decode/Encode are its members)

    ButeIntRect* GetRect(const char* tag, const char* key);
    ButeIntPoint* GetPoint(const char* tag, const char* key);
    ButeDoubleVector* GetVector(const char* tag, const char* key);
    ButeDoubleRange* GetRange(const char* tag, const char* key);
};
SIZE(0x110);

class ButeMgr : public CButeMgr {
public:
    bool ParseAttributeFile();
};
SIZE(0x110);

extern CButeMgr g_buteMgr;

extern "C" void ButeGroup_Apply(char* key, void* value, void* ctx);
extern "C" void ButeTag_Apply(char* key, void* value, void* ctx);
#include <stdio.h>

extern "C" i16 g_charClass[];
extern "C" i16 g_transTable[97][49][3];
#endif // SRC_BUTE_BUTEMGR_H
