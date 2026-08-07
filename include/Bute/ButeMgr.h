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

GZ_ENUM_FORWARD(ButeLexAction);

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

    CButeMgr();

    void Init();

    // Empty in retail (0x170370 is a bare `ret`) and called exactly once, on the
    // `CButeMgr bute;` stack local in CChatBoxOwner::ProcessCheatInput right after
    // its ctor.  It is emitted between Init() and SetErrCallback(), i.e. it is
    // Init()'s counterpart whose real teardown lives in ~CButeMgr.
    void Term();

    void SetErrCallback(ErrCallback cb);

    void NextChar();

    i16 CharClass(char c);

    GZ_ENUM_RETURN(ButeLexAction, i16) PeekState(i16 state, char c);
    i16 PeekState2(i16 state, char c);
    void ScanState(i16 state, char c);

    bool SkipToTag();

    bool ParseGroup();

    void SetValue(const char* tag, const char* key, struct CButeValue* val);

    // The Set<T> family; retail lays each one out directly after its Get<T>.
    void SetInt(const char* tag, const char* key, i32 val);
    void SetDword(const char* tag, const char* key, DWORD val);
    void SetFloat(const char* tag, const char* key, float val);
    void SetDouble(const char* tag, const char* key, double val);
    void SetString(const char* tag, const char* key, const CString& val);
    void SetRect(const char* tag, const char* key, struct ButeIntRect* val);
    void SetVector(const char* tag, const char* key, struct ButeDoubleVector* val);
    void SetRange(const char* tag, const char* key, struct ButeDoubleRange* val);

    bool Exists(const char* tag, const char* key);

    // Inline in retail: ProcessCheatInput 0x205c0 expands the member teardown on its
    // normal path (3x ~zErrHandling + 3x ~zPtrColl) and only its EH funclet calls the
    // out-of-line COMDAT at 0x213c0.
    ~CButeMgr() {}

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
