// FontConfig.cpp - CFontConfig, the GDI font-configuration object.
//
// CFontConfig is a CPtrList-derived container of font-config "items"
// (FontItem records, each {int type; int data; CString name;}) plus a CString
// scratch member (+0x1c), five scroll/threshold ints (+0x20..+0x30), and the
// three cached GDI HFONTs (+0x38/+0x3c/+0x40) built by LoadFontConfig.
//
//   LoadFontConfig - builds three GDI HFONTs (the ARIAL UI font fixed at 12x8
//     bold; the TrainingFont; the MessageFont) via CreateFontA, dims/faces read
//     from the global CButeMgr "Font" config group (butemgr getters).
//   FreeNodes      - walks the list freeing each FontItem (its CString @+8 +
//     operator delete), then RemoveAll + Empty(m_inputText) + m_inputActive=0.
//   Reset          - FreeNodes + Empty(m_inputText) + DeleteObject the three HFONTs.
//   AddItem        - new FontItem, fill name/type/data, AddHead or AddTail
//     (head when type&2); optionally clears the list first (when type&4).
//   Scroll         - advance the running offset by a delta; when it crosses the
//     active threshold, RemoveHead the oldest FontItem and reset the offset.
//   ~CFontConfig   - Reset + ~CString(m_inputText) + the CPtrList base dtor.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). The CPtrList base (sizeof 0x1c) supplies
// m_pNodeHead@+0x04 / m_nCount@+0x0c and the RemoveAll/RemoveHead/AddHead/
// AddTail out-of-line NAFXCW calls; the base subobject's non-trivial dtor is
// what forces the /GX EH frame on AddItem and ~CFontConfig.
// ---------------------------------------------------------------------------
// <Mfc.h> brings <windows.h> (GDI32: CreateFontA / DeleteObject) and the MFC
// CPtrList / CString collection types from <afxcoll.h>.
#include <Mfc.h>
#include <rva.h>

#include <Bute/ButeMgr.h>

// The global empty C string the input-reset assigns into m_inputText (0x6293f4).
extern "C" char g_emptyString[];

// The global CButeMgr instance (the ctor stores the bute config tree here).
// Declared as a named extern so the `mov ecx, offset g_buteMgr` loads
// reloc-match the engine; @address names the delinked target DATA symbol.
DATA(0x002453d8)
extern CButeMgr g_buteMgr;
#define g_bute (&g_buteMgr)

// The font config strings - the original source literals (the "Font" tag-group
// + per-font keys). objdiff matches these relocations by value against the
// target's .data string constants.
#define s_Font "Font"
#define s_ARIAL "ARIAL"
#define s_TrainingFont "TrainingFont"
#define s_TrainingFontWidth "TrainingFontWidth"
#define s_TrainingFontHeight "TrainingFontHeight"
#define s_MessageFont "MessageFont"
#define s_MessageFontWidth "MessageFontWidth"
#define s_MessageFontHeight "MessageFontHeight"

// ---------------------------------------------------------------------------
// A single font-config record stored in the list (0xc bytes). `name` is the
// face/text string; `type`/`data` are the packed flags+payload. The list owns
// these (FreeNodes/Scroll delete them).
// ---------------------------------------------------------------------------
struct FontItem {
    i32 type;     // +0x00
    i32 data;     // +0x04
    CString name; // +0x08
};

// ---------------------------------------------------------------------------
// CFontConfig - the GDI font-configuration object (a CPtrList of FontItem*).
// Only the load-bearing member offsets are reconstructed.
// ---------------------------------------------------------------------------
class CFontConfig : public CPtrList {
public:
    i32 LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold);
    void FreeNodes();
    void Reset();
    i32 AddItem(const char* str, i32 type, i32 data);
    void Scroll(i32 delta);
    i32 TypeChar(i32 ch, i32 a2);
    void EndInput();
    ~CFontConfig();
    i32 winapi_022360_DrawTextA_SelectObject_SetTextColor(i32, i32, i32, i32);

    CString m_inputText;       // +0x1c  scratch input string
    u32 m_scrollOffset;        // +0x20  running offset (unsigned: thresholds compare jb)
    u32 m_lowScrollThreshold;  // +0x24  threshold used for <=3 items
    u32 m_highScrollThreshold; // +0x28  threshold used for >3 items
    i32 m_inputScrollTotal;    // +0x2c  accumulated scroll while input is active
    i32 m_inputActive;         // +0x30  input accumulation flag
    char m_pad34[4];           // +0x34
    HFONT m_arialFont;         // +0x38  the ARIAL UI font
    HFONT m_trainingFont;      // +0x3c  the TrainingFont
    HFONT m_messageFont;       // +0x40  the MessageFont
};

