// FontConfig.cpp - the font/dialog-text TU (C:\Proj\Gruntz), interval
// 0x0218e0-0x022a3a (+ the out-of-band dtor stray). ONE original TU per
// docs/exe-map/interval-dossiers.md #3: our fontconfig + drawtext units were
// slices of this single file - the drawtext hosts ARE CFontConfig by member-
// layout identity (m4::PwdHost +0x1c edit-text CString + +0x38 control HFONT ==
// CFontConfig::m_inputText/m_arialFont), the renderers render the very string
// TypeChar accumulates, and the single fontconfig init-frag run @0x21610
// immediately precedes the interval.
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
// <Mfc.h> brings <windows.h> (GDI32: CreateFontA / DeleteObject; USER32
// DrawTextA) and the MFC CPtrList / CString collection types from <afxcoll.h>.
#include <Mfc.h>
#include <Gruntz/FontConfig.h>
#include <rva.h>

#include <Bute/ButeMgr.h>
#include <string.h> // strlen (the m4 draw helpers)

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
    ~FontItem();  // 0x21c40  out-of-line member dtor (destroys name; add ecx,8; jmp ~CString)
};

// ---------------------------------------------------------------------------
// The m4 draw-helper hosts (the former drawtext unit). @identity-TODO: the
// interval dossier proves by member-layout identity that these hosts ARE
// CFontConfig (PwdHost::m_1c/+0x38 == CFontConfig::m_inputText/m_arialFont;
// TextHost::m_3c/m_40 == m_trainingFont/m_messageFont; DrawHost's +0x1c
// sub-object is the same edit-text source) - the views are kept this package
// (no class renames per the wave1-D brief) and fold onto CFontConfig in a
// follow-up.
// ---------------------------------------------------------------------------

// ImgHolder2::Release1c6a5c @0x1c6a5c IS MFC CImageList::DeleteImageList; minimal local decl.
SIZE_UNKNOWN(CImageList);
class CImageList {
public:
    void DeleteImageList();
};

namespace m4 {

    // Cached measured text width (0x0062b434), read by the caller after measure.
    extern i32 g_62b434;

    // DrawTextA through the game Win32 pointer table (RVA 0x2c454c) -> reloc-masked.
    DATA(0x002c454c)
    extern int(WINAPI* g_pDrawTextA)(HDC, LPCSTR, int, LPRECT, UINT); // 0x2c454c

    // The image-worker/imgHolder scratch (see m4_FlashRect): inline dtor chain, but
    // this call site builds it with a 3-arg out-of-line ctor.
    struct SevWorker2 {
        virtual ~SevWorker2() {}
    };
    struct ImgHolder2 : SevWorker2 {
        // Release1c6a5c @0x1c6a5c IS CImageList::DeleteImageList; cast in the dtor.
        virtual ~ImgHolder2() OVERRIDE {
            ((CImageList*)this)->DeleteImageList();
        }
    };
    struct DrawScratch : ImgHolder2 {
        DrawScratch(i32 a, i32 b, i32 c); // 0x001c6a72
    };

    // The item's rect source: built from the host's +0x1c sub-object; its first
    // field points at the item RECT. Destructible (EH-tracked).
    struct RectSrc {
        RECT* m_rect;             // +0x00
        RectSrc(void* rectField); // 0x001b9ba3
        ~RectSrc();               // 0x001b9cde
    };

    // The engine text renderer bound to the DC (returned by Bind1c56ef).
    struct TextRenderer {
        i32 Push1c58ea(void* obj);                  // 0x001c58ea (save/select, returns prior)
        void MoveTo1c6059(void* out, i32 x, i32 y); // 0x001c6059
        void Draw1c60a5(i32 x, i32 y);              // 0x001c60a5
    };
    TextRenderer* Bind1c56ef(HDC hdc); // 0x001c56ef

    // The dialog host; the item rect sub-object lives at +0x1c. @identity-TODO
    // (CFontConfig facet, see the block note above).
    struct DrawHost {
        char m_pad00[0x1c];
        void* m_1c;                                       // +0x1c  item-rect CString source
        i32 MeasureLabel21f20(HDC hdc, const char* text); // 0x00021f20
    };

    // The game's Win32 pointer table entries (0x6c44xx/0x6c3exx) -> reloc-masked.
    extern SHORT(WINAPI* g_pGetAsyncKeyState)(int); // 0x006c4500
    DATA(0x002c3ec4)
    extern HGDIOBJ(WINAPI* g_pSelectObject)(HDC, HGDIOBJ); // 0x2c3ec4

