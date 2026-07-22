#include <Mfc.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <EmptyString.h> // g_emptyString
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <Gruntz/FontConfig.h>
#include <rva.h>

#include <Bute/ButeMgr.h>
#include <string.h> // strlen (the m4 draw helpers)

DATA(0x0020c7a8)
i32 g_lastDrawTextFormat = 0; // 0x60c7a8: last DrawTextA format flags used
DATA(0x0022b434)
i32 g_chatTextWidth = 0; // 0x62b434: DT_CALCRECT-measured text width, clamped
DATA(0x0022b438)
i32 g_caretBlinkMs = 0; // 0x62b438: caret blink countdown in ms - the frame
DATA(0x0022b43c)
i32 g_caretBlinkOn = 0; // 0x62b43c: caret blink phase (XOR 1 each expiry)

RVA(0x000218e0, 0x1ff)
i32 CFontConfig::LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold) {
    m_lowScrollThreshold = lowScrollThreshold;
    m_highScrollThreshold = highScrollThreshold;
    m_scrollOffset = 0;
    m_inputScrollTotal = 0;
    m_inputActive = 0;

    // --- ARIAL UI font (fixed 12x8 bold ANSI) -------------------------------
    m_arialFont = CreateFontA(0xc, 8, 0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, "ARIAL");
    if (!m_arialFont) {
        m_arialFont = CreateFontA(0xc, 8, 0, 0, 0x2bc, 0, 0, 0, 1, 0, 0, 0, 0, "ARIAL");
    }

    // The ARIAL default-face CString temp - constructed here (just-in-time, after
    // the first CreateFontA), passed by address as the GetStringDef default for
    // both the Training and Message font face lookups; torn down at the tail.
    CString arial("ARIAL");

    // --- TrainingFont (face/dims from config, default ARIAL / 14x28) --------
    const char* faceTF =
        static_cast<const char*>(*g_buteMgr.GetStringDef("Font", "TrainingFont", static_cast<CString*>(&arial)));
    m_trainingFont = CreateFontA(
        g_buteMgr.GetIntDef("Font", "TrainingFontHeight", 0x1c),
        g_buteMgr.GetIntDef("Font", "TrainingFontWidth", 0xe),
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
            g_buteMgr.GetIntDef("Font", "TrainingFontHeight", 0x18),
            g_buteMgr.GetIntDef("Font", "TrainingFontWidth", 0x10),
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
        static_cast<const char*>(*g_buteMgr.GetStringDef("Font", "MessageFont", static_cast<CString*>(&arial)));
    m_messageFont = CreateFontA(
        g_buteMgr.GetIntDef("Font", "MessageFontHeight", 0x2a),
        g_buteMgr.GetIntDef("Font", "MessageFontWidth", 0x18),
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
            g_buteMgr.GetIntDef("Font", "MessageFontHeight", 0x2a),
            g_buteMgr.GetIntDef("Font", "MessageFontWidth", 0x18),
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

RVA(0x00021bd0, 0x45)
void CFontConfig::FreeNodes() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos) {
        FontItem* item = static_cast<FontItem*>(m_list.GetNext(pos));
        if (item) {
            delete item;
        }
    }
    m_list.RemoveAll();
    m_inputText.Empty();
    m_inputActive = 0;
}

RVA(0x00021c40, 0x8)
FontItem::~FontItem() {}

RVA(0x00021c60, 0xde)
i32 CFontConfig::AddItem(const char* str, i32 type, i32 data) {
    if (!str) {
        return 0;
    }
    if (!*str) {
        return 0;
    }
    if (type & 4) {
        POSITION pos = m_list.GetHeadPosition();
        while (pos) {
            FontItem* item = static_cast<FontItem*>(m_list.GetNext(pos));
            if (item) {
                delete item;
            }
        }
        m_list.RemoveAll();
    }
    FontItem* item = new FontItem;
    item->name = str;
    item->type = type;
    item->data = data;
    if (type & 2) {
        m_list.AddHead(item);
    } else {
        m_list.AddTail(item);
    }
    return 1;
}

RVA(0x00021d80, 0x79)
void CFontConfig::Scroll(i32 delta) {
    if (m_inputActive) {
        m_inputScrollTotal += delta;
    }
    i32 count = m_list.GetCount();
    if (!count) {
        m_scrollOffset = 0;
    }
    m_scrollOffset += delta;

    FontItem* item;
    if (count > 3) {
        if (m_scrollOffset < m_highScrollThreshold) {
            return;
        }
        item = static_cast<FontItem*>(m_list.RemoveHead());
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
        item = static_cast<FontItem*>(m_list.RemoveHead());
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
        m_inputText = static_cast<const char*>(g_emptyString);
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
        m_inputText += static_cast<char>(ch);
    }
    return 0;
}

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
// g_caretOffsetX, then stroke the 12px insertion caret at that offset with a 2px pen.
//
// Retail (0x21f20): `add ecx,0x1c` + the CString COPY CTOR (0x1b9ba3) makes a temp of
// THIS->m_inputText - that is the text, whose length it reads at [eax-8] and pushes to
// DrawTextA - while arg2 (esi) is the RECT* it reads left/top/right/bottom from at
// [ecx]..[ecx+0xc].
// @early-stop
// The caret tail is the REAL MFC CDC/CPen. The residual is the DT_CALCRECT measure block's RECT-copy
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
        g_chatTextWidth = 0;
    } else {
        RECT rc;
        rc.left = rect->left;
        rc.top = rect->top;
        rc.right = rect->right;
        rc.bottom = rect->bottom;
        ::DrawTextA(hdc, text, text.GetLength(), &rc, 0x420);
        i32 textW = rc.right - rc.left;
        i32 provW = rect->right - rect->left;
        g_chatTextWidth = provW;
        if (provW >= textW) {
            g_chatTextWidth = textW;
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
        dc->MoveTo(rect->left + g_chatTextWidth, rect->top);
        dc->LineTo(rect->left + g_chatTextWidth, rect->top + 0xc);
        dc->SelectObject(saved);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFontConfig::RenderInputText - the edit-control render path. Copies m_inputText and,
// when Ctrl is held, masks every char with '*'; runs a blink countdown (g_caretBlinkTimer)
// toggling g_caretBlinkOn; then (unless blinked-off + empty) selects m_arialFont,
// DrawTextA-measures the masked text, right-aligns it if it overflows maxWidth, and
// renders it into the rect. thiscall member, /GX (destructible CString).
// (PwdStr is an MFC CString.)
// @early-stop
// regalloc/EH-state wall. Complete correct reconstruction: the /GX frame, the
// arg-null gate before the CString copy, the Ctrl-held '*'-mask loop, the
// g_frameDelta/g_caretBlinkTimer countdown + g_caretBlinkOn toggle, the blink-off-empty caret branch,
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
    if (static_cast<u32>(g_frameDelta) < static_cast<u32>(g_caretBlinkMs)) {
        t = g_caretBlinkMs - g_frameDelta;
    } else {
        t = 0;
    }
    g_caretBlinkMs = t;
    if (t == 0) {
        g_caretBlinkMs = 0xc8;
        g_caretBlinkOn ^= 1;
    }
    if (g_caretBlinkOn != 0 && text.GetLength() == 0) {
        MeasureLabel(hdc, rect); // via ILT 0x258b
    } else {
        HGDIOBJ prev = 0;
        if (m_arialFont) {
            prev = ::SelectObject(hdc, m_arialFont);
        }
        if (g_caretBlinkOn) {
            MeasureLabel(hdc, rect); // via ILT 0x258b
        }
        int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = ::DrawTextA;
        RECT rc;
        rc.left = rect->left;
        rc.top = rect->top;
        rc.right = rect->right;
        rc.bottom = rect->bottom;
        pDraw(hdc, text, text.GetLength(), &rc, 0x420);
        i32 fmt = ((rc.right - rc.left) <= maxWidth) ? 0x20 : 0x22;
        g_lastDrawTextFormat = fmt;
        pDraw(hdc, text, text.GetLength(), rect, fmt);
        if (prev) {
            ::SelectObject(hdc, prev);
        }
    }
    return 1;
}

typedef enum FontItemFlag {
    FONTITEM_COLORED = 0x10, // item->data is a TextColorId palette index
    FONTITEM_SHADOW = 0x20,  // stroke a 1px black drop shadow first
} FontItemFlag;

typedef enum TextColorId {
    TEXTCOLOR_ORANGE = 0,
    TEXTCOLOR_GREEN = 1,
    TEXTCOLOR_BLUE = 2,
    TEXTCOLOR_RED = 3,
    TEXTCOLOR_PURPLE = 4,
    TEXTCOLOR_YELLOW = 5,
    TEXTCOLOR_ROSE = 6,
    TEXTCOLOR_BLACK = 7,
    TEXTCOLOR_NAVY = 8,
    TEXTCOLOR_DKGREEN = 9,
    TEXTCOLOR_TEAL = 10,
    TEXTCOLOR_MAROON = 11,
    TEXTCOLOR_MAGENTA = 12,
    TEXTCOLOR_OLIVE = 13,
    TEXTCOLOR_GRAY = 14,
    TEXTCOLOR_CYAN = 15,
    TEXTCOLOR_WHITE = 16,
} TextColorId;

typedef enum TextColorRef {
    TCLR_ORANGE = 0x0080ff,
    TCLR_GREEN = 0x00ff00,
    TCLR_BLUE = 0xff0000,
    TCLR_RED = 0x0000ff,
    TCLR_PURPLE = 0x800080,
    TCLR_YELLOW = 0x00ffff,
    TCLR_ROSE = 0x8000ff,
    TCLR_BLACK = 0x000000,
    TCLR_NAVY = 0x800000,
    TCLR_DKGREEN = 0x008000,
    TCLR_TEAL = 0x808000,
    TCLR_MAROON = 0x000080,
    TCLR_MAGENTA = 0xff00ff,
    TCLR_OLIVE = 0x008080,
    TCLR_GRAY = 0x808080,
    TCLR_CYAN = 0xffff00,
    TCLR_WHITE = 0xffffff,
} TextColorRef;

// -------------------------------------------------------------------------
// Draw the list's items as `count` stacked text lines into hdc. Trim the list
// to `count` (RemoveHead + delete), then per line: optional 1px black drop
// shadow (type&0x20), a palette color when type&0x10 (else white), a DT_CALCRECT
// measure into `calc`, the real DrawTextA, then advance `cur.top` to the measured
// bottom so the next line stacks below. Text color is reset to white each line.
// -------------------------------------------------------------------------
// @early-stop
// Complete, correct reconstruction (0% stub -> 78% fuzzy). Body/control-flow align.
// Residual is codegen shape, not logic: (1) the GDI calls bind __imp__{DrawTextA,
// SelectObject,SetTextColor} while retail calls through the game's own fn-ptr
// globals g_p*@m4 (0x6c454c/0x6c3ec4/0x6c3eb4) - the import-linking plateau the
// campaign is resolving globally (see recent link(imports) commits); (2) retail
// sinks the color `push` into each switch arm, cl here stores to `color` and pushes
// once at the merge; (3) cl DSE'd the dead pre-loop `work=*rect` copy retail keeps;
// (4) the min-branch is emitted with inverted sense. All logically equivalent.
RVA(0x00022360, 0x2f4)
i32 CFontConfig::DrawTextLines(i32 count, HDC hdc, RECT* rect, UINT format) {
    if (hdc == 0) {
        return 0;
    }
    if (count <= 0) {
        return 0;
    }
    if (m_list.GetCount() <= 0) {
        return 0;
    }
    while (m_list.GetCount() > count) {
        FontItem* dead = static_cast<FontItem*>(m_list.RemoveHead());
        if (dead != 0) {
            dead->name.Empty();
            delete dead;
        }
    }
    i32 n = (count >= m_list.GetCount()) ? m_list.GetCount() : count;
    if (n <= 0) {
        return 0;
    }
    RECT cur;
    RECT work;
    RECT calc;
    cur.left = rect->left;
    cur.top = rect->top;
    cur.right = rect->right;
    cur.bottom = rect->bottom;
    work.left = rect->left;
    work.top = rect->top;
    work.right = rect->right;
    work.bottom = rect->bottom;
    for (i32 i = 0; i < n; i++) {
        HGDIOBJ savedFont = 0;
        if (m_arialFont) {
            savedFont = SelectObject(hdc, m_arialFont);
        }
        FontItem* item = static_cast<FontItem*>(m_list.GetAt(m_list.FindIndex(i)));
        if (item != 0) {
            if (item->type & FONTITEM_SHADOW) {
                SetTextColor(hdc, TCLR_BLACK);
                work.left = cur.left + 1;
                work.right = cur.right + 1;
                work.top = cur.top + 1;
                work.bottom = cur.bottom + 1;
                DrawTextA(hdc, item->name, strlen(item->name), &work, format);
            }
            COLORREF color;
            if (item->type & FONTITEM_COLORED) {
                switch (item->data) {
                    case TEXTCOLOR_NAVY:
                        color = TCLR_NAVY;
                        break;
                    case TEXTCOLOR_DKGREEN:
                        color = TCLR_DKGREEN;
                        break;
                    case TEXTCOLOR_TEAL:
                        color = TCLR_TEAL;
                        break;
                    case TEXTCOLOR_MAROON:
                        color = TCLR_MAROON;
                        break;
                    case TEXTCOLOR_PURPLE:
                        color = TCLR_PURPLE;
                        break;
                    case TEXTCOLOR_OLIVE:
                        color = TCLR_OLIVE;
                        break;
                    case TEXTCOLOR_GRAY:
                        color = TCLR_GRAY;
                        break;
                    case TEXTCOLOR_BLUE:
                        color = TCLR_BLUE;
                        break;
                    case TEXTCOLOR_GREEN:
                        color = TCLR_GREEN;
                        break;
                    case TEXTCOLOR_CYAN:
                        color = TCLR_CYAN;
                        break;
                    case TEXTCOLOR_RED:
                        color = TCLR_RED;
                        break;
                    case TEXTCOLOR_MAGENTA:
                        color = TCLR_MAGENTA;
                        break;
                    case TEXTCOLOR_YELLOW:
                        color = TCLR_YELLOW;
                        break;
                    case TEXTCOLOR_WHITE:
                        color = TCLR_WHITE;
                        break;
                    case TEXTCOLOR_ORANGE:
                        color = TCLR_ORANGE;
                        break;
                    case TEXTCOLOR_ROSE:
                        color = TCLR_ROSE;
                        break;
                    default:
                        color = TCLR_BLACK;
                        break;
                }
            } else {
                color = TCLR_WHITE;
            }
            SetTextColor(hdc, color);
            calc.left = cur.left;
            calc.right = cur.right;
            calc.bottom = cur.bottom;
            calc.top = cur.top;
            DrawTextA(hdc, item->name, strlen(item->name), &calc, format | DT_CALCRECT);
            DrawTextA(hdc, item->name, strlen(item->name), &cur, format);
            i32 measuredBottom = calc.bottom;
            i32 measuredLeft = calc.left;
            i32 rr = rect->right;
            i32 rb = rect->bottom;
            calc.top = measuredBottom;
            calc.bottom = rb;
            calc.right = rr;
            cur.left = measuredLeft;
            cur.top = measuredBottom;
            cur.right = rr;
            cur.bottom = rb;
            SetTextColor(hdc, TCLR_WHITE);
        }
        if (savedFont) {
            SelectObject(hdc, savedFont);
        }
    }
    return 1;
}

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
// (its m_3c/m_40 are m_trainingFont/m_messageFont.)
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
// (Was @early-stop'd as an "EH/vptr wall": our model emitted an extra
// `mov [esi],&??_7CFontConfig` restamp retail elided, judged "not steerable by source
// spelling". It was steerable - the restamp existed only because we wrongly declared the
// class polymorphic. De-inheriting CPtrList (see FontConfig.h) removed it -> 100%.)
RVA(0x00085f40, 0x56)
CFontConfig::~CFontConfig() {
    Reset();
}