// ---------------------------------------------------------------------------
// CFontConfig::LoadFontConfig
RVA(0x000218e0, 0x1ff)
i32 CFontConfig::LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold) {
    m_lowScrollThreshold = lowScrollThreshold;
    m_highScrollThreshold = highScrollThreshold;
    m_scrollOffset = 0;
    m_inputScrollTotal = 0;
    m_inputActive = 0;

    // --- ARIAL UI font (fixed 12x8 bold ANSI) -------------------------------
    m_arialFont = CreateFontA(0xc, 8, 0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, s_ARIAL);
    if (!m_arialFont) {
        m_arialFont = CreateFontA(0xc, 8, 0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, s_ARIAL);
    }

    // The ARIAL default-face CString temp - constructed here (just-in-time, after
    // the first CreateFontA), passed by address as the GetStringDef default for
    // both the Training and Message font face lookups; torn down at the tail.
    CString arial(s_ARIAL);

    // --- TrainingFont (face/dims from config, default ARIAL / 14x28) --------
    const char* faceTF =
        (const char*)*g_bute->GetStringDef(s_Font, s_TrainingFont, (CString*)&arial);
    m_trainingFont = CreateFontA(
        g_bute->GetIntDef(s_Font, s_TrainingFontHeight, 0x1c),
        g_bute->GetIntDef(s_Font, s_TrainingFontWidth, 0xe),
        0,
        0,
        0x2bc,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        faceTF
    );
    if (!m_trainingFont) {
        m_trainingFont = CreateFontA(
            g_bute->GetIntDef(s_Font, s_TrainingFontHeight, 0x18),
            g_bute->GetIntDef(s_Font, s_TrainingFontWidth, 0x10),
            0,
            0,
            0x2bc,
            0,
            0,
            0,
            1,
            0,
            0,
            0,
            0,
            0
        );
    }

    // --- MessageFont (face/dims from config, default ARIAL / 24x42) ---------
    const char* faceMF =
        (const char*)*g_bute->GetStringDef(s_Font, s_MessageFont, (CString*)&arial);
    m_messageFont = CreateFontA(
        g_bute->GetIntDef(s_Font, s_MessageFontHeight, 0x2a),
        g_bute->GetIntDef(s_Font, s_MessageFontWidth, 0x18),
        0,
        0,
        0x2bc,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        faceMF
    );
    if (!m_messageFont) {
        m_messageFont = CreateFontA(
            g_bute->GetIntDef(s_Font, s_MessageFontHeight, 0x2a),
            g_bute->GetIntDef(s_Font, s_MessageFontWidth, 0x18),
            0,
            0,
            0x2bc,
            0,
            0,
            0,
            1,
            0,
            0,
            0,
            0,
            0
        );
    }

    return 1;
}

// ---------------------------------------------------------------------------
// CFontConfig::Reset - tear down the list + scratch string + the three HFONTs.
RVA(0x00021b60, 0x4d)
void CFontConfig::Reset() {
    FreeNodes();
    m_inputText.Empty();
    if (m_arialFont) {
        DeleteObject(m_arialFont);
        m_arialFont = 0;
    }
    if (m_trainingFont) {
        DeleteObject(m_trainingFont);
        m_trainingFont = 0;
    }
    if (m_messageFont) {
        DeleteObject(m_messageFont);
        m_messageFont = 0;
    }
}

// ---------------------------------------------------------------------------
// CFontConfig::FreeNodes - delete every FontItem in the list, then clear it.
RVA(0x00021bd0, 0x45)
void CFontConfig::FreeNodes() {
    CPtrList::CNode* p = m_pNodeHead;
    while (p) {
        CPtrList::CNode* cur = p;
        p = p->pNext;
        FontItem* item = (FontItem*)cur->data;
        if (item) {
            delete item;
        }
    }
    RemoveAll();
    m_inputText.Empty();
    m_inputActive = 0;
}

