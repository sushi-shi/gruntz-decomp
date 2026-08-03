#ifndef GRUNTZ_GRUNTZ_CINGAMEICON_H
#define GRUNTZ_GRUNTZ_CINGAMEICON_H

#include <rva.h>

#include <Mfc.h>

#include <Clock64.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/UserLogic.h>

extern "C" u32 g_frameTime;

class LeafCue;

class CInGameIcon : public CUserLogic, public CWapX {
public:
    RVA(0x00098c90, 0x382)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* obj)
        OVERRIDE {

        char chainName[0x80];

        if (ar == 0) {
            return 0;
        }
        if (CUserLogic::SerializeMove(ar, mode, typeId, obj) == 0) {
            return 0;
        }

        switch (mode) {
            case SERIAL_LOAD: {
                ar->Read(chainName, 0x80);
                ar->Read(m_blob, 0x10);
                m_gameObject = obj;
                m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
                m_animWorker = obj->m_animWorker;
                if (strlen(chainName) == 0) {
                    m_value = 0;
                } else {
                    void* val = 0;
                    m_animWorker->m_ownerCtx->m_animRegistry->m_animations.Lookup(chainName, val);
                    m_value = static_cast<CAniElement*>(val);
                }
                break;
            }
            case SERIAL_SAVE: {
                memset(chainName, 0, sizeof(chainName));
                if (m_value != 0) {
                    CString nm = m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value);
                    strcpy(chainName, static_cast<const char*>(nm));
                }
                ar->Write(chainName, 0x80);
                ar->Write(m_blob, 0x10);
                break;
            }
        }

        Clock64* drift = &m_driftPos;
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(drift, 8);
                drift++;
                ar->Read(drift, 8);
                break;
            case SERIAL_SAVE:
                ar->Write(drift, 8);
                drift++;
                ar->Write(drift, 8);
                break;
        }
        Clock64* idle = &m_peekTimer;
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(idle, 8);
                idle++;
                ar->Read(idle, 8);
                break;
            case SERIAL_SAVE:
                ar->Write(idle, 8);
                idle++;
                ar->Write(idle, 8);
                break;
        }

        char tailName[0x80];
        switch (mode) {
            case SERIAL_SAVE: {
                memset(tailName, 0, sizeof(tailName));
                if (m_cue != 0) {
                    CString nm = m_animWorker->m_ownerCtx->m_soundRegistry->FindKeyOfValue(m_cue);
                    strcpy(tailName, static_cast<const char*>(nm));
                }
                ar->Write(tailName, 0x80);
                g_serialCounter++;
                i32 id = 0;
                if (m_glitterSprite != 0) {
                    id = m_glitterSprite->m_objectId;
                }
                ar->Write(&id, 4);
                break;
            }
            case SERIAL_LOAD: {
                ar->Read(tailName, 0x80);

                if (strlen(tailName) == 0) {
                    m_cue = 0;
                } else {
                    void* val = 0;
                    m_animWorker->m_ownerCtx->m_soundRegistry->m_cues.Lookup(tailName, val);
                    m_cue = static_cast<LeafCue*>(val);
                }
                g_serialCounter++;
                i32 id = 0;
                ar->Read(&id, 4);
                void* found = 0;
                CWwdGameObjectA* sprite = 0;
                if (MapLookupById(m_animWorker->m_ownerCtx->m_childGroup->m_map48, id, found) != 0
                    && found != 0
                    && static_cast<CGameObject*>(found)->GetClassId() == CLASSID_SERIALREF) {
                    sprite = static_cast<CWwdGameObjectA*>(found);
                }
                m_glitterSprite = sprite;
                if (sprite != 0) {
                    break;
                }

                if (id != 0) {
                    return 0;
                }
                break;
            }
            case SERIAL_POSTLOAD:
                if (HandleInput() == 0) {
                    return 0;
                }
                break;
        }
        return 1;
    }
    RVA(0x00011cb0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMEICON;
    }

public:
    CInGameIcon() {}
    CInGameIcon(CGameObject* obj);

    void SetupSprite(const char* cat);

    i32 HandleInput();
    virtual void FireActivation(i32 id) OVERRIDE;

    i32 RefreshCell();
    i32 PeekCycle();
    i32 PlaceAt(i32 idx, i32 gridBase);
    i32 Reposition();

    LeafCue* m_cue;
    Clock64 m_driftPos;
    Clock64 m_driftThresh;
    Clock64 m_peekTimer;
    Clock64 m_peekWindow;
    CWwdGameObjectA* m_glitterSprite;
    i32 m_reserved7c; // retail news 0x80 (push in CreateInGameIcon); position unproven
};
SIZE(0x80);

#endif // GRUNTZ_GRUNTZ_CINGAMEICON_H
