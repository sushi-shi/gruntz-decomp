#include <rva.h>

#include <Gruntz/ChatBoxOwner.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Rez/RezTypeTag.h>

#include <ddraw.h>
#include <string.h>
#include <strstrea.h>

// The zero-store scheduled between the Lookup arg pushes and the call is the
// inline wrapper's first statement (docs/patterns/
// out-param-reset-between-arg-setup-and-call-is-in-the-helper.md).
inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* ob = NULL;
    map.Lookup(name, ob);
    return static_cast<CDDrawWorker*>(ob);
}

RVA(0x000204e0, 0x19)
i32 CChatBoxOwner::Attach(CDDrawSurfaceMgr* world, CFontConfig* host) {
    m_world = world;
    m_fontConfig = host;
    return m_attached = true;
}

RVA(0x00020510, 0x8)
void CChatBoxOwner::Deactivate() {
    m_attached = false;
}

RVA(0x00020530, 0x61)
void CChatBoxOwner::Configure(ChatBoxLayout mode) {
    m_mode = mode;

    if (mode == CHATBOX_WITH_RIGHT_STATUSBAR || mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
        m_originX = 0;
        tagSIZE screenSize = g_gameReg->m_modeSize;
        m_originY = screenSize.cy - 66;
    } else if (mode == CHATBOX_WITH_LEFT_STATUSBAR) {
        m_originX = 0xa0;
        tagSIZE screenSize = g_gameReg->m_modeSize;
        m_originY = screenSize.cy - 66;
    }
    m_fontConfig->m_reserved34 = 1;
}

// @early-stop
// The 26-state unwind topology and teardown targets agree. The remaining local
// layout reserves `sub esp,0x140` against retail's 0x13c and materializes the
// zPtrColl cleanup states differently. The call set also cuts the stream chain
// oppositely: one extra out-of-line istrstream ctor here versus retail's
// out-of-line ostrstream::rdbuf + streambuf::out_waiting pair. See
// docs/patterns/frame-size-mismatch-dominates-the-40-65-band.md.
RVA(0x000205c0, 0x741)
void CChatBoxOwner::ProcessCheatInput(i32 a, i32 b) {
    if (m_fontConfig->TypeChar(a, b) == 0) {
        return;
    }

    if (g_gameReg->m_curState->Update() == GAMESTATE_MULTI) {
        char* input = const_cast<char*>(static_cast<const char*>(m_fontConfig->GetInputText()));
        static_cast<CMulti*>(g_gameReg->m_curState)->BroadcastChatLine(input, 1, 1, NULL);
    } else {
        if (_strcmpi(m_fontConfig->GetInputText().Left(17), "Enable Cheatzfile") == 0) {
            CString args = m_fontConfig->GetInputText();
            args = args.Right(args.GetLength() - 18);
            i32 split = args.Find(' ');
            if (split != -1) {
                CString resourceName = args.Left(split);
                CString key = args.Right(args.GetLength() - split - 1);
                CString qualified;
                qualified.Format(
                    "STATEZ_CREDITZ_PALETTEZ_%s",
                    static_cast<const char*>(resourceName)
                );

                CParseSource* source = g_gameReg->m_symParser->ResolveQualified(
                    static_cast<const char*>(qualified),
                    REZ_TAG_TXT
                );
                CButeMgr bute;
                bute.Term();
                bool parsed;
                if (source == NULL) {
                    parsed = false;
                } else {
                    char* encoded = source->BeginParse();
                    u32 length = source->m_length;
                    istrstream* inputStream = new istrstream(encoded, length);
                    bute.m_crypt.InitKey(static_cast<const char*>(key));
                    char* decoded = new char[length];
                    ostrstream* outputStream = new ostrstream(decoded, length, 2);
                    bute.m_crypt.Decode(inputStream, outputStream);
                    istrstream* parseStream = new istrstream(decoded, outputStream->pcount());
                    delete inputStream;
                    delete outputStream;
                    source->EndParse();

                    bute.Init();
                    bute.m_tree.Reset();
                    // Retail expands Reset() only for m_tree and CALLS the out-of-line
                    // copy (0x212a0) for the other two.
                    bute.m_tree48.ResetCopy();
                    bute.m_tree74.ResetCopy();
                    bute.m_stream = parseStream;
                    parsed = true;
                    if (!bute.ParseGroup()) {
                        bute.m_parseFailed = true;
                        parsed = false;
                    }
                    delete parseStream;
                    delete[] decoded;
                }

                if (parsed) {
                    CString group = "";
                    CString code;
                    i32 enabled = 0;
                    i32 count = bute.GetIntDef("Cheatz", "NumCheatz", 0);
                    for (i32 i = 1; i <= count; i++) {
                        group.Format("Cheat%i", i);
                        const char* groupName = static_cast<const char*>(group);
                        // key == NULL is Exists()'s "the tag itself is present" form
                        // (0x171a60 returns true before the key lookup).
                        if (!bute.Exists(groupName, NULL)) {
                            continue;
                        }
                        code = *bute.GetStringDef(groupName, "Text", &code);
                        if (code.GetLength() == 0) {
                            continue;
                        }
                        if (bute.GetIntDef(groupName, "NonCheat", 0) == 1) {
                            if (g_gameReg->m_cheatMgr->AddCheat(
                                    static_cast<const char*>(code),
                                    bute.GetIntDef(groupName, "Value", 0x807b),
                                    1
                                )) {
                                enabled++;
                            }
                        } else {
                            if (g_gameReg->m_cheatMgr->AddCheat(
                                    static_cast<const char*>(code),
                                    bute.GetIntDef(groupName, "Value", 0x807b),
                                    0
                                )) {
                                enabled++;
                            }
                        }
                    }
                    if (enabled > 0) {
                        code.Format(
                            "Congratulations!  You have just enabled %d new cheats!\n",
                            enabled
                        );
                        g_gameReg->AppendChatMessage(
                            const_cast<char*>(static_cast<const char*>(code))
                        );
                    }
                }
            }
        } else {
            g_gameReg->CheatMgr()->CheckCode(m_fontConfig->GetInputText());
        }
    }
    m_fontConfig->EndInput();
    m_inputActive = 0;
}

