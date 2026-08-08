#ifndef GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
#define GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

struct AnimWorkerObj;

struct CGameObject;

struct CGameObject;
class CWwdGameObjectA;
class CWwdGameObject;
class CWwdGameObjectC;
class CWwdGameObjectF;

class CDDrawChildGroup : public CLoadable {
public:
    CDDrawChildGroup(CDDrawSurfaceMgr* owner) : CLoadable(owner, 0, 0) {
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

    virtual void BltDirtyChildrenEx(
        CDDrawSurfacePair* dst,
        CDDrawSurfacePair* src,
        CDDrawSurfacePair* restoreSrc
    );
    virtual void BltDirtyChildRegions(
        CDDrawSurfacePair* dst,
        CDDrawSurfacePair* src,
        CDDrawSurfacePair* restoreSrc
    );
    virtual void InvalidateChildShadows();
    virtual void DestroyChildren();
    virtual void CollideBroadcast();

    CWwdGameObjectC* CreateDotObject(
        int id,
        int x,
        int y,
        int sortKey,
        AnimWorkerObj* tmpl,
        int dotColor,
        int stateFlags
    );
    CWwdGameObjectF* CreateDeferredObject(int id, int sortKey, AnimWorkerObj* tmpl, int stateFlags);
    CWwdGameObjectA*
    CreateSpriteObject(int id, int x, int y, int sortKey, AnimWorkerObj* tmpl, int stateFlags);
    CWwdGameObject*
    CreateContainerObject(int id, int x, int y, int sortKey, AnimWorkerObj* tmpl, int stateFlags);

    CWwdGameObjectC* CreateNamedDotObject(
        int id,
        int x,
        int y,
        int sortKey,
        const char* name,
        int dotColor,
        int stateFlags
    );
    CWwdGameObjectF*
    CreateNamedDeferredObject(int id, int sortKey, const char* name, int stateFlags);
    CWwdGameObject*
    CreateNamedContainerObject(int id, int x, int y, int sortKey, const char* name, int stateFlags);

    CWwdGameObjectA*
    CreateSprite(i32 id, i32 x, i32 y, i32 sortKey, const char* name, i32 stateFlags);

    i32
    AttachSprite(CWwdGameObject* obj, i32 x, i32 y, i32 sortKey, const char* name, i32 stateFlags);

    i32 LoadObjects(class CFileMemBase* reader, u32 count, LogicTypeId unused);

    void RemoveAll(POSITION pos, CGameObject* obj);
    void RemoveByPosition(POSITION pos, CGameObject* obj);
    void AddToMap48(CWwdGameObject* obj);
    void PruneList();
    i32 CountActive();

    i32 ForEachDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId);
    i32 ForEachProbe(CFileMemBase* ar, LogicTypeId typeId);
    i32 ForEachSerialize(class CFileMemBase* ar, LogicTypeId typeId);
    i32 Deserialize(class CFileMemBase* ar, u32 count, LogicTypeId flag);
    i32 PruneOrphans();
    void RemoveAndDelete(CWwdGameObject* obj);
    void ReinsertUnflagged(CWwdGameObject* obj);
    void InsertSorted(CGameObject* obj, i32 addToMaps);
    i32 CheckSortOrder();
    CWwdGameObject* FindByType04(i32 type);
    CWwdGameObject* FindByTypeProbe(i32 type);
    CWwdGameObject* FindByWorker(i32 type, void* key);
    CWwdGameObject* FindByIdAndCollisionCategory(i32 id, u32 collisionCategory);

    void* Find(i32 id, const char* key);
    CWwdGameObject* FindByKey(void* key);
    CWwdGameObject* FindByStatusKey(void* key);
    i32 IsKindUnique(i32 kind);
    i32 CountByKind(i32 kind);
    i32 SumWeighted();

    CObList m_list;

    CGameObject* NextChild(POSITION& pos);
    CGameObject* HeadChild() const;
    CMapPtrToPtr m_map2c;
    CMapPtrToPtr m_map48;

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
SIZE(0x6c);

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
