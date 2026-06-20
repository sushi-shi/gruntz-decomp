// FontConfig.cpp - CFontConfig::LoadFontConfig, the GDI HFONT setup path.
//
//   CFontConfig::LoadFontConfig - builds three
//     GDI HFONTs (the ARIAL UI font fixed at 12x8 bold; the TrainingFont; the
//     MessageFont) via CreateFontA (GDI32 IAT, cached in edi + called
//     indirectly 6x). The Training/Message face names + width/height are read from
//     the global CButeMgr "Font" config group through the matched
//     GetStringDef/GetIntDef getters (butemgr unit), defaulting to the ARIAL face
//     + per-font default dims. Each CreateFontA has a fallback retry (NULL face /
//     alternate default dims) if the first attempt fails. The three HFONTs are
//     stored at this+0x38/0x3c/0x40; the two ctor args at this+0x24/0x28.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Built /O2 /MT /GX: the ARIAL default-face
// CString temp (passed by address to GetStringDef as the default for both the
// Training and Message face lookups) is destroyed at the tail under a C++ EH frame
// (the target opens an fs:0 EH frame: push -1 / push handler / mov fs:0,esp).
// ---------------------------------------------------------------------------
#include <Font/Font.h>
#include <rva.h>

// CreateFontA - the GDI32 14-arg HFONT creator, reached through its IAT slot
// (the target caches it in edi and calls it indirectly six times).
extern "C" __declspec(dllimport) void *__stdcall CreateFontA(
    int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
    unsigned long fdwItalic, unsigned long fdwUnderline, unsigned long fdwStrikeOut,
    unsigned long fdwCharSet, unsigned long fdwOutputPrecision,
    unsigned long fdwClipPrecision, unsigned long fdwQuality,
    unsigned long fdwPitchAndFamily, const char *lpszFace);

// The global CButeMgr config tree. Modeled minimally so the
// `ecx=&g_buteMgr; call GetIntDef/GetStringDef` shapes reloc-mask against the
// already-matched butemgr getters. GetStringDef returns a CString* whose +0
// m_pchData is the face-name char* the caller dereferences (`mov eax,[eax]`); the
// default (3rd arg) is the address of the ARIAL CString temp.
       // +0x00 the face-name char*

#include <Bute/ButeMgr.h>
// The global CButeMgr instance (the ctor stores the bute config tree
// here). Declared as a named extern so the `mov ecx, offset g_buteMgr` loads
// reloc-match the engine; @address names the delinked target DATA symbol.
DATA(0x2453d8)
extern CButeMgr g_buteMgr;
#define g_bute (&g_buteMgr)

// The font config strings - the original source literals (the "Font" tag-group
// + per-font keys). objdiff matches these relocations by value against the
// target's .data string constants.
#define s_Font               "Font"
#define s_ARIAL              "ARIAL"
#define s_TrainingFont       "TrainingFont"
#define s_TrainingFontWidth  "TrainingFontWidth"
#define s_TrainingFontHeight "TrainingFontHeight"
#define s_MessageFont        "MessageFont"
#define s_MessageFontWidth   "MessageFontWidth"
#define s_MessageFontHeight  "MessageFontHeight"

// ---------------------------------------------------------------------------
// The font-config object the method operates on. Only the load-bearing member
// offsets are reconstructed (this+0x20..0x40).
// ---------------------------------------------------------------------------
class CFontConfig {
public:
    int LoadFontConfig(int a1, int a2);

    char  m_pad00[0x20];
    int   m_20;          // +0x20  (= 0)
    int   m_24;          // +0x24  (= a1)
    int   m_28;          // +0x28  (= a2)
    int   m_2c;          // +0x2c  (= 0)
    int   m_30;          // +0x30  (= 0)
    char  m_pad34[4];
    void *m_arialFont;   // +0x38  (HFONT, the ARIAL UI font)
    void *m_trainingFont;// +0x3c  (HFONT, the TrainingFont)
    void *m_messageFont; // +0x40  (HFONT, the MessageFont)
};

// ---------------------------------------------------------------------------
// CFontConfig::LoadFontConfig
RVA(0x218e0, 0x1ff)
int CFontConfig::LoadFontConfig(int a1, int a2)
{
    m_24 = a1;
    m_28 = a2;
    m_20 = 0;
    m_2c = 0;
    m_30 = 0;

    // --- ARIAL UI font (fixed 12x8 bold ANSI) -------------------------------
    m_arialFont = CreateFontA(0xc, 8, 0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, s_ARIAL);
    if (!m_arialFont)
        m_arialFont = CreateFontA(0xc, 8, 0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, s_ARIAL);

    // The ARIAL default-face CString temp - constructed here (just-in-time, after
    // the first CreateFontA), passed by address as the GetStringDef default for
    // both the Training and Message font face lookups; torn down at the tail.
    CString arial(s_ARIAL);

    // --- TrainingFont (face/dims from config, default ARIAL / 14x28) --------
    char *faceTF = g_bute->GetStringDef(s_Font, s_TrainingFont, (CString *)&arial)->m_pchData;
    m_trainingFont = CreateFontA(
        g_bute->GetIntDef(s_Font, s_TrainingFontHeight, 0x1c),
        g_bute->GetIntDef(s_Font, s_TrainingFontWidth, 0xe),
        0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, faceTF);
    if (!m_trainingFont)
        m_trainingFont = CreateFontA(
            g_bute->GetIntDef(s_Font, s_TrainingFontHeight, 0x18),
            g_bute->GetIntDef(s_Font, s_TrainingFontWidth, 0x10),
            0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, 0);

    // --- MessageFont (face/dims from config, default ARIAL / 24x42) ---------
    char *faceMF = g_bute->GetStringDef(s_Font, s_MessageFont, (CString *)&arial)->m_pchData;
    m_messageFont = CreateFontA(
        g_bute->GetIntDef(s_Font, s_MessageFontHeight, 0x2a),
        g_bute->GetIntDef(s_Font, s_MessageFontWidth, 0x18),
        0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, faceMF);
    if (!m_messageFont)
        m_messageFont = CreateFontA(
            g_bute->GetIntDef(s_Font, s_MessageFontHeight, 0x2a),
            g_bute->GetIntDef(s_Font, s_MessageFontWidth, 0x18),
            0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, 0);

    return 1;
}
