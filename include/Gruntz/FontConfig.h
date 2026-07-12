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

    CString m_inputText;       // +0x1c  scratch input string
    u32 m_scrollOffset;        // +0x20  running offset (unsigned: thresholds compare jb)
    u32 m_lowScrollThreshold;  // +0x24  threshold used for <=3 items
    u32 m_highScrollThreshold; // +0x28  threshold used for >3 items
    i32 m_inputScrollTotal;    // +0x2c  accumulated scroll while input is active
    i32 m_inputActive;         // +0x30  input accumulation flag
    char m_pad34[4];           // +0x34
    HFONT m_arialFont;         // +0x38  the ARIAL UI font
    HFONT m_trainingFont;      // +0x3c  the TrainingFont
    HFONT m_messageFont;       // +0x40  the MessageFont
};

#endif // GRUNTZ_GRUNTZ_FONTCONFIG_H
