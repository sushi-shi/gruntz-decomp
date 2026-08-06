#include <rva.h>

#include <Gruntz/FontConfig.h>
#include <Gruntz/GruntDirStatics.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Rez/FrameClock.h>

#include <string.h>

DATA(0x0020c7a8)
i32 g_lastDrawTextFormat = 0;
DATA(0x0022b434)
i32 g_chatTextWidth = 0;
DATA(0x0022b438)
i32 g_caretBlinkMs = 0;
DATA(0x0022b43c)
i32 g_caretBlinkOn = 0;

RVA(0x000218e0, 0x1ff)
i32 CFontConfig::LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold) {
    m_lowScrollThreshold = lowScrollThreshold;
    m_highScrollThreshold = highScrollThreshold;
    m_scrollOffset = 0;
    m_inputScrollTotal = 0;
    m_inputActive = 0;

    m_arialFont = CreateFontA(0xc, 8, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, "ARIAL");
    if (!m_arialFont) {
        m_arialFont = CreateFontA(0xc, 8, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0);
    }

    CString arial("ARIAL");

    const char* faceTF = static_cast<const char*>(
        *g_buteMgr.GetStringDef("Font", "TrainingFont", static_cast<CString*>(&arial))
    );
    m_trainingFont = CreateFontA(
        g_buteMgr.GetIntDef("Font", "TrainingFontHeight", 0x1c),
        g_buteMgr.GetIntDef("Font", "TrainingFontWidth", 0xe),
        0,
        0,
        FW_BOLD,
        0,
        0,
        0,
        DEFAULT_CHARSET,
        0,
        0,
        0,
        0,
        faceTF
    );
    if (!m_trainingFont) {
        // The retry deliberately asks for DIFFERENT default metrics - 24x16
        // rather than 28x14 - so the two GetIntDef defaults for
        // "TrainingFontHeight"/"TrainingFontWidth" disagreeing is retail's
        // fallback, not a transcription slip.
        m_trainingFont = CreateFontA(
            g_buteMgr.GetIntDef("Font", "TrainingFontHeight", 0x18),
            g_buteMgr.GetIntDef("Font", "TrainingFontWidth", 0x10),
            0,
            0,
            FW_BOLD,
            0,
            0,
            0,
            DEFAULT_CHARSET,
            0,
            0,
            0,
            0,
            0
        );
    }

    const char* faceMF = static_cast<const char*>(
        *g_buteMgr.GetStringDef("Font", "MessageFont", static_cast<CString*>(&arial))
    );
    m_messageFont = CreateFontA(
        g_buteMgr.GetIntDef("Font", "MessageFontHeight", 0x2a),
        g_buteMgr.GetIntDef("Font", "MessageFontWidth", 0x18),
        0,
        0,
        FW_BOLD,
        0,
        0,
        0,
        DEFAULT_CHARSET,
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
            FW_BOLD,
            0,
            0,
            0,
            DEFAULT_CHARSET,
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
        m_arialFont = NULL;
    }
    if (m_trainingFont) {
        DeleteObject(m_trainingFont);
        m_trainingFont = NULL;
    }
    if (m_messageFont) {
        DeleteObject(m_messageFont);
        m_messageFont = NULL;
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

RVA_COMPGEN(0x00021c40, 0x8, ??1FontItem@@QAE@XZ)

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

// @early-stop
RVA(0x00021e20, 0x95)

i32 CFontConfig::TypeChar(i32 ch, i32 flag) {
    m_inputScrollTotal = 0;
    if (ch == '\r') {
        if (m_inputActive == 0) {
            m_inputActive = 1;
            m_scrollOffset = 0;
            m_inputScrollTotal = 0;
            m_inputText = static_cast<const char*>(g_emptyString);
        } else {
            if (m_inputText.GetLength() == 0) {
                return 0;
            }
            m_inputActive = 0;
            return 1;
        }
    }
    if (m_inputActive == 0) {
        return 0;
    }
    if (ch == '\b') {
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

// @early-stop
RVA(0x00021f20, 0x162)
i32 CFontConfig::MeasureLabel(HDC hdc, RECT* rect) {
    if (hdc == NULL) {
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
        DrawTextA(hdc, text, text.GetLength(), &rc, 0x420);
        i32 textW = rc.right - rc.left;
        i32 provW = rect->right - rect->left;
        g_chatTextWidth = provW;
        if (provW >= textW) {
            g_chatTextWidth = textW;
        }
    }

    CDC* dc = CDC::FromHandle(hdc);
    if (dc != NULL) {
        CPen pen(PS_SOLID, 2, RGB(0, 0, 0));
        CPen* saved = dc->SelectObject(&pen);
        dc->MoveTo(rect->left + g_chatTextWidth, rect->top);
        dc->LineTo(rect->left + g_chatTextWidth, rect->top + 0xc);
        dc->SelectObject(saved);
    }
    return 1;
}

// @early-stop
RVA(0x00022160, 0x18e)
i32 CFontConfig::RenderInputText(HDC hdc, i32 maxWidth, RECT* rect) {
    if (hdc != NULL) {
        CString text(m_inputText);
        if (GetAsyncKeyState(0x11) & 0x8000) {
            for (i32 i = 0; i < text.GetLength(); i++) {
                text.SetAt(i, '*');
            }
        }
        i32 t;
        if (g_frameDelta < static_cast<u32>(g_caretBlinkMs)) {
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
            MeasureLabel(hdc, rect);
        } else {
            HGDIOBJ prev = 0;
            if (m_arialFont) {
                prev = SelectObject(hdc, m_arialFont);
            }
            if (g_caretBlinkOn) {
                MeasureLabel(hdc, rect);
            }
            int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = DrawTextA;
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
                SelectObject(hdc, prev);
            }
        }
        return 1;
    }
    return 0;
}

typedef enum FontItemFlag {
    FONTITEM_COLORED = 0x10,
    FONTITEM_SHADOW = 0x20,
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

// @early-stop
RVA(0x00022360, 0x338)
i32 CFontConfig::DrawTextLines(i32 count, HDC hdc, RECT* rect, UINT format) {
    if (hdc == NULL) {
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
        if (dead != NULL) {
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
        if (item != NULL) {
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
    if (hdc == NULL) {
        return 0;
    }
    if (text == NULL) {
        return 0;
    }
    if (rect == NULL) {
        return 0;
    }
    HGDIOBJ prev = 0;
    if (m_arialFont) {
        prev = SelectObject(hdc, m_arialFont);
    }
    DrawTextA(hdc, text, strlen(text), rect, format);
    if (prev) {
        SelectObject(hdc, prev);
    }
    return 1;
}

// @early-stop
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
    if (hdc == NULL) {
        return 0;
    }
    if (dst == NULL) {
        return 0;
    }
    if (strSrc == NULL) {
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
        selPrev = SelectObject(hdc, obj);
    }
    SetBkMode(hdc, 1);
    SetBkColor(hdc, 0);
    CString text(*strSrc);
    DrawTextA(hdc, text, strlen(text), &rc, 0x411);
    i32 hoff = (dst->right + rc.left - dst->left - rc.right) / 2;
    i32 voff = (dst->bottom - dst->top + rc.top - rc.bottom) / 2;
    rc.left += hoff;
    rc.right += hoff;
    rc.top += voff;
    rc.bottom += voff;
    if (shadow) {
        SetTextColor(hdc, 0);
        rc.left += dx;
        rc.top += dy;
        rc.right += dx;
        rc.bottom += dy;
        DrawTextA(hdc, text, strlen(text), &rc, 0x11);
        rc.right -= dx;
        rc.left -= dx;
        rc.bottom -= dy;
        rc.top -= dy;
    }
    SetTextColor(hdc, RGB(r, g, b));
    DrawTextA(hdc, text, strlen(text), &rc, 0x11);
    if (selPrev) {
        SelectObject(hdc, selPrev);
    }
    return 1;
}

RVA(0x00085f40, 0x56)
CFontConfig::~CFontConfig() {
    Reset();
}
