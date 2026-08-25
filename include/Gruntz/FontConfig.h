#ifndef GRUNTZ_GRUNTZ_FONTCONFIG_H
#define GRUNTZ_GRUNTZ_FONTCONFIG_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

// Per-line behavior stored in FontItem::flags. AddItem consumes the queue-order
// bits; DrawTextLines consumes the rendering bits.
GZ_ENUM_FLAGS_BEGIN(FontItemFlags, i32)
    FONT_ITEM_FLAGS_NONE = 0,
    FONT_ITEM_PREPEND = 0x02,
    FONT_ITEM_CLEAR_EXISTING = 0x04,
    FONT_ITEM_COLORED = 0x10,
    FONT_ITEM_SHADOW = 0x20
GZ_ENUM_FLAGS_END(FontItemFlags, i32)
GZ_ENUM_FLAGS_OPS(FontItemFlags)

class CFontConfig {
public:
    CFontConfig() {
        m_reserved34 = 0;
        m_inputActive = 0;
        m_arialFont = NULL;
        m_trainingFont = NULL;
    }

    CPtrList m_list;
    i32 LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold);
    void FreeNodes();
    void Reset();
    i32 AddItem(const char* str, GZ_ENUM_PARAM(FontItemFlags, i32) flags, i32 payload);
    void Scroll(i32 delta);

    i32 HandleInputChar(i32 charCode, i32 keyData);

    CString GetInputText();
    void EndInput();
    ~CFontConfig();

    i32 DrawTextLines(i32 count, HDC hdc, RECT* rect, UINT format);

    i32 MeasureLabel(HDC hdc, RECT* rect);

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
    u32 m_scrollOffset;
    u32 m_lowScrollThreshold;
    u32 m_highScrollThreshold;
    i32 m_inputScrollTotal;
    i32 m_inputActive;
    i32 m_reserved34; // set 1 with chat origin; never read
    HFONT m_arialFont;
    HFONT m_trainingFont;
    HFONT m_messageFont;
};

extern i32 g_chatTextWidth;
extern i32 g_caretBlinkMs;
extern i32 g_caretBlinkOn;
extern i32 g_lastDrawTextFormat;

struct FontItem {
    FontItemFlags flags;
    i32 payload;
    CString name;
};

#endif // GRUNTZ_GRUNTZ_FONTCONFIG_H
