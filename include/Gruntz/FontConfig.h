#ifndef GRUNTZ_GRUNTZ_FONTCONFIG_H
#define GRUNTZ_GRUNTZ_FONTCONFIG_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

SIZE(CFontConfig, 0x44);
class CFontConfig {
public:
    CPtrList m_list; // +0x00 (0x1c: vptr, m_pNodeHead@+0x04, m_nCount@+0x0c) - the FontItem* list
    i32 LoadFontConfig(i32 lowScrollThreshold, i32 highScrollThreshold);
    void FreeNodes();
    void Reset();
    i32 AddItem(const char* str, i32 type, i32 data);
    void Scroll(i32 delta);
    i32 TypeChar(i32 ch, i32 a2);
    // Returns m_inputText (+0x1c) by value. An INLINE accessor: MSVC 5.0 emits a by-value
    // CString return out-of-line, so it is a COMDAT and the surviving copy is the one the
    // ChatBoxOwner obj emitted (rva 0x00020ef0, inside THAT unit's band) - which is where
    // it is DEFINED, and which is why callers reach it through ILT thunk 0x12a3.
    CString GetInputText(); // 0x00020ef0 (def: src/Gruntz/ChatBoxOwner.cpp)
    void EndInput();
    ~CFontConfig(); // 0x00085f40  NON-virtual (no vptr restamp in retail) - lets `delete` bind direct
    // 0x00022360: draw up to `count` list items as stacked text lines into hdc.
    // Trims the list to `count` (RemoveHead), then per line: optional 1px black
    // shadow (type&0x20), a color from the type&0x10 palette switch (else white),
    // a DT_CALCRECT measure, the real draw, and advances the rect down by the
    // measured height. Args proven by the SaveGame caller: (8, hdc, &rect, 0x10).
    i32 DrawTextLines(i32 count, HDC hdc, RECT* rect, UINT format); // 0x00022360

    // The GDI text/edit-control renderers, re-homed here from the FontConfig.cpp
    // `m4::DrawHost` / `m4::PwdHost` / `m4::TextHost` views (2026-07-13). The three
    // were ALREADY proven to be CFontConfig by member-layout identity (their +0x1c is
    // m_inputText, +0x38/+0x3c/+0x40 are the three cached HFONTs) - the dossier said so
    // and the views were parked as an "@identity-TODO ... fold in a follow-up". This is
    // that follow-up; ChatBoxOwner.cpp (the only external caller) already included this
    // header and called its +0x14 "the CFontConfig text host" through the view.
    //
    // Measure m_inputText into `rect` (DT_CALCRECT), clamp the used width into the
    // 0x62b434 cache, then stroke the 12px insertion caret at that offset.
    // NOTE THE ARG ORDER: the old view declared this (HDC, const char* text) and
    // conjured the rect out of the CString copy via a fake `RectSrc`. Retail is the
    // OTHER WAY ROUND - it copy-constructs THIS->m_inputText as the text and takes the
    // RECT* as arg2 (0x21f20: `add ecx,0x1c; call ??0CString@@QAE@ABV0@@Z`, then
    // `mov ecx,esi` / `mov edx,[ecx]`..`[ecx+0xc]` off arg2). The swap - not regalloc -
    // was the "~72% wall".
    i32 MeasureLabel(HDC hdc, RECT* rect); // 0x00021f20 (RenderInputText reaches it
                                           // via ILT 0x258b; the old separate
                                           // "Draw258b" decl was a phantom
                                           // duplicate of this same function)
    i32 RenderInputText(HDC hdc, i32 maxWidth, RECT* rect);               // 0x00022160
    i32 DrawWithFont(const char* text, HDC hdc, RECT* rect, UINT format); // 0x00022770
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
    ); // 0x00022810

    CString m_inputText;       // +0x1c  scratch input string
    u32 m_scrollOffset;        // +0x20  running offset (unsigned: thresholds compare jb)
    u32 m_lowScrollThreshold;  // +0x24  threshold used for <=3 items
    u32 m_highScrollThreshold; // +0x28  threshold used for >3 items
    i32 m_inputScrollTotal;    // +0x2c  accumulated scroll while input is active
    i32 m_inputActive;         // +0x30  input accumulation flag
    i32 m_34;                  // +0x34  dirty/redraw flag (CChatBoxOwner::Configure raises it)
    HFONT m_arialFont;         // +0x38  the ARIAL UI font
    HFONT m_trainingFont;      // +0x3c  the TrainingFont
    HFONT m_messageFont;       // +0x40  the MessageFont
};

extern "C" i32 g_chatTextWidth;      // 0x62b434: DT_CALCRECT-measured text width
extern "C" i32 g_caretBlinkMs;       // 0x62b438: caret blink countdown in ms
extern "C" i32 g_caretBlinkOn;       // 0x62b43c: caret blink phase
extern "C" i32 g_lastDrawTextFormat; // 0x60c7a8: last DrawTextA format flags

struct FontItem {
    i32 type;     // +0x00
    i32 data;     // +0x04
    CString name; // +0x08
    ~FontItem();  // 0x21c40  out-of-line member dtor (destroys name; add ecx,8; jmp ~CString)
};

#endif // GRUNTZ_GRUNTZ_FONTCONFIG_H
