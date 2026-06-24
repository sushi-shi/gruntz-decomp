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
//     operator delete), then RemoveAll + Empty(m_1c) + m_30=0.
//   Reset          - FreeNodes + Empty(m_1c) + DeleteObject the three HFONTs.
//   AddItem        - new FontItem, fill name/type/data, AddHead or AddTail
//     (head when type&2); optionally clears the list first (when type&4).
//   Scroll         - advance the running offset by a delta; when it crosses the
//     active threshold, RemoveHead the oldest FontItem and reset the offset.
//   ~CFontConfig   - Reset + ~CString(m_1c) + the CPtrList base dtor.
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
    int type;     // +0x00
    int data;     // +0x04
    CString name; // +0x08
};

// ---------------------------------------------------------------------------
// CFontConfig - the GDI font-configuration object (a CPtrList of FontItem*).
// Only the load-bearing member offsets are reconstructed.
// ---------------------------------------------------------------------------
class CFontConfig : public CPtrList {
public:
    int LoadFontConfig(int a1, int a2);
    void FreeNodes();
    void Reset();
    int AddItem(const char* str, int type, int data);
    void Scroll(int delta);
    ~CFontConfig();
    int winapi_022360_DrawTextA_SelectObject_SetTextColor(int, int, int, int);

    CString m_1c;         // +0x1c  scratch string
    unsigned int m_20;    // +0x20  running offset (unsigned: thresholds compare jb)
    unsigned int m_24;    // +0x24  (= a1) low threshold
    unsigned int m_28;    // +0x28  (= a2) high threshold
    int m_2c;             // +0x2c  accumulator
    int m_30;             // +0x30  accumulate flag
    char m_pad34[4];      // +0x34
    HFONT m_arialFont;    // +0x38  the ARIAL UI font
    HFONT m_trainingFont; // +0x3c  the TrainingFont
    HFONT m_messageFont;  // +0x40  the MessageFont
};

// ---------------------------------------------------------------------------
// CFontConfig::LoadFontConfig
RVA(0x000218e0, 0x1ff)
int CFontConfig::LoadFontConfig(int a1, int a2) {
    m_24 = a1;
    m_28 = a2;
    m_20 = 0;
    m_2c = 0;
    m_30 = 0;

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
    m_1c.Empty();
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
    m_1c.Empty();
    m_30 = 0;
}

// ---------------------------------------------------------------------------
// CFontConfig::AddItem - append/prepend a new FontItem; optionally clear first.
RVA(0x00021c60, 0xde)
int CFontConfig::AddItem(const char* str, int type, int data) {
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
// the offset crosses the active threshold (m_28 above 3 items, else m_24).
RVA(0x00021d80, 0x79)
void CFontConfig::Scroll(int delta) {
    if (m_30) {
        m_2c += delta;
    }
    int count = m_nCount;
    if (!count) {
        m_20 = 0;
    }
    m_20 += delta;

    FontItem* item;
    if (count > 3) {
        if (m_20 < m_28) {
            return;
        }
        item = (FontItem*)RemoveHead();
        if (!item) {
            return;
        }
    } else {
        if (m_20 < m_24) {
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
    m_20 = 0;
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
int CFontConfig::winapi_022360_DrawTextA_SelectObject_SetTextColor(int, int, int, int) {
    return 0;
}
