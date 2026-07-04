// ChatBoxOwner.cpp - the on-screen chat/text-box owner page (C:\Proj\Gruntz):
// place/clear/configure/hit-test/render helpers. The box origin comes from the
// active viewport (g_gameReg->m_modeW/m_90, the viewport X/Y). Only offsets / code
// bytes are load-bearing; helpers are reloc-masked externals.
#include <rva.h>

#include <ComDefs.h> // STDMETHOD - the DDRAW IDirectDrawSurface COM interface macros
#include <Gruntz/GameRegistry.h>
#include <Gruntz/ChatBoxOwner.h>

DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// Engine views the sprite renderer (LoadChatBoxSprite, 0x20f40) reaches through.
// Modeled minimally; every call/datum through them is reloc-masked.

// The looked-up "GAME_CHATBOX" sprite set: a frame-entry array gated by two indices.
struct CChatBoxFrame {
    char m_pad00[0x14];
    void** m_14; // +0x14  frame-entry array
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame index (mode != 3)
    i32 m_68; // +0x68  frame index (mode == 3)
};
// The name->sprite hash embedded at the registry's +0x10 (Lookup 0x1b8008,
// __thiscall; writes the found set to *out).
struct CChatBoxHash {
    void Lookup(char* szName, void** out); // 0x1b8008
};
struct CChatBoxRegistry { // m_18->m_10 points here
    char m_pad00[0x10];
    CChatBoxHash m_10; // +0x10
};
struct CChatBoxRegRoot { // m_18 points here
    char m_pad00[0x10];
    CChatBoxRegistry* m_10; // +0x10
};
// arg1->m_2c->m_8: the game's IDirectDrawSurface (DDRAW COM). GetDC is slot 17
// (+0x44), ReleaseDC slot 26 (+0x68); both __stdcall with the surface as the
// hidden `this`. A local SDK-named interface (real DX6 slot names) so
// `surf->GetDC(&hdc)` lowers to the same `push &hdc; push surf; mov reg,[surf];
// call [reg+slot]` the manual vtbl-struct view did; pointer-only -> no vtable
// emitted in this TU.
struct IDirectDrawSurfaceZ {
    STDMETHOD(QueryInterface)() PURE;        // slot 0
    STDMETHOD_(u32, AddRef)() PURE;          // slot 1
    STDMETHOD_(u32, Release)() PURE;         // slot 2
    STDMETHOD(AddAttachedSurface)() PURE;    // slot 3
    STDMETHOD(AddOverlayDirtyRect)() PURE;   // slot 4
    STDMETHOD(Blt)() PURE;                   // slot 5
    STDMETHOD(BltBatch)() PURE;              // slot 6
    STDMETHOD(BltFast)() PURE;               // slot 7
    STDMETHOD(DeleteAttachedSurface)() PURE; // slot 8
    STDMETHOD(EnumAttachedSurfaces)() PURE;  // slot 9
    STDMETHOD(EnumOverlayZOrders)() PURE;    // slot 10
    STDMETHOD(Flip)() PURE;                  // slot 11
    STDMETHOD(GetAttachedSurface)() PURE;    // slot 12
    STDMETHOD(GetBltStatus)() PURE;          // slot 13
    STDMETHOD(GetCaps)() PURE;               // slot 14
    STDMETHOD(GetClipper)() PURE;            // slot 15
    STDMETHOD(GetColorKey)() PURE;           // slot 16
    STDMETHOD(GetDC)(HDC* phdc) PURE;        // slot 17 == +0x44
    STDMETHOD(GetFlipStatus)() PURE;         // slot 18
    STDMETHOD(GetOverlayPosition)() PURE;    // slot 19
    STDMETHOD(GetPalette)() PURE;            // slot 20
    STDMETHOD(GetPixelFormat)() PURE;        // slot 21
    STDMETHOD(GetSurfaceDesc)() PURE;        // slot 22
    STDMETHOD(Initialize)() PURE;            // slot 23
    STDMETHOD(IsLost)() PURE;                // slot 24
    STDMETHOD(Lock)() PURE;                  // slot 25
    STDMETHOD(ReleaseDC)(HDC hdc) PURE;      // slot 26 == +0x68
    STDMETHOD(Restore)() PURE;               // slot 27
    STDMETHOD(SetClipper)() PURE;            // slot 28
    STDMETHOD(SetColorKey)() PURE;           // slot 29
    STDMETHOD(SetOverlayPosition)() PURE;    // slot 30
    STDMETHOD(SetPalette)() PURE;            // slot 31
    STDMETHOD(Unlock)() PURE;                // slot 32
    STDMETHOD(UpdateOverlay)() PURE;         // slot 33
    STDMETHOD(UpdateOverlayDisplay)() PURE;  // slot 34
    STDMETHOD(UpdateOverlayZOrder)() PURE;   // slot 35
};
struct CChatBoxDcHost { // arg1->m_2c points here
    char m_pad00[0x8];
    IDirectDrawSurfaceZ* m_8; // +0x08
};
struct CChatBoxCtx { // arg1 points here
    char m_pad00[0x2c];
    CChatBoxDcHost* m_2c; // +0x2c
};
// 0x153790 (__stdcall): renders the chatbox frame into the looked-up set.
void __stdcall RenderChatBoxFrame(i32 ctx, void* a, void* b, i32 z);

