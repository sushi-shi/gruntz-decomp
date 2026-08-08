#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <rva.h>

#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

#include <stddef.h>

class CWwdGameObjectA;
class CAniRecordView;
class CAniElement;
class CFileMemBase;

GZ_ENUM_CONST_BEGIN(AniAdvanceValue)
    ANI_SCALE_ONE_BITS = 0x3f800000
GZ_ENUM_CONST_END(AniAdvanceValue)

class CAniAdvanceCursor : public CLoadable {
public:
    // Tag type: picks the expanded sibling of the out-of-line 0x15b730 ctor.
    // Retail calls 0x15b730 from exactly one site (CreateSpriteObject, whose
    // CGameObject base is itself expanded); every other construction carries the
    // body inline - `call ??0CLoadable`, then the vptr and the three NULLs.
    enum EInlineCursor {
        INLINE_CURSOR
    };

    CAniAdvanceCursor() {}

    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 field04, i32 field08);
    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 field04, i32 field08, EInlineCursor)
        : CLoadable(owner, field04, field08) {
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

    void Construct(CWwdGameObjectA* src);
    void Setup(CAniElement* src);
    void Recompute(i32 resetGate);

    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);

    i32 Find(CFileMemBase* ar, SerialMode type, LogicTypeId typeId, void* self);
    i32 Advance(u32 elapsed);

    CWwdGameObjectA* m_boundObject;
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
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
