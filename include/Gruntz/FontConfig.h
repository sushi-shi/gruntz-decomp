#ifndef GRUNTZ_GRUNTZ_FONTCONFIG_H
#define GRUNTZ_GRUNTZ_FONTCONFIG_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_FLAGS_BEGIN(GameTextFlags, i32)
    GAME_TEXT_FLAGS_NONE = 0,
    GAME_TEXT_PREPEND = 0x02,
    GAME_TEXT_CLEAR_EXISTING = 0x04,
    GAME_TEXT_COLORED = 0x10,
    GAME_TEXT_SHADOW = 0x20
GZ_ENUM_FLAGS_END(GameTextFlags, i32)
GZ_ENUM_FLAGS_OPS(GameTextFlags)

class CGameText {
public:
    CGameText() {
        m_reserved34 = 0;
        m_inputActive = false;
        m_arialFont = NULL;
        m_trainingFont = NULL;
    }

    CPtrList m_lines;
    i32 Initialize(i32 messageHoldMs, i32 crowdedMessageHoldMs);
    void ClearMessages();
    void Reset();
    i32 AddMessage(const char* str, GZ_ENUM_PARAM(GameTextFlags, i32) flags, i32 colorTint);
    void AdvanceMessageTimer(i32 delta);

    i32 HandleInputChar(i32 charCode, i32 keyData);

    RVA(0x00020ef0, 0x20)
    CString GetInputText() {
        return m_inputText;
    }
    void EndInput();
    ~CGameText();

    i32 DrawTextLines(i32 count, HDC hdc, RECT* rect, UINT format);

    i32 DrawInputCaret(HDC hdc, RECT* rect);

    i32 RenderInputText(HDC hdc, i32 maxWidth, RECT* rect);
    i32 DrawWithFont(const char* text, HDC hdc, RECT* rect, UINT format);
    i32 Draw3DText(
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
    );

    CString m_inputText;
    u32 m_messageElapsedMs;
    u32 m_messageHoldMs;
    u32 m_crowdedMessageHoldMs;
    i32 m_inputElapsedMs;
    b32 m_inputActive;
    i32 m_reserved34; // set 1 with chat origin; never read
    HFONT m_arialFont;
    HFONT m_trainingFont;
    HFONT m_messageFont;
};

extern i32 g_chatTextWidth;
extern i32 g_caretBlinkMs;
extern b32 g_caretBlinkOn;
extern i32 g_lastDrawTextFormat;

struct GameTextLine {
    GameTextFlags m_flags;
    i32 m_colorTint;
    CString m_text;
};

#endif // GRUNTZ_GRUNTZ_FONTCONFIG_H