// Attach - latch the source registry root + text host, raise active.
// @early-stop
// constant-materialization wall: retail emits `mov eax,1; ...; mov [m_c],eax`
// (1 register-materialized, stored last) where our cl folds the direct immediate
// `mov [m_c],1`; logic + offsets exact, residual is the last store's form/order.
RVA(0x000204e0, 0x19)
void CChatBoxOwner::Attach(void* reg, CChatBoxTextHost* host) {
    m_18 = (CChatBoxRegRoot*)reg;
    m_14 = host;
    m_c = 1;
}

// Deactivate - lower the active flag.
RVA(0x00020510, 0x8)
void CChatBoxOwner::Deactivate() {
    m_c = 0;
}

// GetField1c - return the box's caption/key CString (m_1c) by value
// (copy-construct it into the caller's sret slot; the return slot pointer flows
// back in eax). The CString copy ctor is NAFXCW (reloc-masked).
RVA(0x00020ef0, 0x20)
CString CChatBoxOwner::GetField1c() {
    return m_1c;
}

// Configure - origin from the viewport for the given mode; mark dirty.
// @early-stop
// dead-global-read-spill wall (docs/patterns/dead-global-read-spill-dce.md): retail
// spills the unused viewport width to a dead `[esp]` slot per arm; our cl DCEs it.
RVA(0x00020530, 0x61)
void CChatBoxOwner::Configure(i32 mode) {
    m_8 = mode;
    // The dev read both viewport coords (g_gameReg->m_modeW width + m_90 height) but
    // uses only height here; retail keeps the dead width load+spill (see the marker
    // above + docs/patterns/dead-global-read-spill-dce.md).
    if (mode == 1 || mode == 3) {
        m_0 = 0;
        m_4 = g_gameReg->m_modeH - 66;
    } else if (mode == 2) {
        m_0 = 0xa0;
        m_4 = g_gameReg->m_modeH - 66;
    }
    m_14->m_34 = 1;
}

// HitTest - is screen point (x,y) over the box for the current mode.
// @early-stop
// dead-global-read-spill wall (docs/patterns/dead-global-read-spill-dce.md): retail
// keeps 4 dead viewport-width loads+spills (`mov [esp+8],reg`); our cl DCEs them all,
// freeing the registers -> the whole regalloc/code-motion diverges. Logic exact.
RVA(0x00021140, 0xda)
i32 CChatBoxOwner::HitTest(i32 x, i32 y) {
    if (!m_10) {
        return 0;
    }
    // The dev read both viewport coords (width + height) per height test but uses
    // only height; retail keeps the dead width loads+spills (see the marker above
    // + docs/patterns/dead-global-read-spill-dce.md).
    if (m_8 == 3) {
        if (x < 0x40) {
            if (y >= g_gameReg->m_modeH - 0x40) {
                return 1;
            }
        }
        if (x <= 0x40) {
            return 0;
        }
        if (y < g_gameReg->m_modeH - 0x20) {
            return 0;
        }
        return 1;
    }
    if (x < 0x40) {
        if (y >= g_gameReg->m_modeH - 0x40) {
            return 1;
        }
    }
    if (x <= m_0 + 0x40) {
        return 0;
    }
    if (x >= m_0 + 0x1e0) {
        return 0;
    }
    if (y < g_gameReg->m_modeH - 0x20) {
        return 0;
    }
    return 1;
}

