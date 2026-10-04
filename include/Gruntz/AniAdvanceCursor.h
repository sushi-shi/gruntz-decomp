#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CWwdSpriteObject;
struct CGameObject;
class CAniFrameRecord;
class CAniElement;
class CFileMemBase;

GZ_ENUM_CONST_BEGIN(AniAdvanceValue)
    ANI_SCALE_ONE_BITS = 0x3f800000
GZ_ENUM_CONST_END(AniAdvanceValue)

class CAniAdvanceCursor : public CWapObj {
public:
    inline i32 IsComplete() const;
    enum EInlineCursor {
        INLINE_CURSOR
    };
    CAniAdvanceCursor() {}

    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 id, i32 flags);
    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 id, i32 flags, EInlineCursor)
        : CWapObj(owner, id, flags) {
        m_boundObject = NULL;
        m_animation = NULL;
        m_currentRecord = NULL;
    }
    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 id, i32 flags, CWapObj::ENoSeed)
        : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
        m_boundObject = NULL;
        m_animation = NULL;
        m_currentRecord = NULL;
    }
    virtual ~CAniAdvanceCursor() OVERRIDE {
        Unload();
        m_id = -1;
        m_flags = 0;
        m_ownerCtx = NULL;
    }
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;

    void BindSprite(CWwdSpriteObject* src);
    CAniElement* GetAnimation() const {
        return m_animation;
    }
    void SetConsumeDraw(b32 consume) {
        m_consumeDraw = consume;
    }
    void SetAnimation(CAniElement* animation);
    void RestartAnimation(i32 resetElapsedTime);

    i32 CanSerialize(CFileMemBase* ar);
    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);
    i32 CanDeserialize(CFileMemBase* ar);

    i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);
    i32 Advance(u32 elapsed);
    inline void AdvanceToNextRecord();

    CWwdSpriteObject* m_boundObject;
    CAniElement* m_animation;

    CAniFrameRecord* m_currentRecord;
    i32 m_recordIndex;
    u32 m_frameTicksLeft;
    b32 m_useElapsedTime;
    b32 m_finished;
    i32 m_consumeDraw;
    i32 m_pendingDraw;
    i32 m_curDraw;

    union {
        float m_scale;
        i32 m_scaleBits;
    };
};

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
