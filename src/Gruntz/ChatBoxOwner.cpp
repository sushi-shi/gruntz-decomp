#include <rva.h>

#include <Gruntz/ChatBoxOwner.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
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
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>

#include <ddraw.h>
#include <string.h>
#include <strstrea.h>

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
        CSize screenSize = g_gameReg->m_modeSize;
        m_origin = CPoint(0, screenSize.cy - 66);
    } else if (mode == CHATBOX_WITH_LEFT_STATUSBAR) {
        CSize screenSize = g_gameReg->m_modeSize;
        m_origin = CPoint(0xa0, screenSize.cy - 66);
    }
    m_fontConfig->m_reserved34 = 1;
}

// @early-stop
RVA(0x000205c0, 0x741)
void CChatBoxOwner::HandleTextInputKey(i32 charCode, i32 keyData) {
    if (m_fontConfig->HandleInputChar(charCode, keyData) == 0) {
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

                CRezItm* source = g_gameReg->m_resourceArchive->GetRezFromPath(
                    static_cast<const char*>(qualified),
                    REZ_TAG_TXT
                );
                CButeMgr bute;
                bute.Term();
                bool parsed = bute.Parse(source, static_cast<const char*>(key));

                if (parsed) {
                    CString group = "";
                    CString code;
                    b32 enabled = false;
                    i32 count = bute.GetInt("Cheatz", "NumCheatz", 0);
                    for (i32 i = 1; i <= count; i++) {
                        group.Format("Cheat%i", i);
                        if (!bute.Exist(group, NULL)) {
                            continue;
                        }
                        code = *bute.GetString(group, "Text", &code);
                        if (code.GetLength() == 0) {
                            continue;
                        }
                        if (bute.GetInt(group, "NonCheat", 0) == 1) {
                            if (g_gameReg->m_cheatMgr->AddCheat(
                                    static_cast<const char*>(code),
                                    bute.GetInt(group, "Value", 0x807b),
                                    1
                                )) {
                                enabled++;
                            }
                        } else {
                            if (g_gameReg->m_cheatMgr->AddCheat(
                                    static_cast<const char*>(code),
                                    bute.GetInt(group, "Value", 0x807b),
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
    m_inputActive = false;
}

RVA(0x00020ef0, 0x20)
CString CFontConfig::GetInputText() {
    return m_inputText;
}

// @early-stop
RVA(0x00020f40, 0x188)
i32 CChatBoxOwner::LoadChatBoxSprite(CDDrawSurfacePair* target) {
    CChatBoxOwner* self = this;
    if (!self->m_inputActive) {
        return 1;
    }

    CDDSurface* surface = target->m_surface;
    if (!surface) {
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
        CPoint framePosition = self->m_origin;
        framePosition += CPoint(0x140, 0x20);
        frame->RenderFrame(target, framePosition.x, framePosition.y, 0);
    } else {
        CImage* frame = static_cast<CImage*>(spr->m_items.GetAt(spr->m_minIndex));
        if (!frame) {
            return 0;
        }
        CPoint framePosition = self->m_origin;
        framePosition += CPoint(0xf0, 0x20);
        frame->RenderFrame(target, framePosition.x, framePosition.y, 0);
    }

    HDC hdc = NULL;
    surface->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return 1;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0);
    SetBkColor(hdc, 0);

    RECT rect;
    if (self->m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
        SetRect(
            &rect,
            self->m_origin.x + 0x4c,
            self->m_origin.y + 0x2b,
            self->m_origin.x + 0x267,
            self->m_origin.y + 0x37
        );
        self->m_fontConfig->RenderInputText(hdc, 0x21b, &rect);
    } else {
        SetRect(
            &rect,
            self->m_origin.x + 0x4c,
            self->m_origin.y + 0x2b,
            self->m_origin.x + 0x1c7,
            self->m_origin.y + 0x37
        );
        self->m_fontConfig->RenderInputText(hdc, 0x17b, &rect);
    }
    surface->m_ddSurface->ReleaseDC(hdc);
    return 1;
}
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
                || (x > m_origin.x + 0x40 && x < m_origin.x + 0x1e0 && y >= ModeSize().cy - 0x20)) {
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

RVA_COMPGEN(0x000212e0, 0x1e, ??_GzPTree@@MAEPAXI@Z)

RVA_COMPGEN(0x00021310, 0x70, ??1zPTree@@MAE@XZ)
RVA_COMPGEN(0x00021570, 0x70, ??1?$zSymTab@V?$zSymTab@VCSymTabItem@CButeMgr@@@@@@UAE@XZ)
RVA_COMPGEN(0x00021600, 0x8, ??_EzPTree@@O7AEPAXI@Z)