    // Password blink timer + last-format cache (reached by address).
    extern i32 g_645584; // 0x00645584 elapsed-time delta
    extern i32 g_62b438; // 0x0062b438 blink countdown
    extern i32 g_62b43c; // 0x0062b43c blink on/off state
    extern i32 g_60c7a8; // 0x0060c7a8 last DrawText format

    // The control's MFC CString (data ptr at +0, length at [data-8]); copy ctor +
    // SetAt + dtor are out-of-line (other TU) -> reloc-masked.
    struct PwdStr {
        char* m_data;
        PwdStr(void* srcCString);   // 0x001b9ba3 (CString copy ctor)
        ~PwdStr();                  // 0x001b9cde
        void SetAt(i32 i, char ch); // 0x001ba282
        i32 Len() {
            return *((i32*)m_data - 2);
        }
    };

    // @identity-TODO: PwdHost IS CFontConfig (m_1c == m_inputText's data ptr slot,
    // m_38 == m_arialFont) per the dossier's layout proof.
    struct PwdHost {
        char m_pad00[0x1c];
        char* m_1c; // +0x1c CString data ptr (edit text)
        char m_pad20[0x38 - 0x20];
        HGDIOBJ m_38;                       // +0x38 control font
        void Draw258b(HDC hdc, RECT* rect); // 0x0000258b (caret/underline draw)
        i32 Render22160(HDC hdc, i32 maxWidth, RECT* rect);
        i32 DrawWithFont22770(const char* text, HDC hdc, RECT* rect, UINT format); // 0x22770
    };

    extern int(WINAPI* g_pSetBkMode)(HDC, int);              // 0x006c3eb8
    extern COLORREF(WINAPI* g_pSetBkColor)(HDC, COLORREF);   // 0x006c3eb0
    extern COLORREF(WINAPI* g_pSetTextColor)(HDC, COLORREF); // 0x006c3eb4

