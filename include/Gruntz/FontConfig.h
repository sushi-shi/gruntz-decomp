// FontConfig.h - CFontConfig, the GDI font-configuration object (a CPtrList of
// FontItem*). Promoted from FontConfig.cpp so the reduced per-TU views (Refresh/
// Scroll sinks) can fold onto it. Only the load-bearing member offsets are modeled.
#ifndef GRUNTZ_GRUNTZ_FONTCONFIG_H
#define GRUNTZ_GRUNTZ_FONTCONFIG_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

SIZE(CFontConfig, 0x44);
class CFontConfig : public CPtrList {
public:
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
    virtual ~CFontConfig() OVERRIDE;
    i32 winapi_022360_DrawTextA_SelectObject_SetTextColor(i32, i32, i32, i32);

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
    i32 RenderInputText(HDC hdc, i32 maxWidth, RECT* rect); // 0x00022160
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

#endif // GRUNTZ_GRUNTZ_FONTCONFIG_H