// ---------------------------------------------------------------------------
// CFontConfig::AddItem - append/prepend a new FontItem; optionally clear first.
RVA(0x00021c60, 0xde)
i32 CFontConfig::AddItem(const char* str, i32 type, i32 data) {
    if (!str) {
        return 0;
    }
    if (!*str) {
        return 0;
    }
    if (type & 4) {
        CPtrList::CNode* p = m_pNodeHead;
        while (p) {
            CPtrList::CNode* cur = p;
            p = p->pNext;
            FontItem* item = (FontItem*)cur->data;
            if (item) {
                delete item;
            }
        }
        RemoveAll();
    }
    FontItem* item = new FontItem;
    item->name = str;
    item->type = type;
    item->data = data;
    if (type & 2) {
        AddHead(item);
    } else {
        AddTail(item);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFontConfig::Scroll - advance the running offset; drop the head FontItem when
// the offset crosses the active threshold (m_highScrollThreshold above 3 items,
// else m_lowScrollThreshold).
RVA(0x00021d80, 0x79)
void CFontConfig::Scroll(i32 delta) {
    if (m_inputActive) {
        m_inputScrollTotal += delta;
    }
    i32 count = m_nCount;
    if (!count) {
        m_scrollOffset = 0;
    }
    m_scrollOffset += delta;

    FontItem* item;
    if (count > 3) {
        if (m_scrollOffset < m_highScrollThreshold) {
            return;
        }
        item = (FontItem*)RemoveHead();
        if (!item) {
            return;
        }
    } else {
        if (m_scrollOffset < m_lowScrollThreshold) {
            return;
        }
        if (!count) {
            return;
        }
        item = (FontItem*)RemoveHead();
        if (!item) {
            return;
        }
    }
    item->name.Empty();
    item->FontItem::~FontItem();
    ::operator delete(item);
    m_scrollOffset = 0;
}

// ---------------------------------------------------------------------------
// CFontConfig::TypeChar - the typed-character accumulator into the scratch
// string m_inputText. Enter (0xd) toggles the accumulating flag m_inputActive: first
// press arms it (reset offset/accumulator + clear m_inputText); a second press while the
// buffer is non-empty disarms and returns 1 (commit). While armed, backspace
// (8) trims one char, and a printable byte (0x20..0xff) appends if under 0x50.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): logic +
// control flow byte-identical, but retail pins `ch` in ebx (loaded early between
// the prologue pushes) and the 0 constant in edi, while our cl swaps them (ch in
// edi, 0 in ebx). A 1-instr phase shift through every =0 store / compare; not
// source-steerable. Effectively matched.
RVA(0x00021e20, 0x95)
i32 CFontConfig::TypeChar(i32 ch, i32 a2) {
    m_inputScrollTotal = 0;
    if (ch == 0xd) {
        if (m_inputActive != 0) {
            if (m_inputText.GetLength() == 0) {
                return 0;
            }
            m_inputActive = 0;
            return 1;
        }
        m_inputActive = 1;
        m_scrollOffset = 0;
        m_inputScrollTotal = 0;
        m_inputText = (const char*)g_emptyString;
    }
    if (m_inputActive == 0) {
        return 0;
    }
    if (ch == 8) {
        i32 len = m_inputText.GetLength();
        if (len <= 0) {
            return 0;
        }
        m_inputText.GetBufferSetLength(len - 1);
        return 0;
    }
    if (ch < 0x20 || ch > 0xff) {
        return 0;
    }
    if (m_inputText.GetLength() < 0x50) {
        m_inputText += (char)ch;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CFontConfig::EndInput - cancel accumulation: clear the flag and empty m_inputText.
RVA(0x00021ef0, 0x17)
void CFontConfig::EndInput() {
    if (m_inputActive != 0) {
        m_inputActive = 0;
        m_inputText.Empty();
    }
}

// ---------------------------------------------------------------------------
// CFontConfig::~CFontConfig - Reset, then the CString member + the CPtrList base
// dtor run by the compiler-emitted cleanup (the EH frame comes from the base).
// @early-stop
// EH/vptr wall: our polymorphic model emits one extra `mov [esi],&??_7CFontConfig`
// most-derived vptr re-stamp that retail elided; /GX frame + base-dtor exact.
// Not steerable by source spelling - docs/patterns/eh-dtor-vptr-restamp-presence.md.
RVA(0x00085f40, 0x56)
CFontConfig::~CFontConfig() {
    Reset();
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
// -------------------------------------------------------------------------
// @confidence: low
// @source: winapi:DrawTextA;SelectObject;SetTextColor
// @stub
RVA(0x00022360, 0x2f4)
i32 CFontConfig::winapi_022360_DrawTextA_SelectObject_SetTextColor(i32, i32, i32, i32) {
    return 0;
}
