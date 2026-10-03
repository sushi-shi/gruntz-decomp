#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/FontConfig.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/ColorTintRef.h>
#include <Gruntz/GruntDirStatics.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>

#include <string.h>

i32 g_lastDrawTextFormat = DT_SINGLELINE;

i32 g_chatTextWidth = 0;

i32 g_caretBlinkMs = 0;

b32 g_caretBlinkOn = false;

i32 CFontConfig::LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold) {
    m_lowScrollThreshold = lowScrollThreshold;
    m_highScrollThreshold = highScrollThreshold;
    m_scrollOffset = 0;
    m_inputScrollTotal = 0;
    m_inputActive = false;

    m_arialFont = CreateFontA(
        0xc,
        8,
        0,
        0,
        FW_BOLD,
        0,
        0,
        0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "ARIAL"
    );
    if (!m_arialFont) {
        m_arialFont = CreateFontA(
            0xc,
            8,
            0,
            0,
            FW_BOLD,
            0,
            0,
            0,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            NULL
        );
    }

    std::string arial("ARIAL");

    const char* faceTF = (*g_buteMgr.GetString("Font", "TrainingFont", static_cast<std::string*>(&arial))).c_str();
    m_trainingFont = CreateFontA(
        g_buteMgr.GetInt("Font", "TrainingFontHeight", 0x1c),
        g_buteMgr.GetInt("Font", "TrainingFontWidth", 0xe),
        0,
        0,
        FW_BOLD,
        0,
        0,
        0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        faceTF
    );
    if (!m_trainingFont) {
        m_trainingFont = CreateFontA(
            g_buteMgr.GetInt("Font", "TrainingFontHeight", 0x18),
            g_buteMgr.GetInt("Font", "TrainingFontWidth", 0x10),
            0,
            0,
            FW_BOLD,
            0,
            0,
            0,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            NULL
        );
    }

    const char* faceMF = (*g_buteMgr.GetString("Font", "MessageFont", static_cast<std::string*>(&arial))).c_str();
    m_messageFont = CreateFontA(
        g_buteMgr.GetInt("Font", "MessageFontHeight", 0x2a),
        g_buteMgr.GetInt("Font", "MessageFontWidth", 0x18),
        0,
        0,
        FW_BOLD,
        0,
        0,
        0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        faceMF
    );
    if (!m_messageFont) {
        m_messageFont = CreateFontA(
            g_buteMgr.GetInt("Font", "MessageFontHeight", 0x2a),
            g_buteMgr.GetInt("Font", "MessageFontWidth", 0x18),
            0,
            0,
            FW_BOLD,
            0,
            0,
            0,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            NULL
        );
    }

    return 1;
}

void CFontConfig::Reset() {
    FreeNodes();
    (m_inputText).erase();
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

void CFontConfig::FreeNodes() {
    std::list<FontItem*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        FontItem* item = static_cast<FontItem*>(*(pos++));
        if (item) {
            delete item;
        }
    }
    m_list.clear();
    (m_inputText).erase();
    m_inputActive = false;
}

i32 CFontConfig::AddItem(const char* str, GZ_ENUM_PARAM(FontItemFlags, i32) flags, i32 payload) {
    if (!str) {
        return 0;
    }
    if (!*str) {
        return 0;
    }
    if (HAS(flags, FONT_ITEM_CLEAR_EXISTING)) {
        std::list<FontItem*>::iterator pos = m_list.begin();
        while (pos != m_list.end()) {
            FontItem* item = static_cast<FontItem*>(*(pos++));
            if (item) {
                delete item;
            }
        }
        m_list.clear();
    }
    FontItem* item = new FontItem;
    item->m_name = str;
    item->m_flags = flags;
    item->m_payload = payload;
    if (HAS(flags, FONT_ITEM_PREPEND)) {
        m_list.insert(m_list.begin(), item);
    } else {
        m_list.insert(m_list.end(), item);
    }
    return 1;
}

void CFontConfig::Scroll(i32 delta) {
    if (m_inputActive) {
        m_inputScrollTotal += delta;
    }
    i32 count = static_cast<i32>(m_list.size());
    if (!count) {
        m_scrollOffset = 0;
    }
    m_scrollOffset += delta;

    FontItem* item;
    if (count > 3) {
        if (m_scrollOffset < m_highScrollThreshold) {
            return;
        }
        item = static_cast<FontItem*>(takeFront(m_list));
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
        item = static_cast<FontItem*>(takeFront(m_list));
        if (!item) {
            return;
        }
    }
    (item->m_name).erase();

    item->~FontItem();
    ::operator delete(item);
    m_scrollOffset = 0;
}

