#include <StdAfx.h>

#include <rva.h>

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

DATA(0x0020c7a8)
i32 g_inputTextDrawFormat = DT_SINGLELINE;
DATA(0x0022b434)
i32 g_inputCaretOffsetX = 0;
DATA(0x0022b438)
i32 g_caretBlinkRemainingMs = 0;
DATA(0x0022b43c)
b32 g_caretBlinkOn = false;

RVA(0x000218e0, 0x1ff)
i32 CGameText::Initialize(i32 messageHoldMs, i32 crowdedMessageHoldMs) {
    m_messageHoldMs = messageHoldMs;
    m_crowdedMessageHoldMs = crowdedMessageHoldMs;
    m_messageElapsedMs = 0;
    m_inputElapsedMs = 0;
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

    CString arial("ARIAL");

    const char* trainingFontFace = static_cast<const char*>(
        *g_buteMgr.GetString("Font", "TrainingFont", static_cast<CString*>(&arial))
    );
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
        trainingFontFace
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

    const char* messageFontFace = static_cast<const char*>(
        *g_buteMgr.GetString("Font", "MessageFont", static_cast<CString*>(&arial))
    );
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
        messageFontFace
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

RVA(0x00021b60, 0x4d)
void CGameText::Reset() {
    ClearMessages();
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
void CGameText::ClearMessages() {
    POSITION messagePosition = m_messages.GetHeadPosition();
    while (messagePosition) {
        GameTextMessage* message =
            static_cast<GameTextMessage*>(m_messages.GetNext(messagePosition));
        if (message) {
            delete message;
        }
    }
    m_messages.RemoveAll();
    m_inputText.Empty();
    m_inputActive = false;
}

RVA_COMPGEN(0x00021c40, 0x8, ??1GameTextMessage@@QAE@XZ)

RVA(0x00021c60, 0xde)
i32 CGameText::AddMessage(
    const char* messageText,
    GZ_ENUM_PARAM(GameTextFlags, i32) flags,
    i32 colorTint
) {
    if (!messageText) {
        return 0;
    }
    if (!*messageText) {
        return 0;
    }
    if (HAS(flags, GAME_TEXT_CLEAR_EXISTING)) {
        POSITION messagePosition = m_messages.GetHeadPosition();
        while (messagePosition) {
            GameTextMessage* message =
                static_cast<GameTextMessage*>(m_messages.GetNext(messagePosition));
            if (message) {
                delete message;
            }
        }
        m_messages.RemoveAll();
    }
    GameTextMessage* message = new GameTextMessage;
    message->m_text = messageText;
    message->m_flags = flags;
    message->m_colorTint = colorTint;
    if (HAS(flags, GAME_TEXT_PREPEND)) {
        m_messages.AddHead(message);
    } else {
        m_messages.AddTail(message);
    }
    return 1;
}

RVA(0x00021d80, 0x79)
void CGameText::AdvanceMessageTimer(i32 deltaMs) {
    if (m_inputActive) {
        m_inputElapsedMs += deltaMs;
    }
    i32 messageCount = m_messages.GetCount();
    if (!messageCount) {
        m_messageElapsedMs = 0;
    }
    m_messageElapsedMs += deltaMs;

    GameTextMessage* expiredMessage;
    if (messageCount > 3) {
        if (m_messageElapsedMs < m_crowdedMessageHoldMs) {
            return;
        }
        expiredMessage = static_cast<GameTextMessage*>(m_messages.RemoveHead());
        if (!expiredMessage) {
            return;
        }
    } else {
        if (m_messageElapsedMs < m_messageHoldMs) {
            return;
        }
        if (!messageCount) {
            return;
        }
        expiredMessage = static_cast<GameTextMessage*>(m_messages.RemoveHead());
        if (!expiredMessage) {
            return;
        }
    }
    expiredMessage->m_text.Empty();
    // Retail destroys and frees without a delete-expression null check.
    expiredMessage->~GameTextMessage();
    ::operator delete(expiredMessage);
    m_messageElapsedMs = 0;
}

// @early-stop
RVA(0x00021e20, 0x95)

i32 CGameText::HandleInputChar(i32 charCode, i32 keyData) {
    static_cast<void>(keyData);
    m_inputElapsedMs = 0;
    if (charCode == '\r') {
        if (m_inputActive == false) {
            m_inputActive = true;
            m_messageElapsedMs = 0;
            m_inputElapsedMs = 0;
            m_inputText = static_cast<const char*>("");
        } else {
            if (m_inputText.IsEmpty()) {
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
        i32 len = m_inputText.GetLength();
        if (len <= 0) {
            return 0;
        }
        m_inputText.GetBufferSetLength(len - 1);
        return 0;
    }
    if (charCode < ' ' || charCode > 0xff) {
        return 0;
    }
    if (m_inputText.GetLength() < 0x50) {
        m_inputText += static_cast<char>(charCode);
    }
    return 0;
}

RVA(0x00021ef0, 0x17)
void CGameText::EndInput() {
    if (m_inputActive != false) {
        m_inputActive = false;
        m_inputText.Empty();
    }
}

RVA(0x00021f20, 0x162)
i32 CGameText::DrawInputCaret(HDC hdc, RECT* rect) {
    if (hdc == NULL) {
        return 0;
    }
    CString text(m_inputText);
    if (text.IsEmpty()) {
        g_inputCaretOffsetX = 0;
    } else {
        RECT rc = *rect;
        DrawTextA(hdc, text, text.GetLength(), &rc, DT_CALCRECT | DT_SINGLELINE);
        i32 textWidth = rc.right - rc.left;
        i32 availableWidth = rect->right - rect->left;
        g_inputCaretOffsetX = Min(availableWidth, textWidth);
    }

    CDC* dc = CDC::FromHandle(hdc);
    if (dc != NULL) {
        CPen pen(PS_SOLID, 2, RGB(0, 0, 0));
        CPen* saved = dc->SelectObject(&pen);
        dc->MoveTo(rect->left + g_inputCaretOffsetX, rect->top);
        dc->LineTo(rect->left + g_inputCaretOffsetX, rect->top + 0xc);
        dc->SelectObject(saved);
    }
    return 1;
}

RVA_COMPGEN(0x000220f0, 0x46, ??1CPen@@UAE@XZ)

// @early-stop
RVA(0x00022160, 0x18e)
i32 CGameText::RenderInputText(HDC hdc, i32 maxWidth, RECT* rect) {
    if (hdc == NULL) {
        return 0;
    }
    CString text(m_inputText);
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        for (i32 i = 0; i < text.GetLength(); i++) {
            text.SetAt(i, '*');
        }
    }
    i32 t;
    if (g_frameDelta >= static_cast<u32>(g_caretBlinkRemainingMs)) {
        t = 0;
    } else {
        t = g_caretBlinkRemainingMs - g_frameDelta;
    }
    g_caretBlinkRemainingMs = t;
    if (t == 0) {
        g_caretBlinkRemainingMs = 0xc8;
        g_caretBlinkOn ^= 1;
    }
    if (g_caretBlinkOn != false && text.IsEmpty()) {
        DrawInputCaret(hdc, rect);
        return 1;
    }
    HGDIOBJ prev = NULL;
    if (m_arialFont) {
        prev = SelectObject(hdc, m_arialFont);
    }
    if (g_caretBlinkOn) {
        DrawInputCaret(hdc, rect);
    }
    int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = DrawTextA;
    RECT rc = *rect;
    pDraw(hdc, text, text.GetLength(), &rc, DT_CALCRECT | DT_SINGLELINE);
    i32 fmt = ((rc.right - rc.left) > maxWidth) ? DT_RIGHT | DT_SINGLELINE : DT_SINGLELINE;
    g_inputTextDrawFormat = fmt;
    pDraw(hdc, text, text.GetLength(), rect, fmt);
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

RVA(0x00022360, 0x338)
i32 CGameText::DrawMessages(i32 maxMessages, HDC hdc, RECT* bounds, UINT format) {
    if (hdc == NULL) {
        return 0;
    }
    if (maxMessages <= 0) {
        return 0;
    }
    // The signed maxMessages guard is required; IsEmpty emits a zero-only test.
    if (m_messages.GetCount() <= 0) {
        return 0;
    }
    while (m_messages.GetCount() > maxMessages) {
        GameTextMessage* expiredMessage = static_cast<GameTextMessage*>(m_messages.RemoveHead());
        if (expiredMessage != NULL) {
            expiredMessage->m_text.Empty();
            delete expiredMessage;
        }
    }
    i32 messageCount = min(maxMessages, m_messages.GetCount());
    if (messageCount <= 0) {
        return 0;
    }
    RECT measuredBounds;
    RECT messageBounds = *bounds;
    RECT shadowBounds = *bounds;
    for (i32 messageIndex = 0; messageIndex < messageCount; messageIndex++) {
        HGDIOBJ savedFont = NULL;
        if (m_arialFont) {
            savedFont = SelectObject(hdc, m_arialFont);
        }
        GameTextMessage* message =
            static_cast<GameTextMessage*>(m_messages.GetAt(m_messages.FindIndex(messageIndex)));
        if (message != NULL) {
            if (HAS(message->m_flags, GAME_TEXT_SHADOW)) {
                SetTextColor(hdc, TCLR_BLACK);
                shadowBounds = messageBounds;
                OFFSET_RECT_X_EDGES(shadowBounds, 1, 1);
                OFFSET_RECT_Y_EDGES(shadowBounds, 1, 1);
                DrawTextA(hdc, message->m_text, strlen(message->m_text), &shadowBounds, format);
            }
            if (HAS(message->m_flags, GAME_TEXT_COLORED)) {
                COLORREF color;
                color = TintColorRef(static_cast<ColorTint>(message->m_colorTint));
                SetTextColor(hdc, color);
            } else {
                SetTextColor(hdc, TCLR_WHITE);
            }
            measuredBounds = messageBounds;
            DrawTextA(
                hdc,
                message->m_text,
                strlen(message->m_text),
                &measuredBounds,
                format | DT_CALCRECT
            );
            DrawTextA(hdc, message->m_text, strlen(message->m_text), &messageBounds, format);
            measuredBounds.top = measuredBounds.bottom;
            measuredBounds.bottom = bounds->bottom;
            measuredBounds.right = bounds->right;
            messageBounds = measuredBounds;
            SetTextColor(hdc, TCLR_WHITE);
            if (savedFont) {
                SelectObject(hdc, savedFont);
            }
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00022770, 0x7d)
i32 CGameText::DrawWithFont(const char* text, HDC hdc, RECT* rect, UINT format) {
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

// @early-stop
RVA(0x00022810, 0x22a)
i32 CGameText::DrawCenteredText(
    const CString* sourceText,
    HDC hdc,
    RECT* bounds,
    i32 useMessageFont,
    i32 red,
    i32 green,
    i32 blue,
    i32 drawShadow,
    i32 shadowOffsetX,
    i32 shadowOffsetY
) {
    if (hdc == NULL) {
        return 0;
    }
    if (bounds == NULL) {
        return 0;
    }
    if (sourceText == NULL) {
        return 0;
    }
    HGDIOBJ previousFont = NULL;
    RECT textBounds = *bounds;
    if (useMessageFont == 0) {
        if (m_trainingFont) {
            previousFont = SelectObject(hdc, m_trainingFont);
        }
    } else {
        if (m_messageFont) {
            previousFont = SelectObject(hdc, m_messageFont);
        }
    }
    SetBkMode(hdc, TRANSPARENT);
    SetBkColor(hdc, RGB(0, 0, 0));
    CString text(*sourceText);
    DrawTextA(hdc, text, strlen(text), &textBounds, DT_CALCRECT | DT_WORDBREAK | DT_CENTER);
    i32 centerOffsetX = ((bounds->right - bounds->left) - (textBounds.right - textBounds.left)) / 2;
    i32 centerOffsetY = ((bounds->bottom - bounds->top) - (textBounds.bottom - textBounds.top)) / 2;
    textBounds.right += centerOffsetX;
    textBounds.left += centerOffsetX;
    textBounds.bottom += centerOffsetY;
    textBounds.top += centerOffsetY;
    if (drawShadow) {
        SetTextColor(hdc, RGB(0, 0, 0));
        textBounds.left += shadowOffsetX;
        textBounds.top += shadowOffsetY;
        textBounds.right += shadowOffsetX;
        textBounds.bottom += shadowOffsetY;
        DrawTextA(hdc, text, strlen(text), &textBounds, DT_WORDBREAK | DT_CENTER);
        textBounds.right -= shadowOffsetX;
        textBounds.left -= shadowOffsetX;
        textBounds.bottom -= shadowOffsetY;
        textBounds.top -= shadowOffsetY;
    }
    SetTextColor(hdc, RGB(red, green, blue));
    DrawTextA(hdc, text, strlen(text), &textBounds, DT_WORDBREAK | DT_CENTER);
    if (previousFont) {
        SelectObject(hdc, previousFont);
    }
    return 1;
}

RVA(0x00085f40, 0x56)
CGameText::~CGameText() {
    Reset();
}
