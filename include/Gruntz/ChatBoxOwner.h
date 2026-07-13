// ChatBoxOwner.h - the on-screen chat/text-box owner (C:\Proj\Gruntz). The page
// object that positions the chat box from the active viewport, latches the source
// registry + text host, and hit-tests the cursor against the box rectangle. Only
// offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_GRUNTZ_CHATBOXOWNER_H
#define GRUNTZ_GRUNTZ_CHATBOXOWNER_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CString, HDC

struct CSpriteFactoryHolder; // the world holder latched at m_18 (<Gruntz/GameRegistry.h>)

// The text host at m_14 IS the canonical CFontConfig (the ex-CChatBoxTextHost view is
// DISSOLVED, 2026-07-13): TypeChar/GetInputText/ClearInput already ran on it, its +0x34
// is CFontConfig::m_34 (the dirty/redraw flag Configure raises), and its "StampText"
// @0x1cd0 is CFontConfig::RenderInputText. Full def in <Gruntz/FontConfig.h> (the deref
// TUs include it); forward-declared here so this header stays lean.
class CFontConfig;

// SIZE proven by the alloc sites (operator-new ground truth): CPlay::Vfunc1 @0xc7f7f
// and CMulti::SetupMultiplayerSession @0xb583d both `push 0x1c; call RezAlloc`.
SIZE(CChatBoxOwner, 0x1c);
class CChatBoxOwner {
public:
    // Inline ctor, recovered from its retail inline expansion at CMulti::
    // SetupMultiplayerSession 0xb583d (`push 0x1c; call RezAlloc` then the seven
    // field stores in this exact order, m_8 = 1 through the pinned-1 register):
    // clear everything, mode = 1. No out-of-line ??0 exists (header-inline).
    CChatBoxOwner() {
        m_18 = 0;
        m_14 = 0;
        m_c = 0;
        m_10 = 0;
        m_0 = 0;
        m_4 = 0;
        m_8 = 1;
    }

    // Latch the world holder (the sprite/name-registry source) + text host and
    // raise the active flag.
    // RETURNS i32 (constant 1): retail 0x204e0 materializes `mov eax,1` LIVE at the
    // `ret` and CPlay::Vfunc1 (0xc7ec0) TESTs it. The old `void` decl is what produced
    // the bogus "@early-stop constant-materialization wall" on this 25-byte function -
    // it was a wrong RETURN TYPE, not codegen.
    i32 Attach(CSpriteFactoryHolder* world, CFontConfig* host);
    // Lower the active flag.
    void Deactivate(); // 0x00020510
    // Configure the box origin from the current viewport for the given mode and
    // mark the text host dirty.
    void Configure(i32 mode);
    // Hit-test a screen point against the box rectangle for the current mode.
    i32 HitTest(i32 x, i32 y);
    // (The ex-`GetField1c` @0x20ef0 was a misattribution: 0x20ef0 is
    // CFontConfig::GetInputText, the +0x14 text host's inline CString accessor whose COMDAT
    // this obj emitted. It is defined in ChatBoxOwner.cpp - its rva-order home - but
    // declared on CFontConfig, and the invented +0x1c CString member is gone with it.)
    // The chat-box cheat-code processor ("Enable Cheatzfile" command).
    void ProcessCheatInput(i32 a, i32 b);
    // Render the chat-box sprite + stamp its text for the current mode.
    i32 LoadChatBoxSprite(i32 arg1);
    // (The ex-"HitTest43e0" decl was a THUNK-ALIAS DUPLICATE: ILT 0x43e0 jmps to the
    // real HitTest @0x21140 above - CPlay's OnMouseUp probe reaches it that way. One
    // function, two source names, one of them an unresolvable symbol.)

    i32 m_0;                    // +0x00  box origin X (or 0/0xa0 by mode)
    i32 m_4;                    // +0x04  box origin Y (viewport-relative)
    i32 m_8;                    // +0x08  mode (1/2/3)
    i32 m_c;                    // +0x0c  active flag
    i32 m_10;                   // +0x10  enabled flag (hit-test gate)
    CFontConfig* m_14;          // +0x14  text host (the real CFontConfig; TypeChar /
                                //        GetInputText / ClearInput / RenderInputText run on it)
    CSpriteFactoryHolder* m_18; // +0x18  the world holder (name registry source)
};

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
