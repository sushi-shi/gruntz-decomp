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
class CAnimationSequence;
class CFileMemBase;

GZ_ENUM_CONST_BEGIN(AniAdvanceValue)
    ANI_DURATION_SCALE_ONE_BITS = 0x3f800000
GZ_ENUM_CONST_END(AniAdvanceValue)

class CAniAdvanceCursor : public CWapObj {
public:
    inline i32 IsComplete() const;
    enum EInlineCursor {
        INLINE_CURSOR
    };
    CAniAdvanceCursor() {}

    CAniAdvanceCursor(class CGameWorld* owner, i32 id, i32 flags);
    CAniAdvanceCursor(class CGameWorld* owner, i32 id, i32 flags, EInlineCursor)
        : CWapObj(owner, id, flags) {
        m_boundObject = NULL;
        m_animation = NULL;
        m_currentRecord = NULL;
    }
    CAniAdvanceCursor(class CGameWorld* owner, i32 id, i32 flags, CWapObj::ENoSeed)
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

    void BindSprite(CWwdSpriteObject* sprite);
    CAnimationSequence* GetAnimation() const {
        return m_animation;
    }
    void SetConsumeAnimationEvents(b32 consume) {
        m_consumeEvents = consume;
    }
    void SetAnimation(CAnimationSequence* animation);
    void RestartAnimation(i32 resetElapsedTime);

    i32 CanSerialize(CFileMemBase* archive);
    i32 Serialize(CFileMemBase* archive);
    i32 Deserialize(CFileMemBase* archive);
    i32 CanDeserialize(CFileMemBase* archive);

    i32 SerializeDispatch(
        CFileMemBase* archive,
        SerialMode mode,
        LogicTypeId typeId,
        CGameObject* object
    );
    i32 Advance(u32 elapsedMs);
    inline void AdvanceToNextRecord();

    CWwdSpriteObject* m_boundObject;
    CAnimationSequence* m_animation;

    CAniFrameRecord* m_currentRecord;
    i32 m_recordIndex;
    // Units follow m_useElapsedTime: milliseconds or animation updates.
    u32 m_recordDurationRemaining;
    b32 m_useElapsedTime;
    b32 m_finished;
    i32 m_consumeEvents;
    i32 m_pendingEventCode;
    i32 m_currentEventCode;

    union {
        float m_durationScale;
        i32 m_durationScaleBits;
    };
};

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