// ===========================================================================
// CChatBoxOwner::ProcessCheatInput - cheat processor
// ===========================================================================
// Fired when the player submits a line in the chat box. DECODED BEHAVIOR (for the
// final sweep; see the disasm at RVA 0x205c0):
//
//   if (m_14->IsAcceptingInput() == 0) goto done;       // 0x3508 on m_14
//   mode = g_gameReg->m_curState->vtbl->GetInputMode();        // [m_2c]->[vtbl+0x10]
//   if (mode == 0x11) {                                  // a "paste/special" key
//       CString s = m_14->GetText();                     // 0x12a3
//       m_14->host->Dispatch(s, 1, 1, 0);                // 0x2243
//       goto done;
//   }
//   CString line = m_14->GetText();                      // 0x12a3
//   // case-insensitive "Enable Cheatzfile" prefix test (0x11 chars):
//   if (_strcmpi(line.Left(0x11), "Enable Cheatzfile") != 0) goto reset; // 0x11fdf0
//   CString arg  = line.Mid(0x12);                       // text after the command
//   CString name; name.Format("STATEZ_CREDITZ_PALETTEZ_%s", arg);        // 0x1b2cf5
//   CButeMgr* bm = g_gameReg->m_34->LoadBute('TXT', name);// 0x13bff0 (FourCC 'TXT')
//   if (!bm) { ok = false; goto teardown; }
//   int enabled = 0;
//   int nCheatz = bm->GetInt("Cheatz", "NumCheatz", 0);  // 0x171aa0
//   for (int i = 1; i <= nCheatz; i++) {
//       CString key; key.Format("Cheat%i", i);           // 0x1b2cf5
//       CString cheat = bm->GetString("Cheatz", key, "Text"); // 0x173180/0x171a60
//       if (cheat.IsEmpty()) continue;
//       if (bm->GetInt("NonCheat", cheat, 0) == 1)        // 0x171aa0
//           { if (g_gameReg->m_44->Apply(cheat, bm->GetInt("Value", cheat, 0x807b), 1)) enabled++; } // 0x4269
//       else
//           { if (g_gameReg->m_44->Apply(cheat, bm->GetInt("Value", cheat, 0x807b)))    enabled++; }
//   }
//   if (enabled > 0) {
//       CString msg; msg.Format("Congratulations!  You have just enabled %d new cheats!", enabled);
//       g_gameReg->ShowSystemMessage(msg);               // 0x1b54
//   }
//   // teardown: destruct ~8 CString temps in reverse EH-state order, free the
//   // two RezAlloc'd scratch buffers (0x1b9b82), reset the bute reader (0x170330).
//   reset:  m_14->ClearInput();                          // 0x167c -> 0x442b
//   done:   m_14->Refresh(); this->m_10 = 0;             // 0x25c2
//
// @early-stop
// DEFERRED to the final sweep. This is a 1857-byte CString-temp-heavy /GX body
// with ~24 cascading EH states (the [esp+0x154] state byte runs 0..0x18), MFC
// CString Format/Left/Mid temps, virtual-dispatch input-mode probes, and a
// CButeMgr cheat-file load+enumerate. Per the eh-state-numbering-base.md +
// gx-state-machine-scalar-delete-cleanup.md walls (same family as the deferred
// Font word-wrap monsters), reproducing the exact EH-state machine + the deep
// engine-collection model is a leaf-first redo - homed by RVA as a complete-intent
// placeholder rather than a half-reconstructed body that would diverge regalloc.
RVA(0x000205c0, 0x741)
void CChatBoxOwner::ProcessCheatInput(i32 a, i32 b) {
    (void)a;
    (void)b;
}