RVA(0x00020ef0, 0x20)
CString CFontConfig::GetInputText() {
    return m_inputText;
}

// The inline accessor ProcessCheatInput above reaches m_cheatMgr through; this TU
// wins the COMDAT, so retail's copy is the 4-byte `mov eax,[ecx+0x44]; ret` here.

// @early-stop
RVA(0x00020f40, 0x188)
i32 CChatBoxOwner::LoadChatBoxSprite(CDDrawSurfacePair* target) {
    CChatBoxOwner* self = this;
    if (!self->m_inputActive) {
        return 1;
    }

    CDDSurface* host = target->m_surface;
    if (!host) {
        return 0;
    }

    CDDrawWorker* spr =
        LookupWorker(self->m_world->m_imageRegistry->m_workersByName, "GAME_CHATBOX");
    if (!spr) {
        return 0;
    }

    if (self->m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
        CImage* frame = static_cast<CImage*>(spr->m_items.GetAt(spr->m_maxIndex));
        if (!frame) {
            return 0;
        }
        frame->RenderFrame(target, self->m_originX + 0x140, self->m_originY + 0x20, 0);
    } else {
        CImage* frame = static_cast<CImage*>(spr->m_items.GetAt(spr->m_minIndex));
        if (!frame) {
            return 0;
        }
        frame->RenderFrame(target, self->m_originX + 0xf0, self->m_originY + 0x20, 0);
    }

    HDC hdc = NULL;
    host->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return 1;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0);
    SetBkColor(hdc, 0);

    RECT rect;
    if (self->m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
        rect.left = self->m_originX + 0x4c;
        rect.right = self->m_originX + 0x267;
        rect.top = self->m_originY + 0x2b;
        rect.bottom = self->m_originY + 0x37;
        self->m_fontConfig->RenderInputText(hdc, 0x21b, &rect);
    } else {
        rect.left = self->m_originX + 0x4c;
        rect.right = self->m_originX + 0x1c7;
        rect.top = self->m_originY + 0x2b;
        rect.bottom = self->m_originY + 0x37;
        self->m_fontConfig->RenderInputText(hdc, 0x17b, &rect);
    }
    host->m_ddSurface->ReleaseDC(hdc);
    return 1;
}
// Retail reproduces the whole 8-byte tagSIZE at every `.cy` use (an 8-byte frame slot
// whose `.cx` half is a dead store), so the size arrives as an rvalue, not as one local.
static __inline tagSIZE ModeSize() {
    return g_gameReg->m_modeSize;
}

RVA(0x00021140, 0xda)
i32 CChatBoxOwner::HitTest(i32 x, i32 y) {
    if (m_inputActive) {
        if (m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
            if ((x < 0x40 && y >= ModeSize().cy - 0x40)
                || (x > 0x40 && y >= ModeSize().cy - 0x20)) {
                return 1;
            }
        } else {
            if ((x < 0x40 && y >= ModeSize().cy - 0x40)
                || (x > m_originX + 0x40 && x < m_originX + 0x1e0 && y >= ModeSize().cy - 0x20)) {
                return 1;
            }
        }
    }
    return 0;
}

// @identity-TODO ChatBoxOwnerReturnTrue - the surviving incremental-link thunk
// proves external linkage, and `ret 4` proves one callee-popped dword. No caller,
// address-taker, or operand survives to recover the original semantic identity.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00021260, 0x8)
i32 __stdcall ChatBoxOwnerReturnTrue(i32) {
    return 1;
}

// 0x212a0 IS `zPTree::Reset()` emitted out of line: its body is exactly the
// inline (ClearRecursive(0) + the three zero stores at +0x18/+0x28/+0x14), and
// its only caller is this TU's ProcessCheatInput, which calls it for
// m_tree48/m_tree74 after expanding the same inline for m_tree.
RVA(0x000212a0, 0x21)
void zPTree::ResetCopy() {
    Reset();
}

// The zPTree/CBSecStream/CButeMgr teardown COMDATs of this TU's `CButeMgr bute;`
// stack local: retail expands ~CButeMgr on ProcessCheatInput's normal path and
// leaves these out-of-line copies for its unwind funclets - the only callers of
// 0x213c0 / 0x21570 are the EH thunks in this TU's 0x5d93xx-0x5d95xx band.
RVA_COMPGEN(0x000212e0, 0x1e, ??_GzPTree@@UAEPAXI@Z)

RVA_COMPGEN(0x00021310, 0x70, ??1zPTree@@UAE@XZ)
RVA_COMPGEN(0x00021570, 0x70, ??1CBSecStream@@UAE@XZ)
RVA_COMPGEN(0x00021600, 0x8, ??_EzPTree@@W7AEPAXI@Z)
