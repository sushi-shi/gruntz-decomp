#ifndef GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
#define GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H

#include <list>
struct CGameObject;

#include <map>
#include <string>

#include <Ints.h>

#include <DDrawMgr/DDrawChildGroupFlags.h>
#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdGameObjectFlags.h>

struct CLogicRecord;

struct CGameObject;

struct CGameObject;
class CWwdSpriteObject;
class CWwdGameObject;
class CWwdDotObject;
class CWwdDeferredObject;
class CDrawSubWorker;

class CDDrawChildGroup : public CWapObj {
public:
    inline CGameObject* NextChild();
    inline CGameObject* FirstChild();
    CDDrawChildGroup(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {
        m_walkCursor = m_list.end();
        m_scanCursor = m_list.end();
    }

    virtual ~CDDrawChildGroup()  ;
    virtual i32 IsLoaded()  ;
    virtual i32 IsReady()  ;
    virtual void Unload()  ;
    virtual LoadableClassId GetClassId()  ;

    virtual void TickKillCues(i32 advance);
    virtual void RenderChildren(class CDDrawSurfacePair* target);

    virtual void BltDirtyChildren(CDDrawSurfacePair* dst, CDDrawSurfacePair* src);

    virtual void
    BltDirtyChildrenEx(CDrawSubWorker* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc);
    virtual void BltDirtyChildRegions(
        CDDrawSurfacePair* dst,
        CDDrawSurfacePair* src,
        CDDrawSurfacePair* restoreSrc
    );
    virtual void InvalidateChildShadows();
    virtual void DestroyChildren();
    virtual void CollideBroadcast();

    CWwdDotObject* CreateDotObject(
        int id,
        int x,
        int y,
        int sortKey,
        CLogicRecord* logicTemplate,
        int dotColor,
        int objectFlags
    );
    CWwdDeferredObject*
    CreateDeferredObject(int id, int sortKey, CLogicRecord* logicTemplate, int objectFlags);
    CWwdSpriteObject* CreateSpriteObject(
        int id,
        int x,
        int y,
        int sortKey,
        CLogicRecord* logicTemplate,
        int objectFlags
    );
    CWwdGameObject* CreateContainerObject(
        int id,
        int x,
        int y,
        int sortKey,
        CLogicRecord* logicTemplate,
        int objectFlags
    );

    CWwdDotObject* CreateNamedDotObject(
        int id,
        int x,
        int y,
        int sortKey,
        const std::string& name,
        int dotColor,
        int objectFlags
    );
    CWwdDeferredObject*
    CreateNamedDeferredObject(int id, int sortKey, const std::string& name, int objectFlags);
    CWwdGameObject* CreateNamedContainerObject(
        int id,
        int x,
        int y,
        int sortKey,
        const std::string& name,
        int objectFlags
    );

    CWwdSpriteObject*
    CreateSprite(i32 id, i32 x, i32 y, i32 sortKey, const std::string& name, i32 objectFlags);

    i32 AddObject(CGameObject* obj);
    i32
    AttachSprite(CWwdGameObject* obj, i32 x, i32 y, i32 sortKey, const std::string& name, i32 objectFlags);

    i32 LoadObjects(class CFileMemBase* reader, u32 count, LogicTypeId unused);

    void RemoveAll(std::list<CGameObject*>::iterator pos, CGameObject* obj);
    void RemoveByPosition(std::list<CGameObject*>::iterator pos, CGameObject* obj);
    void RegisterObjectId(CWwdGameObject* obj);
    void PruneList();
    i32 CountActive();

    i32 DispatchSerializationToObjects(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId);
    i32 WriteObjectSnapshots(CFileMemBase* ar, LogicTypeId typeId);
    i32 SerializeObjects(class CFileMemBase* ar, LogicTypeId typeId);
    i32 DeserializeObjects(class CFileMemBase* ar, u32 count, LogicTypeId typeId);
    i32 PruneOrphans();
    void RemoveAndDelete(CWwdGameObject* obj);
    void ReinsertUnflagged(CWwdGameObject* obj);
    void InsertSorted(CGameObject* obj, i32 addToMaps);
    i32 CheckSortOrder();
    CWwdGameObject* FindById(i32 id);
    CWwdGameObject* FindSerialRefById(i32 id);
    CWwdGameObject* FindByLogicRecord(i32 id, CLogicRecord* logicRecord);
    CWwdGameObject* FindByIdAndCollisionCategory(i32 id, u32 collisionCategory);

    CGameObject* Find(i32 id, const std::string& key);
    CWwdGameObject* FindByObjectId(i32 objectId);
    CWwdGameObject* FindSerialRefByObjectId(i32 objectId);
    i32 IsKindUnique(i32 kind);
    i32 CountByKind(i32 kind);
    i32 SumWeighted();

    std::list<CGameObject*> m_list;

    CGameObject* NextChild(std::list<CGameObject*>::iterator& pos);
    CGameObject* HeadChild() const;
    std::map<i32, CGameObject*> m_activeGameObjectsById;
    std::map<i32, CGameObject*> m_registeredGameObjectsById;

    std::list<CGameObject*>::iterator m_walkCursor;

    std::list<CGameObject*>::iterator m_scanCursor;

    void DrawObjectDebugGeometry();
    void DrawObjectCounts();

    void ClearChildren();

    i32 RectsOverlap(RECT* a, RECT* b);
    i32 BoxesOverlap(CGameObject* areaObj, CGameObject* switchObj);

    inline CGameObject* Drain();
    inline CGameObject* FirstSerialChild();
};

inline CGameObject* CDDrawChildGroup::FirstChild() {
    m_walkCursor = m_list.begin();
    if (m_walkCursor == m_list.end()) {
        return NULL;
    }
    return NextChild(m_walkCursor);
}

inline CGameObject* CDDrawChildGroup::NextChild() {
    if (m_walkCursor == m_list.end()) {
        return NULL;
    }
    return NextChild(m_walkCursor);
}

#endif