i32 CFontConfig::HandleInputChar(i32 charCode, i32 keyData) {
    static_cast<void>(keyData);
    m_inputScrollTotal = 0;
    if (charCode == '\r') {
        if (m_inputActive == false) {
            m_inputActive = true;
            m_scrollOffset = 0;
            m_inputScrollTotal = 0;
            m_inputText = static_cast<const char*>("");
        } else {
            if ((m_inputText).empty()) {
                return 0;
            }
            m_inputActive = false;
            return 1;
        }
    }
    if (m_inputActive == false) {
        return 0;
    }
    if (charCode == '\b') {
        i32 len = static_cast<i32>((m_inputText).size());
        if (len <= 0) {
            return 0;
        }
        m_inputText.resize(len - 1);
        return 0;
    }
    if (charCode < ' ' || charCode > 0xff) {
        return 0;
    }
    if (static_cast<i32>((m_inputText).size()) < 0x50) {
        m_inputText += static_cast<char>(charCode);
    }
    return 0;
}

void CFontConfig::EndInput() {
    if (m_inputActive != false) {
        m_inputActive = false;
        (m_inputText).erase();
    }
}

i32 CFontConfig::MeasureLabel(HDC hdc, RECT* rect) {
    if (hdc == NULL) {
        return 0;
    }
    std::string text(m_inputText);
    if ((text).empty()) {
        g_chatTextWidth = 0;
    } else {
        RECT rc = *rect;
        DrawTextA(hdc, (text).c_str(), static_cast<i32>((text).size()), &rc, DT_CALCRECT | DT_SINGLELINE);
        i32 textW = rc.right - rc.left;
        i32 provW = rect->right - rect->left;
        g_chatTextWidth = Min(provW, textW);
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

i32 CFontConfig::RenderInputText(HDC hdc, i32 maxWidth, RECT* rect) {
    if (hdc == NULL) {
        return 0;
    }
    std::string text(m_inputText);
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        for (i32 i = 0; i < static_cast<i32>((text).size()); i++) {
            (text)[i] = '*';
        }
    }
    i32 t;
    if (g_frameDelta >= static_cast<u32>(g_caretBlinkMs)) {
        t = 0;
    } else {
        t = g_caretBlinkMs - g_frameDelta;
    }
    g_caretBlinkMs = t;
    if (t == 0) {
        g_caretBlinkMs = 0xc8;
        g_caretBlinkOn ^= 1;
    }
    if (g_caretBlinkOn != false && (text).empty()) {
        MeasureLabel(hdc, rect);
        return 1;
    }
    HGDIOBJ prev = NULL;
    if (m_arialFont) {
        prev = SelectObject(hdc, m_arialFont);
    }
    if (g_caretBlinkOn) {
        MeasureLabel(hdc, rect);
    }
    int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = DrawTextA;
    RECT rc = *rect;
    pDraw(hdc, (text).c_str(), static_cast<i32>((text).size()), &rc, DT_CALCRECT | DT_SINGLELINE);
    i32 fmt = ((rc.right - rc.left) > maxWidth) ? DT_RIGHT | DT_SINGLELINE : DT_SINGLELINE;
    g_lastDrawTextFormat = fmt;
    pDraw(hdc, (text).c_str(), static_cast<i32>((text).size()), rect, fmt);
    if (prev) {
        SelectObject(hdc, prev);
    }
    return 1;
}

typedef enum TextColorRef {
    TCLR_ORANGE = RGB(255, 128, 0),
    TCLR_GREEN = RGB(0, 255, 0),
    TCLR_BLUE = RGB(0, 0, 255),
    TCLR_RED = RGB(255, 0, 0),
    TCLR_PURPLE = RGB(128, 0, 128),
    TCLR_YELLOW = RGB(255, 255, 0),
    TCLR_ROSE = RGB(255, 0, 128),
    TCLR_BLACK = RGB(0, 0, 0),
    TCLR_NAVY = RGB(0, 0, 128),
    TCLR_DKGREEN = RGB(0, 128, 0),
    TCLR_TEAL = RGB(0, 128, 128),
    TCLR_MAROON = RGB(128, 0, 0),
    TCLR_MAGENTA = RGB(255, 0, 255),
    TCLR_OLIVE = RGB(128, 128, 0),
    TCLR_GRAY = RGB(128, 128, 128),
    TCLR_CYAN = RGB(0, 255, 255),
    TCLR_WHITE = RGB(255, 255, 255),
} TextColorRef;

i32 CFontConfig::DrawTextLines(i32 count, HDC hdc, RECT* rect, UINT format) {
    if (hdc == NULL) {
        return 0;
    }
    if (count <= 0) {
        return 0;
    }

    if (static_cast<i32>(m_list.size()) <= 0) {
        return 0;
    }
    while (static_cast<i32>(m_list.size()) > count) {
        FontItem* dead = static_cast<FontItem*>(takeFront(m_list));
        if (dead != NULL) {
            (dead->m_name).erase();
            delete dead;
        }
    }
    i32 n = min(count, static_cast<i32>(m_list.size()));
    if (n <= 0) {
        return 0;
    }
    RECT calc;
    RECT cur = *rect;
    RECT work = *rect;
    for (i32 i = 0; i < n; i++) {
        HGDIOBJ savedFont = NULL;
        if (m_arialFont) {
            savedFont = SelectObject(hdc, m_arialFont);
        }
        FontItem* item = static_cast<FontItem*>(*(iteratorAt(m_list.begin(), m_list.end(), i)));
        if (item != NULL) {
            if (HAS(item->m_flags, FONT_ITEM_SHADOW)) {
                SetTextColor(hdc, TCLR_BLACK);
                SET_RECT_XY_EXTENTS(work, cur.left + 1, cur.right + 1, cur.top + 1, cur.bottom + 1);
                DrawTextA(hdc, (item->m_name).c_str(), strlen((item->m_name).c_str()), &work, format);
            }
            if (HAS(item->m_flags, FONT_ITEM_COLORED)) {
                COLORREF color;
                color = TintColorRef(static_cast<ColorTint>(item->m_payload));
                SetTextColor(hdc, color);
            } else {
                SetTextColor(hdc, TCLR_WHITE);
            }
            calc = cur;
            DrawTextA(hdc, (item->m_name).c_str(), strlen((item->m_name).c_str()), &calc, format | DT_CALCRECT);
            DrawTextA(hdc, (item->m_name).c_str(), strlen((item->m_name).c_str()), &cur, format);
            i32 measuredBottom = calc.bottom;
            i32 measuredLeft = calc.left;
            i32 rr = rect->right;
            i32 rb = rect->bottom;
            calc.top = measuredBottom;
            calc.bottom = rb;
            calc.right = rr;
            SET_RECT_COMPONENTS(cur, measuredLeft, measuredBottom, rr, rb);
            SetTextColor(hdc, TCLR_WHITE);
        }
        if (savedFont) {
            SelectObject(hdc, savedFont);
        }
    }
    return 1;
}

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
    HGDIOBJ prev = NULL;
    if (m_arialFont) {
        prev = SelectObject(hdc, m_arialFont);
    }
    DrawTextA(hdc, text, strlen(text), rect, format);
    if (prev) {
        SelectObject(hdc, prev);
    }
    return 1;
}

