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
class CAniRecordView;
class CAniElement;
class CFileMemBase;

GZ_ENUM_CONST_BEGIN(AniAdvanceValue)
    ANI_SCALE_ONE_BITS = 0x3f800000
GZ_ENUM_CONST_END(AniAdvanceValue)

class CAniAdvanceCursor : public CWapObj {
public:
    // Tag type: picks the expanded sibling of the out-of-line 0x15b730 ctor.
    // Retail calls 0x15b730 from exactly one site (CreateSpriteObject, whose
    // CGameObject base is itself expanded); every other construction carries the
    // body inline - `call ??0CLoadable`, then the vptr and the three NULLs.
    enum EInlineCursor {
        INLINE_CURSOR
    };
    CAniAdvanceCursor() {}

    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 id, i32 flags);
    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 id, i32 flags, EInlineCursor)
        : CWapObj(owner, id, flags) {
        m_boundObject = NULL;
        m_animation = NULL;
        m_element = NULL;
    }
    // The third shape: body inline AND CWapObj's three stores inline, so the tag
    // is CWapObj's own.  CWwdGameObject::CreateObject (0x166640) writes
    // id/flags/owner straight to [esi+0x1a4/0x1a8/0x1ac] with no `call 0x156cb0`,
    // where the two other users of the same CWwdSpriteObject ctor keep the call.
    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 id, i32 flags, CWapObj::ENoSeed)
        : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
        m_boundObject = NULL;
        m_animation = NULL;
        m_element = NULL;
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
    void SetAnimation(CAniElement* animation);
    void RestartAnimation(i32 resetElapsedTime);

    i32 CanSerialize(CFileMemBase* ar);
    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);
    i32 CanDeserialize(CFileMemBase* ar);

    i32 ProcessSerialMode(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* self);
    i32 Advance(u32 elapsed);

    CWwdSpriteObject* m_boundObject;
    CAniElement* m_animation;

    CAniRecordView* m_element;
    i32 m_index;
    u32 m_frameTicksLeft;
    i32 m_useElapsedTime;
    i32 m_finished;
    i32 m_consumeDraw;
    i32 m_pendingDraw;
    i32 m_curDraw;

    union {
        float m_scale;
        i32 m_scaleBits;
    };
};

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
