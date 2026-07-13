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
// Real MFC CDC / CPen / CGdiObject (the caret stroke below). Skip the afxwin*.inl
// bodies for the CLANG LABEL STEP only (implicit-int inlines clang rejects); wine cl
// keeps the inlines. docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
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
// THE THREE m4 DRAW-HELPER HOSTS ARE DISSOLVED (2026-07-13). DrawHost / PwdHost /
// TextHost were all CFontConfig - the interval dossier had ALREADY proven it by
// member-layout identity (their +0x1c is m_inputText, +0x38/+0x3c/+0x40 are the
// three cached HFONTs) and parked them as "@identity-TODO ... fold in a follow-up".
// Their four methods are now real CFontConfig methods (<Gruntz/FontConfig.h>).
//
// Two more views went with them:
//   PwdStr  -> ::CString. Its ctor/dtor/SetAt are the NAFXCW library routines
//              ??0CString@@QAE@ABV0@@Z (0x1b9ba3) / ??1CString@@QAE@XZ (0x1b9cde) /
//              ?SetAt@CString@@QAEXHD@Z (0x1ba282) - config/library_labels.csv, all
//              HIGH-confidence. It was never anything but an MFC CString.
//   RectSrc -> DELETED, it was a FICTION. Same ctor rva (the CString copy ctor) but
//              its "RECT* m_rect" re-read the copied string's data pointer as a rect.
//              See MeasureLabel's note: retail passes the RECT in as arg2.
//
// DISSOLVED (2026-07-13, Fable lane): SevWorker2 / ImgHolder2 / DrawScratch /
// TextRenderer / CImageList were hand-rolled stand-ins for the REAL MFC GDI classes
// (<afxwin.h>, statically-linked NAFXCW): the caret scratch pen IS a stack ::CPen
// (ctor 0x1c6a72 == ??0CPen@@QAE@HHK@Z, HIGH FID; its destruction is the standard
// MSVC inlined ~CPen chain - vptr restamps to ??_7CGdiObject @0x1e8cd4 then
// ??_7CObject @0x1e8cb4, both already in config/vtable_names.csv, around a direct
// call to 0x1c6a5c == ?DeleteObject@CGdiObject@@QAEHXZ, whose body is the exact
// `if (!m_hObject) return 0; ::DeleteObject(Detach())` shape - the old
// "CImageList::DeleteImageList" FID row was the AMBIG GDI/ImageList twin, as
// Dialogs.cpp / GameMode.h already concluded). The device context IS a ::CDC:
// 0x1c56ef == ?FromHandle@CDC@@SGPAV1@PAUHDC__@@@Z (body walks the HDC handle map),
// 0x1c58ea == ?SelectObject@CDC@@QAEPAVCPen@@PAV2@@Z (HIGH FID), 0x1c6059 ==
// ?MoveTo@CDC@@QAE?AVCPoint@@HH@Z (body: MoveToEx on both m_hDC/m_hAttribDC
// returning the old CPoint through the hidden slot; the old "SetViewportOrg" row
// was the AMBIG twin - SetViewportOrg would call SetViewportOrgEx), and 0x1c60a5 ==
// ?LineTo@CDC@@QAEHHH@Z (the MFC shape: MoveToEx(m_hAttribDC,..,NULL) then
// ::LineTo(m_hDC,..)). All five real names verified present in NAFXCWD.LIB; the
// three wrong library_labels.csv rows are corrected in the same change. The fake
// SevWorker2<-ImgHolder2<-DrawScratch virtual-dtor hierarchy (a fabricated vtable
// chain) is deleted outright - MFC's real CObject -> CGdiObject -> CPen supplies
// the true one.
// ---------------------------------------------------------------------------

namespace m4 {

    // Cached measured text width (0x0062b434), read by the caller after measure.
    extern i32 g_62b434;

    // DrawTextA through the game Win32 pointer table (RVA 0x2c454c) -> reloc-masked.

    // The game's Win32 pointer table entries (0x6c44xx/0x6c3exx) -> reloc-masked.