i32 CFontConfig::Draw3DText(
    const std::string* strSrc,
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
    HGDIOBJ selPrev = NULL;
    RECT rc = *dst;
    if (fontFlag == 0) {
        if (m_trainingFont) {
            selPrev = SelectObject(hdc, m_trainingFont);
        }
    } else {
        if (m_messageFont) {
            selPrev = SelectObject(hdc, m_messageFont);
        }
    }
    SetBkMode(hdc, TRANSPARENT);
    SetBkColor(hdc, RGB(0, 0, 0));
    std::string text(*strSrc);
    DrawTextA(hdc, (text).c_str(), strlen((text).c_str()), &rc, DT_CALCRECT | DT_WORDBREAK | DT_CENTER);
    i32 hoff = (dst->right + rc.left - dst->left - rc.right) / 2;
    i32 voff = (dst->bottom - dst->top + rc.top - rc.bottom) / 2;
    rc.left += hoff;
    rc.right += hoff;
    rc.top += voff;
    rc.bottom += voff;
    if (shadow) {
        SetTextColor(hdc, RGB(0, 0, 0));
        rc.left += dx;
        rc.top += dy;
        rc.right += dx;
        rc.bottom += dy;
        DrawTextA(hdc, (text).c_str(), strlen((text).c_str()), &rc, DT_WORDBREAK | DT_CENTER);
        rc.right -= dx;
        rc.left -= dx;
        rc.bottom -= dy;
        rc.top -= dy;
    }
    SetTextColor(hdc, RGB(r, g, b));
    DrawTextA(hdc, (text).c_str(), strlen((text).c_str()), &rc, DT_WORDBREAK | DT_CENTER);
    if (selPrev) {
        SelectObject(hdc, selPrev);
    }
    return 1;
}

CFontConfig::~CFontConfig() {
    Reset();
}