    // @identity-TODO: TextHost IS CFontConfig (m_3c/m_40 == m_trainingFont/
    // m_messageFont) per the dossier's layout proof.
    struct TextHost {
        char m_pad00[0x3c];
        HGDIOBJ m_3c; // +0x3c font A
        HGDIOBJ m_40; // +0x40 font B
        i32 Draw3DText22810(
            void* strSrc,
            HDC hdc,
            RECT* dst,
            i32 fontFlag,
            i32 r,
            i32 g,
            i32 b,
            i32 shadow,
            i32 dx,
            i32 dy
        );
    };

} // namespace m4

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
// FontItem::~FontItem (0x21c40) - the out-of-line record dtor: destroy the +8
// CString name member (add ecx,8; jmp ~CString). Referenced by FreeNodes/AddItem's
// `delete item` and Scroll's explicit `item->FontItem::~FontItem()`. Re-homed from
// src/Stub/DiscoveredSmall.cpp (was the CU35Host::DestroyStr view).
// ---------------------------------------------------------------------------
RVA(0x00021c40, 0x8)
FontItem::~FontItem() {}

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
// m4::DrawHost::MeasureLabel21f20 - measure a CString label into the item's rect
// (DrawTextA with DT_CALCRECT-ish flags 0x420), clamp the used width into
// g_62b434, then run the engine text renderer at that origin. The scratch draw
// object is the same three-level image-worker/imgHolder hierarchy as m4_FlashRect
// (vtables in other TUs -> reloc-masked); its dtor chain inlines but its 3-arg
// ctor is out-of-line.
// @early-stop
// regalloc + EH-state wall (~72%). Complete correct reconstruction: the /GX
// frame, the empty-CString (*(text-8)==0) gate, the rect copy + DrawTextA
// measure, the min(provW,textW) clamp into g_62b434, and the Bind/Push/MoveTo/
// Draw render with the inline image-worker/imgHolder scratch dtor chain all align
// by shape (llvm-objdump -dr). Residual is MSVC5 using a 4th callee-saved reg
// (ebp) where retail packs into ebx/esi/edi (so the arg + local offsets shift)
// plus the EH scope-table addend (0 vs 8) - not steerable from source.
RVA(0x00021f20, 0x162)
i32 m4::DrawHost::MeasureLabel21f20(HDC hdc, const char* text) {
    if (hdc == 0) {
        return 0;
    }
    RectSrc src(&m_1c);
    RECT* rp = src.m_rect;
    if (*((i32*)text - 2) == 0) {
        g_62b434 = 0;
    } else {
        RECT rc;
        rc.left = rp->left;
        rc.top = rp->top;
        rc.right = rp->right;
        rc.bottom = rp->bottom;
        g_pDrawTextA(hdc, text, *((i32*)text - 2), &rc, 0x420);
        i32 textW = rc.right - rc.left;
        i32 provW = rp->right - rp->left;
        g_62b434 = provW;
        if (provW >= textW) {
            g_62b434 = textW;
        }
    }
    TextRenderer* tr = Bind1c56ef(hdc);
    if (tr != 0) {
        DrawScratch scratch(0, 2, 0);
        i32 pen;
        i32 saved = tr->Push1c58ea(&pen);
        i32 origin;
        tr->MoveTo1c6059(&origin, rp->left + g_62b434, rp->top);
        tr->Draw1c60a5(rp->left + g_62b434, rp->top + 0xc);
        tr->Push1c58ea((void*)saved);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// m4::PwdHost::Render22160 - the password edit-control render path. Copies the
// control's CString (this->m_1c), and when Ctrl is held, masks every char with
// '*'; runs a blink countdown (g_62b438) toggling g_62b43c; then (unless blinked-
// off + empty) selects the control font, DrawTextA-measures the masked text,
// right-aligns it if it overflows maxWidth, and renders it into the rect.
// thiscall member, /GX (destructible CString). Placeholder names.
// @early-stop
// regalloc/EH-state wall (~sibling of MeasureLabel21f20). Complete correct
// reconstruction: the /GX frame, the arg-null gate before the CString copy, the
// Ctrl-held '*'-mask loop, the g_645584/g_62b438 countdown + g_62b43c toggle,
// the blink-off-empty caret branch, the font SelectObject save/restore, the
// DT_CALCRECT measure + overflow right-align, and both DrawTextA renders align
// by shape (llvm-objdump -dr). Residual is MSVC5 pinning the shared zero in edi
// + reusing dead arg slots for the CString/RECT locals differently, shifting the
// [esp+N] operands and EH scope addend - not steerable from source.
RVA(0x00022160, 0x18e)
i32 m4::PwdHost::Render22160(HDC hdc, i32 maxWidth, RECT* rect) {
    if (hdc == 0) {
        return 0;
    }
    PwdStr text(&m_1c);
    if (g_pGetAsyncKeyState(0x11) & 0x8000) {
        for (i32 i = 0; i < text.Len(); i++) {
            text.SetAt(i, '*');
        }
    }
    i32 t;
    if ((u32)g_645584 < (u32)g_62b438) {
        t = g_62b438 - g_645584;
    } else {
        t = 0;
    }
    g_62b438 = t;
    if (t == 0) {
        g_62b438 = 0xc8;
        g_62b43c ^= 1;
    }
    if (g_62b43c != 0 && text.Len() == 0) {
        Draw258b(hdc, rect);
    } else {
        HGDIOBJ prev = 0;
        if (m_38) {
            prev = g_pSelectObject(hdc, m_38);
        }
        if (g_62b43c) {
            Draw258b(hdc, rect);
        }
        int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = g_pDrawTextA;
        RECT rc;
        rc.left = rect->left;
        rc.top = rect->top;
        rc.right = rect->right;
        rc.bottom = rect->bottom;
        pDraw(hdc, text.m_data, text.Len(), &rc, 0x420);
        i32 fmt = ((rc.right - rc.left) <= maxWidth) ? 0x20 : 0x22;
        g_60c7a8 = fmt;
        pDraw(hdc, text.m_data, text.Len(), rect, fmt);
        if (prev) {
            g_pSelectObject(hdc, prev);
        }
    }
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stub (owned by this class here).
// -------------------------------------------------------------------------
// @confidence: low
// @source: winapi:DrawTextA;SelectObject;SetTextColor
// @stub
RVA(0x00022360, 0x2f4)
i32 CFontConfig::winapi_022360_DrawTextA_SelectObject_SetTextColor(i32, i32, i32, i32) {
    return 0;
}

// -------------------------------------------------------------------------
// m4::PwdHost::DrawWithFont22770 (0x22770): draw a plain C string with the control
// font. Null-guard hdc/text/rect, select m_38 (saving the prior), DrawTextA the
// strlen(text) into rect with the caller's format, then restore the font. ret 1.
RVA(0x00022770, 0x7d)
i32 m4::PwdHost::DrawWithFont22770(const char* text, HDC hdc, RECT* rect, UINT format) {
    if (hdc == 0) {
        return 0;
    }
    if (text == 0) {
        return 0;
    }
    if (rect == 0) {
        return 0;
    }
    HGDIOBJ prev = 0;
    if (m_38) {
        prev = g_pSelectObject(hdc, m_38);
    }
    g_pDrawTextA(hdc, text, strlen(text), rect, format);
    if (prev) {
        g_pSelectObject(hdc, prev);
    }
    return 1;
}

// -------------------------------------------------------------------------
// m4::TextHost::Draw3DText22810 - a centered "3D" text renderer. Selects one of
// two control fonts, sets transparent bk, copies a source CString, DT_CALCRECT-
// measures it centered in the dst rect, and draws it centered - first a black
// shadow pass offset by (dx,dy) when the shadow flag is set, then the RGB(r,g,b)
// main pass. thiscall member, 10 args, /GX (destructible CString).
// @early-stop
// regalloc/scheduling wall. Complete correct reconstruction: the /GX frame, the
// three arg-null gates before the CString copy, the two-font SelectObject, the
// transparent-bk setup, the DT_CALCRECT centering math (signed /2 round-toward-
// zero on both axes), the black-shadow offset pass and the RGB main pass all
// align by shape (llvm-objdump -dr). Residual is MSVC5 permuting the rc/centering
// temporaries across ebx/ebp/esi/edi vs retail and reusing dead arg slots for the
// rc + selPrev locals differently, shifting the [esp+N] operands - not steerable.
RVA(0x00022810, 0x22a)
i32 m4::TextHost::Draw3DText22810(
    void* strSrc,
    HDC hdc,
    RECT* dst,
    i32 fontFlag,
    i32 r,
    i32 g,
    i32 b,
    i32 shadow,
    i32 dx,
    i32 dy
) {
    if (hdc == 0) {
        return 0;
    }
    if (dst == 0) {
        return 0;
    }
    if (strSrc == 0) {
        return 0;
    }
    HGDIOBJ selPrev = 0;
    RECT rc;
    rc.left = dst->left;
    rc.top = dst->top;
    rc.right = dst->right;
    rc.bottom = dst->bottom;
    HGDIOBJ obj = fontFlag ? m_40 : m_3c;
    if (obj) {
        selPrev = g_pSelectObject(hdc, obj);
    }
    g_pSetBkMode(hdc, 1);
    g_pSetBkColor(hdc, 0);
    PwdStr text(strSrc);
    g_pDrawTextA(hdc, text.m_data, strlen(text.m_data), &rc, 0x411);
    i32 hoff = (dst->right + rc.left - dst->left - rc.right) / 2;
    i32 voff = (dst->bottom - dst->top + rc.top - rc.bottom) / 2;
    rc.left += hoff;
    rc.right += hoff;
    rc.top += voff;
    rc.bottom += voff;
    if (shadow) {
        g_pSetTextColor(hdc, 0);
        rc.left += dx;
        rc.top += dy;
        rc.right += dx;
        rc.bottom += dy;
        g_pDrawTextA(hdc, text.m_data, strlen(text.m_data), &rc, 0x11);
        rc.right -= dx;
        rc.left -= dx;
        rc.bottom -= dy;
        rc.top -= dy;
    }
    g_pSetTextColor(hdc, RGB(r, g, b));
    g_pDrawTextA(hdc, text.m_data, strlen(text.m_data), &rc, 0x11);
    if (selPrev) {
        g_pSelectObject(hdc, selPrev);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFontConfig::~CFontConfig (0x085f40, out-of-band stray) - Reset, then the
// CString member + the CPtrList base dtor run by the compiler-emitted cleanup
// (the EH frame comes from the base).
// @early-stop
// EH/vptr wall: our polymorphic model emits one extra `mov [esi],&??_7CFontConfig`
// most-derived vptr re-stamp that retail elided; /GX frame + base-dtor exact.
// Not steerable by source spelling - docs/patterns/eh-dtor-vptr-restamp-presence.md.
RVA(0x00085f40, 0x56)
CFontConfig::~CFontConfig() {
    Reset();
}

SIZE_UNKNOWN(FontItem);
