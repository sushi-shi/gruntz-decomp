// ChatBoxOwner.cpp - the on-screen chat/text-box owner page (C:\Proj\Gruntz):
// place/clear/configure/hit-test helpers. The box origin comes from the active
// viewport (g_gameReg->m_8c/m_90, the viewport X/Y). The renderer LoadChatBoxSprite
// (0x20f40) stays in src/Stub/ApiCallers.cpp at its scheduling wall. Only offsets /
// code bytes are load-bearing; helpers are reloc-masked externals.
#include <rva.h>

#include <Gruntz/CGameRegistry.h>
#include <Gruntz/ChatBoxOwner.h>

DATA(0x0064556c)
extern CGameRegistry* g_gameReg;

// Attach @0x204e0 - latch the source registry root + text host, raise active.
// @early-stop
// constant-materialization wall: retail emits `mov eax,1; ...; mov [m_c],eax`
// (1 register-materialized, stored last) where our cl folds the direct immediate
// `mov [m_c],1`; logic + offsets exact, residual is the last store's form/order.
RVA(0x000204e0, 0x19)
void CChatBoxOwner::Attach(void* reg, CChatBoxTextHost* host) {
    m_18 = reg;
    m_14 = host;
    m_c = 1;
}

// Deactivate @0x20510 - lower the active flag.
RVA(0x00020510, 0x8)
void CChatBoxOwner::Deactivate() {
    m_c = 0;
}

// GetField1c @0x20ef0 - return the box's caption/key CString (m_1c) by value
// (copy-construct it into the caller's sret slot; the return slot pointer flows
// back in eax). The CString copy ctor is NAFXCW (reloc-masked).
RVA(0x00020ef0, 0x20)
CString CChatBoxOwner::GetField1c() {
    return m_1c;
}

// Configure @0x20530 - origin from the viewport for the given mode; mark dirty.
// @early-stop
// dead-global-read-spill wall (docs/patterns/dead-global-read-spill-dce.md): retail
// spills the unused viewport width to a dead `[esp]` slot per arm; our cl DCEs it.
RVA(0x00020530, 0x61)
void CChatBoxOwner::Configure(i32 mode) {
    m_8 = mode;
    // The dev read both viewport coords (g_gameReg->m_8c width + m_90 height) but
    // uses only height here; retail keeps the dead width load+spill (see the marker
    // above + docs/patterns/dead-global-read-spill-dce.md).
    if (mode == 1 || mode == 3) {
        m_0 = 0;
        m_4 = g_gameReg->m_90 - 66;
    } else if (mode == 2) {
        m_0 = 0xa0;
        m_4 = g_gameReg->m_90 - 66;
    }
    m_14->m_34 = 1;
}

// HitTest @0x21140 - is screen point (x,y) over the box for the current mode.
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
            if (y >= g_gameReg->m_90 - 0x40) {
                return 1;
            }
        }
        if (x <= 0x40) {
            return 0;
        }
        if (y < g_gameReg->m_90 - 0x20) {
            return 0;
        }
        return 1;
    }
    if (x < 0x40) {
        if (y >= g_gameReg->m_90 - 0x40) {
            return 1;
        }
    }
    if (x <= m_0 + 0x40) {
        return 0;
    }
    if (x >= m_0 + 0x1e0) {
        return 0;
    }
    if (y < g_gameReg->m_90 - 0x20) {
        return 0;
    }
    return 1;
}

// ===========================================================================
// CChatBoxOwner::ProcessCheatInput  (0x205c0, 0x741 = 1857 B) - cheat processor
// ===========================================================================
// Fired when the player submits a line in the chat box. DECODED BEHAVIOR (for the
// final sweep; see the disasm at RVA 0x205c0):
//
//   if (m_14->IsAcceptingInput() == 0) goto done;       // 0x3508 on m_14
//   mode = g_gameReg->m_2c->vtbl->GetInputMode();        // [m_2c]->[vtbl+0x10]
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
