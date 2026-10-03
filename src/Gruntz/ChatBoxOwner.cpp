#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ChatBoxOwner.h>

#include <Bute/ButeMgr.h>
#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <RectMacros.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>

#include <ddraw.h>
#include <string.h>
#include <strstrea.h>

i32 CChatBoxOwner::Attach(CDDrawSurfaceMgr* world, CFontConfig* host) {
    m_world = world;
    m_fontConfig = host;
    return m_attached = true;
}

void CChatBoxOwner::Deactivate() {
    m_attached = false;
}

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

void CChatBoxOwner::HandleTextInputKey(i32 charCode, i32 keyData) {
    if (m_fontConfig->HandleInputChar(charCode, keyData) == 0) {
        return;
    }

    if (g_gameReg->m_curState->Update() == GAMESTATE_MULTI) {
        CMulti* multi = static_cast<CMulti*>(g_gameReg->m_curState);
        const std::string input = m_fontConfig->GetInputText();
        multi->BroadcastChatLine(input.c_str(), 1, 1, NULL);
    } else {
        if (compareAsciiCaseInsensitive(sliceText(m_fontConfig->GetInputText(), 0, 17), "Enable Cheatzfile") == 0) {
            std::string args = m_fontConfig->GetInputText();
            args = rightText(args, static_cast<i32>((args).size()) - 18);
            i32 length = static_cast<i32>((args).size());
            i32 split = stringIndex((args).find(' '));
            if (split != -1) {
                std::string resourceName = sliceText(args, 0, split);
                std::string key = rightText(args, length - split - 1);
                std::string text;
                text = formatText("STATEZ_CREDITZ_PALETTEZ_%s", (resourceName).c_str());

                CRezItm* source = g_gameReg->ResourceArchive()->GetRezFromPath(
                    (text).c_str(),
                    REZ_TAG_TXT
                );
                CButeMgr bute;
                bute.Term();
                bool parsed = bute.Parse(source, (key).c_str());

                if (parsed) {
                    std::string noText = "";
                    std::string code;
                    i32 enabled = 0;
                    i32 count = bute.GetInt("Cheatz", "NumCheatz", 0);
                    for (i32 i = 1; i <= count; i++) {
                        text = formatText("Cheat%i", i);
                        if (!bute.Exist((text).c_str(), NULL)) {
                            continue;
                        }
                        code = *bute.GetString((text).c_str(), "Text", &noText);
                        if ((code).empty()) {
                            continue;
                        }
                        if (bute.GetInt((text).c_str(), "NonCheat", 0) == 1) {
                            if (g_gameReg->CheatMgr()->AddCheat(
                                    (code).c_str(),
                                    bute.GetInt((text).c_str(), "Value", 0x807b),
                                    1
                                )) {
                                enabled++;
                            }
                        } else {
                            if (g_gameReg->CheatMgr()->AddCheat(
                                    (code).c_str(),
                                    bute.GetInt((text).c_str(), "Value", 0x807b),
                                    0
                                )) {
                                enabled++;
                            }
                        }
                    }
                    if (enabled > 0) {
                        text = formatText("Congratulations!  You have just enabled %d new cheats!\n", enabled);
                        g_gameReg->AppendChatMessage(
                            (text).c_str()
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

i32 CChatBoxOwner::LoadChatBoxSprite(CDDrawSurfacePair* target) {
    CChatBoxOwner* self = this;
    if (!self->m_inputActive) {
        return 1;
    }

    CDDSurface* surface = target->GetSurface();
    if (!surface) {
        return 0;
    }

    CDDrawWorker* spr = self->m_world->FindWorker("GAME_CHATBOX");
    if (!spr) {
        return 0;
    }

    if (self->m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
        CImage* frame = DDRAW_WORKER_FRAME_AT_UNCHECKED(spr, spr->GetMaxIndex());
        if (!frame) {
            return 0;
        }
        frame->RenderFrame(target, self->m_originX + 0x140, self->m_originY + 0x20, 0);
    } else {
        CImage* frame = DDRAW_WORKER_FRAME_AT_UNCHECKED(spr, spr->GetMinIndex());
        if (!frame) {
            return 0;
        }
        frame->RenderFrame(target, self->m_originX + 0xf0, self->m_originY + 0x20, 0);
    }

    HDC hdc = NULL;
    surface->GetDirectDrawSurface()->GetDC(&hdc);
    if (!hdc) {
        return 1;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(0, 0, 0));

    if (self->m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
        CRect rect(
            self->m_originX + 0x4c,
            self->m_originY + 0x2b,
            self->m_originX + 0x267,
            self->m_originY + 0x37
        );
        self->m_fontConfig->RenderInputText(hdc, 0x21b, &rect);
    } else {
        CRect rect(
            self->m_originX + 0x4c,
            self->m_originY + 0x2b,
            self->m_originX + 0x1c7,
            self->m_originY + 0x37
        );
        self->m_fontConfig->RenderInputText(hdc, 0x17b, &rect);
    }
    surface->GetDirectDrawSurface()->ReleaseDC(hdc);
    return 1;
}

i32 CChatBoxOwner::HitTest(i32 x, i32 y) {
    if (m_inputActive) {
        if (m_mode == CHATBOX_WITH_HIDDEN_STATUSBAR) {
            if ((x < 0x40 && y >= g_gameReg->GetModeSize().cy - 0x40)
                || (x > 0x40 && y >= g_gameReg->GetModeSize().cy - 0x20)) {
                return 1;
            }
        } else {
            if ((x < 0x40 && y >= g_gameReg->GetModeSize().cy - 0x40)
                || (x > m_originX + 0x40 && x < m_originX + 0x1e0
                    && y >= g_gameReg->GetModeSize().cy - 0x20)) {
                return 1;
            }
        }
    }
    return 0;
}

i32 __stdcall ChatBoxOwnerReturnTrue(i32) {
    return 1;
}
