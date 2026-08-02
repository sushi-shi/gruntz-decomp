#ifndef GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
#define GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H

#include <rva.h>

#include <Gruntz/Loadable.h>
#include <Ints.h>

class CWwdGameObjectA;
class CAniRecordView;
class CAniElement;
class CFileMemBase;

class CAniAdvanceCursor : public CLoadable {
public:
    CAniAdvanceCursor() {}

    CAniAdvanceCursor(class CDDrawSurfaceMgr* owner, i32 field04, i32 field08);
    virtual ~CAniAdvanceCursor() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;

    void Construct(CWwdGameObjectA* src);
    void Setup(CAniElement* src);
    void Recompute(i32 resetGate);

    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);

    i32 Find(CFileMemBase* ar, i32 type, i32 typeId, void* self);
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

#ifndef ANIADVANCECURSOR_OOL_CTOR
inline CAniAdvanceCursor::CAniAdvanceCursor(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CLoadable(owner, field04, field08) {
    m_boundObject = 0;
    m_animation = 0;
    m_element = 0;
}
#endif

#endif // GRUNTZ_GRUNTZ_ANIADVANCECURSOR_H
