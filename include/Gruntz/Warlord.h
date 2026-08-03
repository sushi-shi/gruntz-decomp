#ifndef GRUNTZ_CWARLORD_H
#define GRUNTZ_CWARLORD_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

extern "C" u32 g_engineFrameDelta;

extern "C" u32 g_frameTime;

class CWarlord : public CUserLogic, public CWapX {
public:
    RVA(0x00043670, 0xc20)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId a3, CGameObject* obj)
        OVERRIDE {

        char buf[0x80];
        char hdr[0x80];

        if (CUserLogic::SerializeMove(ar, mode, a3, obj) == 0) {
            return 0;
        }
        if (ar == 0) {

            goto fail;
        }

        switch (mode) {
            case SERIAL_LOAD: {
                ar->Read(hdr, 0x80);
                ar->Read(m_blob, 0x10);
                m_gameObject = obj;
                m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
                m_animWorker = obj->m_animWorker;
                if (strlen(hdr) == 0) {
                    m_value = 0;
                } else {
                    CMapStringToPtr* map = &m_animWorker->m_ownerCtx->m_animRegistry->m_animations;
                    void* v = 0;
                    map->Lookup(hdr, v);
                    m_value = static_cast<CAniElement*>(v);
                }
                break;
            }
            case SERIAL_SAVE: {
                memset(buf, 0, sizeof(buf));
                if (m_value != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(
                            m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value)
                        )
                    );
                }
                ar->Write(buf, 0x80);
                ar->Write(m_blob, 0x10);
                break;
            }
        }

        switch (mode) {
            case SERIAL_SAVE: {
                CDDrawSurfaceMgr* world = m_animWorker->m_ownerCtx;
                if (world == 0) {
                    goto fail;
                }
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                strcpy(buf, static_cast<const char*>(m_warlordName));
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_idleAnims[0] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[0]))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_idleAnims[1] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[1]))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_idleAnims[2] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[2]))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_idleAnims[3] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[3]))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_battlecryAnims[0] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(
                            world->m_animRegistry->KeyOfValue(m_battlecryAnims[0])
                        )
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_battlecryAnims[1] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(
                            world->m_animRegistry->KeyOfValue(m_battlecryAnims[1])
                        )
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_battlecryAnims[2] != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(
                            world->m_animRegistry->KeyOfValue(m_battlecryAnims[2])
                        )
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_animJoy != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animJoy))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_animDeath != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animDeath))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_animMoving != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animMoving))
                    );
                }
                ar->Write(buf, 0x80);
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_animPanic != 0) {
                    strcpy(
                        buf,
                        static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animPanic))
                    );
                }
                ar->Write(buf, 0x80);
                ar->Write(&m_deathStarted, 4);
                ar->Write(&m_ownerTag, 4);
                break;
            }
            case SERIAL_LOAD: {
                CDDrawSurfaceMgr* world = m_animWorker->m_ownerCtx;
                if (world == 0) {
                    return 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                m_warlordName = buf;

                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_idleAnims[0] = static_cast<CAniElement*>(v);
                } else {
                    m_idleAnims[0] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_idleAnims[1] = static_cast<CAniElement*>(v);
                } else {
                    m_idleAnims[1] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_idleAnims[2] = static_cast<CAniElement*>(v);
                } else {
                    m_idleAnims[2] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_idleAnims[3] = static_cast<CAniElement*>(v);
                } else {
                    m_idleAnims[3] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_battlecryAnims[0] = static_cast<CAniElement*>(v);
                } else {
                    m_battlecryAnims[0] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_battlecryAnims[1] = static_cast<CAniElement*>(v);
                } else {
                    m_battlecryAnims[1] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_battlecryAnims[2] = static_cast<CAniElement*>(v);
                } else {
                    m_battlecryAnims[2] = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_animJoy = static_cast<CAniElement*>(v);
                } else {
                    m_animJoy = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_animDeath = static_cast<CAniElement*>(v);
                } else {
                    m_animDeath = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_animMoving = static_cast<CAniElement*>(v);
                } else {
                    m_animMoving = 0;
                }
                g_serialCounter++;
                ar->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* v = 0;
                    world->m_animRegistry->m_animations.Lookup(buf, v);
                    m_animPanic = static_cast<CAniElement*>(v);
                } else {
                    m_animPanic = 0;
                }
                ar->Read(&m_deathStarted, 4);
                ar->Read(&m_ownerTag, 4);
                break;
            }
            case SERIAL_POSTLOAD: {

                CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(
                    g_gameReg->m_options[m_object->m_smarts].m_colorIndex,
                    0
                );
                if (sel == 0) {
                    sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                }

                CWwdGameObjectA* sprite = m_object;
                sprite->m_drawActive = 1;
                sprite->m_drawFillCmd = SHADE_PAL_16;
                sprite->m_drawFillArg = sel;
                break;
            }
        }

        {
            switch (mode) {
                case SERIAL_LOAD:
                    ar->Read(&m_cooldownStamp, sizeof(m_cooldownStamp));
                    ar->Read(&m_cooldownWindow, sizeof(m_cooldownWindow));
                    break;
                case SERIAL_SAVE:
                    ar->Write(&m_cooldownStamp, sizeof(m_cooldownStamp));
                    ar->Write(&m_cooldownWindow, sizeof(m_cooldownWindow));
                    break;
            }
            switch (mode) {
                case SERIAL_LOAD:
                    ar->Read(&m_timer2Stamp, sizeof(m_timer2Stamp));
                    ar->Read(&m_timer2Window, sizeof(m_timer2Window));
                    break;
                case SERIAL_SAVE:
                    ar->Write(&m_timer2Stamp, sizeof(m_timer2Stamp));
                    ar->Write(&m_timer2Window, sizeof(m_timer2Window));
                    break;
            }
        }
        return 1;
    fail:
        return 0;
    }
    RVA(0x000107a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARLORD;
    }

public:
    CWarlord() {}
    CWarlord(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 RearmMoving();

    i32 RearmMoving2();

    i32 AdvanceMovingAnim();

    i32 LoadAttributes();
    i32 LoadAttributes2();

    i32 BuildFortSplashParticles();

    i32 NotifyFortUnderAttack();

    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    i32 RaiseBattleAlert();
    i32 ResolveIdleAnimation();
    i32 ResolveBattlecryAnimation();

    CString m_warlordName;

    CAniElement* m_idleAnims[4];
    CAniElement* m_battlecryAnims[3];
    CAniElement* m_animJoy;
    CAniElement* m_animDeath;
    CAniElement* m_animMoving;
    CAniElement* m_animPanic;
    char m_pad84[0x88 - 0x84];

    i64 m_cooldownStamp;
    i64 m_cooldownWindow;
    i64 m_timer2Stamp;
    i64 m_timer2Window;
    i32 m_deathStarted;

    i32 m_ownerTag;
};
SIZE(0xb0);

#endif // GRUNTZ_CWARLORD_H