    // Password blink timer + last-format cache (reached by address).
    extern "C" i32 g_645584; // 0x00645584 elapsed-time delta
    extern i32 g_62b438;     // 0x0062b438 blink countdown
    extern i32 g_62b43c;     // 0x0062b43c blink on/off state
    extern i32 g_60c7a8;     // 0x0060c7a8 last DrawText format

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
// CFontConfig::MeasureLabel - measure m_inputText into the caller's rect
// (DrawTextA, DT_CALCRECT|DT_SINGLELINE flags 0x420), clamp the used width into
// g_62b434, then stroke the 12px insertion caret at that offset with a 2px pen.
//
// THE ARGS WERE SWAPPED (fixed 2026-07-13). The old m4::DrawHost view declared this
// `(HDC, const char* text)` and manufactured the rect out of the copied CString via
// a fake `RectSrc`. Retail (0x21f20) does the opposite: `add ecx,0x1c` + the CString
// COPY CTOR (0x1b9ba3) makes a temp of THIS->m_inputText - that is the text, whose
// length it reads at [eax-8] and pushes to DrawTextA - while arg2 (esi) is the RECT*
// it reads left/top/right/bottom from at [ecx]..[ecx+0xc]. The old "~72% regalloc
// wall" was this wrong shape, not regalloc: fixing the swap took it 72.46 -> 85.98.
// @early-stop
// The caret tail is now the REAL MFC CDC/CPen (views dissolved 2026-07-13, Fable
// lane; 85.98 -> 86.86). The residual is the DT_CALCRECT measure block's RECT-copy
// + argument scheduling (retail interleaves `push 0x420` with the four field copies
// and walks them through one register; cl copies then pushes - same instruction
// multiset, /O2 scheduling) - a codegen residual for the final sweep, no view left.
RVA(0x00021f20, 0x162)
i32 CFontConfig::MeasureLabel(HDC hdc, RECT* rect) {
    if (hdc == 0) {
        return 0;
    }
    CString text(m_inputText);
    if (text.GetLength() == 0) {
        m4::g_62b434 = 0;
    } else {
        RECT rc;
        rc.left = rect->left;
        rc.top = rect->top;
        rc.right = rect->right;
        rc.bottom = rect->bottom;
        ::DrawTextA(hdc, text, text.GetLength(), &rc, 0x420);
        i32 textW = rc.right - rc.left;
        i32 provW = rect->right - rect->left;
        m4::g_62b434 = provW;
        if (provW >= textW) {
            m4::g_62b434 = textW;
        }
    }
    // Stroke the 12px insertion caret with the real MFC GDI objects: a stack CPen
    // (PS_SOLID, width 2, black) selected into the CDC, MoveTo/LineTo, restore.
    // The pen's scope-end destruction inlines the ~CPen chain (vptr restamps to
    // ??_7CGdiObject/??_7CObject around CGdiObject::DeleteObject @0x1c6a5c),
    // exactly as retail's 0x21fda..0x22040 tail does.
    CDC* dc = CDC::FromHandle(hdc);
    if (dc != 0) {
        CPen pen(PS_SOLID, 2, RGB(0, 0, 0));
        CPen* saved = dc->SelectObject(&pen);
        dc->MoveTo(rect->left + m4::g_62b434, rect->top);
        dc->LineTo(rect->left + m4::g_62b434, rect->top + 0xc);
        dc->SelectObject(saved);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFontConfig::RenderInputText - the edit-control render path. Copies m_inputText and,
// when Ctrl is held, masks every char with '*'; runs a blink countdown (g_62b438)
// toggling g_62b43c; then (unless blinked-off + empty) selects m_arialFont,
// DrawTextA-measures the masked text, right-aligns it if it overflows maxWidth, and
// renders it into the rect. thiscall member, /GX (destructible CString).
// (ex m4::PwdHost - dissolved onto CFontConfig; PwdStr was an MFC CString.)
// @early-stop
// regalloc/EH-state wall. Complete correct reconstruction: the /GX frame, the
// arg-null gate before the CString copy, the Ctrl-held '*'-mask loop, the
// g_645584/g_62b438 countdown + g_62b43c toggle, the blink-off-empty caret branch,
// the font SelectObject save/restore, the DT_CALCRECT measure + overflow
// right-align, and both DrawTextA renders align by shape (llvm-objdump -dr).
// Residual is MSVC5 pinning the shared zero in edi + reusing dead arg slots for the
// CString/RECT locals differently, shifting the [esp+N] operands and EH scope addend.
RVA(0x00022160, 0x18e)
i32 CFontConfig::RenderInputText(HDC hdc, i32 maxWidth, RECT* rect) {
    if (hdc == 0) {
        return 0;
    }
    CString text(m_inputText);
    if (::GetAsyncKeyState(0x11) & 0x8000) {
        for (i32 i = 0; i < text.GetLength(); i++) {
            text.SetAt(i, '*');
        }
    }
    i32 t;
    if ((u32)m4::g_645584 < (u32)m4::g_62b438) {
        t = m4::g_62b438 - m4::g_645584;
    } else {
        t = 0;
    }
    m4::g_62b438 = t;
    if (t == 0) {
        m4::g_62b438 = 0xc8;
        m4::g_62b43c ^= 1;
    }
    if (m4::g_62b43c != 0 && text.GetLength() == 0) {
        MeasureLabel(hdc, rect); // via ILT 0x258b (ex the phantom "Draw258b" duplicate decl)
    } else {
        HGDIOBJ prev = 0;
        if (m_arialFont) {
            prev = ::SelectObject(hdc, m_arialFont);
        }
        if (m4::g_62b43c) {
            MeasureLabel(hdc, rect); // via ILT 0x258b (ex the phantom "Draw258b" duplicate decl)
        }
        int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = ::DrawTextA;
        RECT rc;
        rc.left = rect->left;
        rc.top = rect->top;
        rc.right = rect->right;
        rc.bottom = rect->bottom;
        pDraw(hdc, text, text.GetLength(), &rc, 0x420);
        i32 fmt = ((rc.right - rc.left) <= maxWidth) ? 0x20 : 0x22;
        m4::g_60c7a8 = fmt;
        pDraw(hdc, text, text.GetLength(), rect, fmt);
        if (prev) {
            ::SelectObject(hdc, prev);
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
// CFontConfig::DrawWithFont (0x22770): draw a plain C string with the ARIAL UI
// font. Null-guard hdc/text/rect, select m_arialFont (saving the prior), DrawTextA
// the strlen(text) into rect with the caller's format, then restore the font. ret 1.
// (ex m4::PwdHost.)
RVA(0x00022770, 0x7d)
i32 CFontConfig::DrawWithFont(const char* text, HDC hdc, RECT* rect, UINT format) {
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
    if (m_arialFont) {
        prev = ::SelectObject(hdc, m_arialFont);
    }
    ::DrawTextA(hdc, text, strlen(text), rect, format);
    if (prev) {
        ::SelectObject(hdc, prev);
    }
    return 1;
}

// -------------------------------------------------------------------------
// CFontConfig::Draw3DText - a centered "3D" text renderer. Selects the Message
// or Training font, sets transparent bk, copies the source CString, DT_CALCRECT-
// measures it centered in the dst rect, and draws it centered - first a black
// shadow pass offset by (dx,dy) when the shadow flag is set, then the RGB(r,g,b)
// main pass. thiscall member, 10 args, /GX (destructible CString).
// (ex m4::TextHost - its m_3c/m_40 are m_trainingFont/m_messageFont.)
// @early-stop
// regalloc/scheduling wall. Complete correct reconstruction: the /GX frame, the
// three arg-null gates before the CString copy, the two-font SelectObject, the
// transparent-bk setup, the DT_CALCRECT centering math (signed /2 round-toward-
// zero on both axes), the black-shadow offset pass and the RGB main pass all
// align by shape (llvm-objdump -dr). Residual is MSVC5 permuting the rc/centering
// temporaries across ebx/ebp/esi/edi vs retail and reusing dead arg slots for the
// rc + selPrev locals differently, shifting the [esp+N] operands - not steerable.
RVA(0x00022810, 0x22a)
i32 CFontConfig::Draw3DText(
    const CString* strSrc,
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
    HGDIOBJ obj = fontFlag ? m_messageFont : m_trainingFont;
    if (obj) {
        selPrev = ::SelectObject(hdc, obj);
    }
    ::SetBkMode(hdc, 1);
    ::SetBkColor(hdc, 0);
    CString text(*strSrc);
    ::DrawTextA(hdc, text, strlen(text), &rc, 0x411);
    i32 hoff = (dst->right + rc.left - dst->left - rc.right) / 2;
    i32 voff = (dst->bottom - dst->top + rc.top - rc.bottom) / 2;
    rc.left += hoff;
    rc.right += hoff;
    rc.top += voff;
    rc.bottom += voff;
    if (shadow) {
        ::SetTextColor(hdc, 0);
        rc.left += dx;
        rc.top += dy;
        rc.right += dx;
        rc.bottom += dy;
        ::DrawTextA(hdc, text, strlen(text), &rc, 0x11);
        rc.right -= dx;
        rc.left -= dx;
        rc.bottom -= dy;
        rc.top -= dy;
    }
    ::SetTextColor(hdc, RGB(r, g, b));
    ::DrawTextA(hdc, text, strlen(text), &rc, 0x11);
    if (selPrev) {
        ::SelectObject(hdc, selPrev);
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
