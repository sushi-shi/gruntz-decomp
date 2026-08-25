#ifndef GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
#define GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroupFlags.h>
#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

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
    CDDrawChildGroup(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {
        m_walkCursor = NULL;
        m_scanCursor = NULL;
    }

    virtual ~CDDrawChildGroup() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

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
        const char* name,
        int dotColor,
        int objectFlags
    );
    CWwdDeferredObject*
    CreateNamedDeferredObject(int id, int sortKey, const char* name, int objectFlags);
    CWwdGameObject* CreateNamedContainerObject(
        int id,
        int x,
        int y,
        int sortKey,
        const char* name,
        int objectFlags
    );

    CWwdSpriteObject*
    CreateSprite(i32 id, i32 x, i32 y, i32 sortKey, const char* name, i32 objectFlags);

    i32 AddObject(CGameObject* obj);
    i32
    AttachSprite(CWwdGameObject* obj, i32 x, i32 y, i32 sortKey, const char* name, i32 objectFlags);

    i32 LoadObjects(class CFileMemBase* reader, u32 count, LogicTypeId unused);

    void RemoveAll(POSITION pos, CGameObject* obj);
    void RemoveByPosition(POSITION pos, CGameObject* obj);
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

    CGameObject* Find(i32 id, const char* key);
    CWwdGameObject* FindByObjectId(i32 objectId);
    CWwdGameObject* FindSerialRefByObjectId(i32 objectId);
    i32 IsKindUnique(i32 kind);
    i32 CountByKind(i32 kind);
    i32 SumWeighted();

    CObList m_list;

    CGameObject* NextChild(POSITION& pos);
    CGameObject* HeadChild() const;
    CMapPtrToPtr m_activeGameObjectsById;
    CMapPtrToPtr m_registeredGameObjectsById;

    POSITION m_walkCursor;

    POSITION m_scanCursor;

    void DrawObjectDebugGeometry();
    void DrawObjectCounts();

    void ClearChildren();

    // Both are members in retail: CollideBroadcast spills `this` to [esp] purely
    // to reload ecx before these two calls (0x159f06 / 0x15a0d8 / 0x15a105).
    i32 RectsOverlap(struct CDDrawRect* a, struct CDDrawRect* b);
    i32 BoxesOverlap(CGameObject* areaObj, CGameObject* switchObj);

    CGameObject* Drain();
};

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
