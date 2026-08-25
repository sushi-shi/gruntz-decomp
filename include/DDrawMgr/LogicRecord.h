#ifndef GRUNTZ_DDRAWMGR_LOGICRECORD_H
#define GRUNTZ_DDRAWMGR_LOGICRECORD_H

#include <rva.h>

#include <DDrawMgr/LogicRecordFlags.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Wap32/WapObj.h>
#include <Wwd/LogicRecordEvent.h>

#include <stddef.h>

class CUserLogic;
class CFileMemBase;
class CAmbientPosSound;

struct CGameObject;

typedef i32(__cdecl* LogicRecordDispatchFn)(CGameObject* obj);

class CDDrawSurfaceMgr;

struct CLogicRecord : public CWapObj {

    virtual ~CLogicRecord() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 Init(LogicRecordDispatchFn dispatch, i32 flags);

    CLogicRecord() {}

    // Out of line at 0x15b300 in WwdObjMgr.cpp; <DDrawMgr/LogicRecordCtorInline.h>
    // is the opt-in inline view for the one TU that expands it - a workaround
    // whose cost and removal condition are measured in that header.
    CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 logicFlags);

    CLogicRecord(CDDrawSurfaceMgr* owner, i32 id) : CWapObj(owner, id, 0, CWapObj::NO_SEED) {
        ResetLogicFields();
    }

    void ResetLogicFields() {
        m_dispatch = NULL;
        m_payload = NULL;
        m_userLogic = NULL;
        m_target = NULL;
        m_eventCode = 0;
        m_targetId = 0;
        m_payloadSize = 0;
    }

    i32 Consume(i32 amount);

    i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);
    i32 CacheTargetId(void* context);
    i32 Save(CFileMemBase* archive);
    i32 Load(CFileMemBase* archive);

    i32 ResolveTarget(void* context);

    LogicRecordDispatchFn m_dispatch;
    u8* m_payload;
    CUserLogic* m_userLogic;

    i32 EventCode() const {
        return m_eventCode;
    }
    void SetEventCode(i32 id) {
        m_eventCode = id;
    }
    GZ_ENUM_RETURN(LogicRecordEvent, u32) LogicEvent() const {
        return static_cast<LogicRecordEvent>(m_eventCode);
    }
    void SetLogicEvent(LogicRecordEvent act) {
        m_eventCode = IDX(act);
    }
    i32 m_eventCode;

    i32 m_timeDelay;
    i32 m_frameDelay;
    i32 m_userFlags;
    i32 m_minX;
    i32 m_maxX;
    i32 m_minY;
    i32 m_maxY;
    char m_pad3c[0x40 - 0x3c];
    i32 m_reserved40;
    i32 m_tweakX;
    i32 m_tweakY;
    i32 m_scrollTargetX;
    i32 m_scrollTargetY;
    char m_pad54[0x58 - 0x54];
    i32 m_reserved58;
    i32 m_reserved5c;
    i32 m_reserved60;
    i32 m_user1;
    i32 m_user2;
    i32 m_user3;
    i32 m_user4;
    i32 m_user5;
    i32 m_user6;
    i32 m_user7;
    i32 m_user8;
    i32 m_reserved84;
    i32 m_reserved88;
    i32 m_reserved8c;
    i32 m_reserved90;
    i32 m_reserved94;
    i32 m_reserved98;
    i32 m_reserved9c;
    i32 m_reserveda0;
    i32 m_reserveda4;
    i32 m_reserveda8;
    i32 m_reservedac;
    i32 m_reservedb0;
    i32 m_reservedb4;
    i32 m_counter;
    i32 m_speed;

    char m_padc0[0xc4 - 0xc0];
    i32 m_reservedc4;
    i32 m_width;
    i32 m_height;
    RECT m_reservedd0;
    RECT m_reservede0;

    RECT m_userRect1;
    RECT m_userRect2;
    char m_pad110[0x120 - 0x110];
    RECT m_reserved120;
    i32 m_sparkleDelay;
    char m_pad134[0x138 - 0x134];
    i32 m_reserved138;
    i32 m_reserved13c;
    i32 m_reserved140;
    i32 m_reserved144;
    i32 m_reserved148;
    i32 m_reserved14c;
    i32 m_reserved150;
    i32 m_reserved154;
    i32 m_reserved158;
    i32 m_reserved15c;
    i32 m_reserved160;
    i32 m_reserved164;
    CAmbientPosSound* m_positionedSound;
    i32 m_reserved16c;
    CGameObject* m_target;
    i32 m_targetId;
    u32 m_payloadSize;
};

#endif // GRUNTZ_DDRAWMGR_LOGICRECORD_H