// ===========================================================================
// CChatBoxOwner::LoadChatBoxSprite - look up the "GAME_CHATBOX" sprite
// set, blit the frame for the current mode, then stamp the caption text via the
// DC source. int(BOOL) return; the m_10==0 / hdc==0 guards return 1, the
// m_2c==0 / spr==0 / frame==0 guards return 0.
// @early-stop
// scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md): logic + arg
// order + the int(BOOL) per-site epilogues all match; residual is two store
// hoist/sink permutations - retail SINKS the Lookup out-param `mov [&spr],0` past
// the arg pushes (cl hoists) and SINKS the rect[1] struct store past `push &rect`
// at a shifted esp offset (same instruction multiset, /O2-invariant), plus the
// frame guard `mov ecx,[..]; test` vs cl's `cmp [..],0` materialization. No local
// source diff closes these (hoisting rect[0] regressed 83->82%). ~83%.
RVA(0x00020f40, 0x188)
i32 CChatBoxOwner::LoadChatBoxSprite(i32 arg1) {
    CChatBoxOwner* self = this;
    if (!self->m_10) {
        return 1;
    }

    CChatBoxCtx* ctx = (CChatBoxCtx*)arg1;
    CChatBoxDcHost* host = ctx->m_2c;
    if (!host) {
        return 0;
    }

    void* spr = 0;
    self->m_18->m_10->m_10.Lookup("GAME_CHATBOX", &spr);
    if (!spr) {
        return 0;
    }

    if (self->m_8 == 3) {
        void* frame = ((CChatBoxFrame*)spr)->m_14[((CChatBoxFrame*)spr)->m_68];
        if (!frame) {
            return 0;
        }
        RenderChatBoxFrame(arg1, (void*)(self->m_0 + 0x140), (void*)(self->m_4 + 0x20), 0);
    } else {
        void* frame = ((CChatBoxFrame*)spr)->m_14[((CChatBoxFrame*)spr)->m_64];
        if (!frame) {
            return 0;
        }
        RenderChatBoxFrame(arg1, (void*)(self->m_0 + 0xf0), (void*)(self->m_4 + 0x20), 0);
    }

    HDC hdc = 0;
    host->m_8->GetDC(&hdc);
    if (!hdc) {
        return 1;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0);
    SetBkColor(hdc, 0);

    void* rect[4];
    if (self->m_8 == 3) {
        rect[0] = (void*)(self->m_0 + 0x4c);
        rect[2] = (void*)(self->m_0 + 0x267);
        rect[1] = (void*)(self->m_4 + 0x2b);
        rect[3] = (void*)(self->m_4 + 0x37);
        self->m_14->StampText(hdc, 0x21b, rect);
    } else {
        rect[0] = (void*)(self->m_0 + 0x4c);
        rect[2] = (void*)(self->m_0 + 0x1c7);
        rect[1] = (void*)(self->m_4 + 0x2b);
        rect[3] = (void*)(self->m_4 + 0x37);
        self->m_14->StampText(hdc, 0x17b, rect);
    }
    host->m_8->ReleaseDC(hdc);
    return 1;
}

// SIZE metadata for the .cpp-local engine views (CChatBoxOwner + CChatBoxTextHost
// live in ChatBoxOwner.h).
SIZE_UNKNOWN(CChatBoxCtx);
SIZE_UNKNOWN(CChatBoxDcHost);
SIZE_UNKNOWN(IDirectDrawSurfaceZ);
SIZE_UNKNOWN(CChatBoxFrame);
SIZE_UNKNOWN(CChatBoxHash);
SIZE_UNKNOWN(CChatBoxRegRoot);
SIZE_UNKNOWN(CChatBoxRegistry);
