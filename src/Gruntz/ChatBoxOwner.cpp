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
#include <EmptyString.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <strstrea.h>

#include <ddraw.h>
#include <string.h>

RVA(0x000204e0, 0x19)
i32 CChatBoxOwner::Attach(CDDrawSurfaceMgr* world, CFontConfig* host) {
    m_world = world;
    m_fontConfig = host;
    return m_attached = 1;
}

RVA(0x00020510, 0x8)
void CChatBoxOwner::Deactivate() {
    m_attached = 0;
}

// @early-stop
RVA(0x00020530, 0x61)
void CChatBoxOwner::Configure(i32 mode) {
    m_mode = mode;

    if (mode == 1 || mode == 3) {
        m_originX = 0;
        m_originY = g_gameReg->m_modeH - 66;
    } else if (mode == 2) {
        m_originX = 0xa0;
        m_originY = g_gameReg->m_modeH - 66;
    }
    m_fontConfig->m_reserved34 = 1;
}

// @early-stop
RVA(0x00021140, 0xda)
i32 CChatBoxOwner::HitTest(i32 x, i32 y) {
    if (m_inputActive) {

        if (m_mode == 3) {
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
        if (x <= m_originX + 0x40) {
            return 0;
        }
        if (x >= m_originX + 0x1e0) {
            return 0;
        }
        if (y < g_gameReg->m_modeH - 0x20) {
            return 0;
        }
        return 1;
    }
    return 0;
}

RVA(0x000205c0, 0x741)
void CChatBoxOwner::ProcessCheatInput(i32 a, i32 b) {
    if (m_fontConfig->TypeChar(a, b) == 0) {
        return;
    }

    if (g_gameReg->m_curState->Update() == GAMESTATE_NONE) {
        CString input = m_fontConfig->GetInputText();
        static_cast<CMulti*>(g_gameReg->m_curState)
            ->BroadcastChatLine(const_cast<char*>(static_cast<const char*>(input)), 1, 1, 0);
    } else {
        CString input = m_fontConfig->GetInputText();
        if (_strcmpi(input.Left(17), "Enable Cheatzfile") == 0) {
            CString args = input.Right(input.GetLength() - 18);
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
                    'TXT'
                );
                CButeMgr bute;
                bool parsed = false;
                if (source != 0) {
                    char* encoded = source->BeginParse();
                    u32 length = source->m_length;
                    istrstream* inputStream = new istrstream(encoded, length);
                    Blowfish_InitKey(static_cast<const char*>(key));
                    char* decoded = new char[length];
                    ostrstream* outputStream = new ostrstream(decoded, length, 2);
                    CButeTail cryptTail;
                    cryptTail.Decode(inputStream, outputStream);
                    istrstream* parseStream =
                        new istrstream(decoded, outputStream->rdbuf()->out_waiting());
                    delete inputStream;
                    delete outputStream;
                    source->EndParse();

                    bute.Init();
                    bute.m_tree.Reset();
                    bute.m_tree48.Reset();
                    bute.m_tree74.Reset();
                    bute.m_stream = parseStream;
                    parsed = bute.ParseGroup();
                    delete parseStream;
                    delete[] decoded;
                }

                if (parsed) {
                    CString empty(g_emptyString);
                    CString code;
                    i32 enabled = 0;
                    i32 count = bute.GetIntDef("Cheatz", "NumCheatz", 0);
                    for (i32 i = 1; i <= count; i++) {
                        CString group;
                        group.Format("Cheat%i", i);
                        const char* groupName = static_cast<const char*>(group);
                        if (!bute.Exists(groupName, "Text")) {
                            continue;
                        }
                        code = *bute.GetStringDef(groupName, "Text", &empty);
                        if (code.GetLength() == 0) {
                            continue;
                        }
                        i32 nonCheat = bute.GetIntDef(groupName, "NonCheat", 0);
                        i32 value = bute.GetIntDef(groupName, "Value", 0x807b);
                        if (g_gameReg->m_cheatMgr->AddCheat(
                                static_cast<const char*>(code),
                                value,
                                nonCheat == 1 ? 1 : 0
                            )) {
                            enabled++;
                        }
                    }
                    if (enabled > 0) {
                        CString message;
                        message.Format(
                            "Congratulations!  You have just enabled %d new cheats!",
                            enabled
                        );
                        g_gameReg->AppendChatMessage(
                            const_cast<char*>(static_cast<const char*>(message))
                        );
                    }
                }
            }
        } else {
            g_gameReg->m_cheatMgr->CheckCode(m_fontConfig->GetInputText());
        }
    }
    m_fontConfig->EndInput();
    m_inputActive = 0;
}

RVA(0x00020ef0, 0x20)
CString CFontConfig::GetInputText() {
    return m_inputText;
}

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

    CDDrawWorker* spr = 0;
    CObject* sprOb = 0;
    self->m_world->m_imageRegistry->m_10map.Lookup("GAME_CHATBOX", sprOb);
    spr = static_cast<CDDrawWorker*>(sprOb);
    if (!spr) {
        return 0;
    }

    if (self->m_mode == 3) {
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

    HDC hdc = 0;
    host->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return 1;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0);
    SetBkColor(hdc, 0);

    RECT rect;
    if (self->m_mode == 3) {
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
