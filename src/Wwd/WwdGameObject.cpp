#include <rva.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Blk6c.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>
#include <Wwd/AnimWorkerAct.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002bf674)
i32 g_logicTypesRegistered;

// @identity-TODO ApplyGeometryDirect@CWwdGameObjectA - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (44 fns) came from the static library. It belongs to another compiland.
// @early-stop
// one scheduling slot: retail sinks the `sprOb = 0` store below both Lookup
// argument pushes. The post-call body is exact; 96 mixed TU states and 35
// local variants were byte-identical at this remaining slot.
RVA(0x001504d0, 0x6c)
void CWwdGameObjectA::ApplyLookupSprite(const char* name, i32 frame) {
    CObject* sprOb = 0;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(name, sprOb);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(sprOb);
    m_frameSet = spr;
    if (spr) {
        CImage* f;
        if (frame >= spr->m_minIndex && frame <= spr->m_maxIndex) {
            f = static_cast<CImage*>(spr->m_items.GetAt(frame));
        } else {
            f = 0;
        }
        m_frameIndex = frame;
        m_layer = f;
    }
}

RVA(0x00150540, 0x65)
void CWwdGameObjectA::ApplyName(const char* name) {
    CDDrawWorker* spr = 0;
    CObject* sprOb = 0;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(name, sprOb);
    spr = static_cast<CDDrawWorker*>(sprOb);
    m_frameSet = spr;
    if (spr) {
        i32 n = spr->m_minIndex;
        m_frameIndex = n;
        if (n >= spr->m_minIndex && n <= spr->m_maxIndex) {
            m_layer = static_cast<CImage*>(spr->m_items.GetAt(n));
        } else {
            CImage* none = 0;
            m_layer = none;
        }
    }
}

RVA(0x001505b0, 0x5e)
i32 CWwdGameObjectA::ApplyLookupGeometry(const char* name, i32 applyDefault) {

    CAniElement* spr = 0;
    MapLookup(OwnerMgr()->m_animRegistry->m_animations, name, spr);
    if (!spr) {
        return 0;
    }
    m_animCursor.Setup(spr);
    if (applyDefault) {
        m_animCursor.Advance(g_engineFrameDelta);
    }
    return 1;
}

// @early-stop
RVA(0x00150610, 0x41)
i32 CWwdGameObjectA::LookupAnimSprite(const char* name) {
    LeafCue* cue = 0;
    MapLookup(OwnerMgr()->m_soundRegistry->m_cues, name, cue);
    if (cue == NULL) {
        return 0;
    }
    m_soundCue = cue;
    return 1;
}

RVA(0x00150660, 0x49)
void CWwdGameObjectA::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {

    m_shadow = m_dirty;
    if (m_dirty.m_armed != -1) {
        RECT* r = &m_dirty.m_rect;
        a->m_surface->BltFast(r->left, r->top, b->m_surface, r, 0x10);
        m_dirty.m_armed = -1;
    }
}

// @early-stop
RVA(0x001506b0, 0x1ec)
void CWwdGameObjectA::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) {
    i32 rc[4];
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        RECT ir;
        if (IntersectRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect)) {
            UnionRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect);
            i32 w = ir.right - ir.left + 1;
            i32 h = ir.bottom - ir.top + 1;
            rc[0] = ir.left;
            rc[1] = ir.top;
            rc[2] = ir.left + w;
            rc[3] = ir.top + h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        } else {
            rc[0] = m_dirty.m_lastX;
            rc[1] = m_dirty.m_lastY;
            rc[2] = m_dirty.m_lastX + m_dirty.m_w;
            rc[3] = m_dirty.m_lastY + m_dirty.m_h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
            rc[0] = m_shadow.m_lastX;
            rc[1] = m_shadow.m_lastY;
            rc[2] = m_shadow.m_lastX + m_shadow.m_w;
            rc[3] = m_shadow.m_lastY + m_shadow.m_h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        }
    } else if (m_dirty.m_armed != -1) {
        rc[0] = m_dirty.m_lastX;
        rc[1] = m_dirty.m_lastY;
        rc[2] = m_dirty.m_lastX + m_dirty.m_w;
        rc[3] = m_dirty.m_lastY + m_dirty.m_h;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    } else if (m_shadow.m_armed != -1) {
        rc[0] = m_shadow.m_lastX;
        rc[1] = m_shadow.m_lastY;
        rc[2] = m_shadow.m_lastX + m_shadow.m_w;
        rc[3] = m_shadow.m_lastY + m_shadow.m_h;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    }
}

RVA(0x001508a0, 0x117)
void CWwdGameObjectA::BltDirtyRegions(
    CDDrawSurfacePair* a,
    CDDrawSurfacePair* b,
    CDDrawSurfacePair* c
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        RECT ir;
        if (IntersectRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect)) {
            UnionRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect);
            i32 pos[2];
            i32 size[2];

            pos[0] = ir.left;
            pos[1] = ir.top;
            size[0] = ir.right - ir.left + 1;
            size[1] = ir.bottom - ir.top + 1;
            a->BlitDirtyRect(b, pos, size);
        } else {
            a->BlitDirtyRect(b, &m_dirty.m_lastX, &m_dirty.m_w);
            a->BlitDirtyRect(b, &m_shadow.m_lastX, &m_shadow.m_w);
        }
    } else if (m_dirty.m_armed != -1) {
        a->BlitDirtyRect(b, &m_dirty.m_lastX, &m_dirty.m_w);
    } else if (m_shadow.m_armed != -1) {
        a->BlitDirtyRect(b, &m_shadow.m_lastX, &m_shadow.m_w);
    }
}

// @early-stop
RVA(0x001509c0, 0xab)
i32 CWwdGameObjectA::Test() {
    if (m_layer == NULL) {
        return 0;
    }
    i32 sx = m_screenX;
    i32 ax = m_layer->m_anchorX;
    i32 right = sx + ax;
    i32 left = sx - ax;
    i32 sy = m_screenY;
    i32 ay = m_layer->m_anchorY;
    i32 top = sy - ay;
    i32 bottom = sy + ay;
    if (m_flags & 0x40000) {

        RECT* r = &OwnerMgr()->m_level->m_mainPlane->m_viewRect;
        if (right < r->left) {
            return 0;
        }
        if (left > r->right) {
            return 0;
        }
        if (bottom < r->top) {
            return 0;
        }
        return top <= r->bottom;
    } else {

        CDDrawSurfaceChildA* g = OwnerMgr()->m_drawTarget->m_frontPair;

        i32 gw = g->m_width;
        i32 gh = g->m_height;
        if (right < 0) {
            return 0;
        }
        if (left >= gw) {
            return 0;
        }
        if (bottom < 0) {
            return 0;
        }
        return top < gh;
    }
}

RVA(0x00150a70, 0x89)
i32 CWwdGameObjectA::Play(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, void* self) {
    if (ar == NULL) {
        return 0;
    }
    if (m_animCursor.Find(ar, mode, typeId, self) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (ReadState(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (SerializeSpriteName(ar) == 0) {
                return 0;
            }
            break;
    }
    return CGameObject::Play(ar, mode, typeId, self) != 0;
}

RVA(0x00150b00, 0x12b)
i32 CWwdGameObjectA::ReadState(CFileMemBase* src) {
    CFileMemBase* ar = src;
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_reserved18c, sizeof(m_reserved18c));
    ar->Write(&m_frameIndex, sizeof(m_frameIndex));
    i32 flag = 0;
    if (m_layer != NULL) {
        flag = 1;
    }
    ar->Write(&flag, sizeof(flag));

    char tmp[0x100];
    memset(tmp, 0, SERIAL_NAME_LEN);
    if (m_frameSet != NULL) {
        strcpy(tmp, m_frameSet->m_name);
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    memset(tmp, 0, SERIAL_NAME_LEN);
    {
        strcpy(tmp, OwnerMgr()->m_soundRegistry->FindKeyOfValue(m_soundCue));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);
    return 1;
}

RVA(0x00150c30, 0x130)
i32 CWwdGameObjectA::SerializeSpriteName(CFileMemBase* src) {
    CFileMemBase* ar = src;
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_reserved18c, sizeof(m_reserved18c));
    ar->Read(&m_frameIndex, sizeof(m_frameIndex));
    i32 flag;
    ar->Read(&flag, sizeof(flag));
    m_frameSet = NULL;

    char name[0x100];
    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {

        CDDrawWorker* found = 0;
        CObject* foundOb = 0;
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        mgr->m_imageRegistry->m_10map.Lookup(name, foundOb);
        found = static_cast<CDDrawWorker*>(foundOb);
        m_frameSet = found;
        if (found != NULL && flag == 1) {
            i32 idx = m_frameIndex;
            CImage* frame;
            if (idx >= found->m_minIndex && idx <= found->m_maxIndex) {
                frame = static_cast<CImage*>(found->m_items.GetAt(idx));
            } else {
                frame = NULL;
            }
            m_layer = frame;
        }
    }

    m_soundCue = NULL;
    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {

        void* found = 0;
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        mgr->m_soundRegistry->m_cues.Lookup(name, found);
        m_soundCue = static_cast<LeafCue*>(found);
    }
    return 1;
}

// @early-stop
RVA(0x00150d60, 0x14d)
i32 CGameObject::Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) {
    CResolveNode::SetPosition(x, y);
    m_screenX = x;
    m_screenY = y;
    m_sortKey = sortKey;
    m_spawnX = x;
    AnimWorkerObj* w = m_animWorker;
    m_spawnY = y;
    m_spawnSortKey = sortKey;
    m_strideX = 10;
    m_strideY = 10;
    m_points = 0;
    m_score = 0;
    m_health = 0;
    m_smarts = 0;
    m_powerup = 0;
    m_damage = 0;
    m_direction = 0;
    m_faceDirection = 0;
    m_speedX = 0;
    m_speedY = 0;
    m_reservede0 = 0;
    m_reserved180 = 0;

    if (w->Init(tmpl->m_notify, tmpl->m_flags) == 0) {
        return 0;
    }
    m_hitWorker = NULL;
    m_attackWorker = NULL;
    m_collideWorker = NULL;
    m_hitSource = NULL;
    m_attackTarget = NULL;
    m_hitOther = NULL;
    m_objectType = 0;
    m_hitTypeFlags = 0;
    m_attackTypeMask = 0;
    m_collMask = 0;
    m_extent.left = COORD_UNSET;
    m_area.left = COORD_UNSET;
    m_switchRect.left = COORD_UNSET;
    m_region.m_object = this;
    m_region.m_x = m_screenX;
    m_region.m_y = m_screenY;
    i32 wf = m_animWorker->m_flags;
    if (wf & 1) {
        m_flags |= 0x800000;
        return 1;
    }
    if (wf & 2) {
        m_flags |= 0x1000000;
    }
    return 1;
}

RVA(0x00150eb0, 0x98)
i32 CGameObject::EnsureHitWorker(AnimWorkerObj* src) {
    if (src == NULL) {
        return 0;
    }
    if (m_hitWorker != NULL) {
        m_hitWorker->Unload();
    } else {
        m_hitWorker = new AnimWorkerObj(m_ownerCtx, m_id);
    }
    if (m_hitWorker == NULL) {
        return 0;
    }

    return m_hitWorker->Init(src->m_notify, 0);
}

// @early-stop
RVA(0x00150f50, 0x35)
void CGameObject::AddLogicHit(char* key) {
    CObject* handlerOb = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(key, handlerOb);
    EnsureHitWorker(static_cast<AnimWorkerObj*>(handlerOb));
}

RVA(0x00150f90, 0x98)
i32 CGameObject::EnsureAttackWorker(AnimWorkerObj* src) {
    if (src == NULL) {
        return 0;
    }
    if (m_attackWorker != NULL) {
        m_attackWorker->Unload();
    } else {
        m_attackWorker = new AnimWorkerObj(m_ownerCtx, m_id);
    }
    if (m_attackWorker == NULL) {
        return 0;
    }

    return m_attackWorker->Init(src->m_notify, 0);
}

// @early-stop
RVA(0x00151030, 0x35)
void CGameObject::AddLogicAttack(char* key) {
    CObject* handlerOb = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(key, handlerOb);
    EnsureAttackWorker(static_cast<AnimWorkerObj*>(handlerOb));
}

RVA(0x00151070, 0x98)
i32 CGameObject::EnsureBumpWorker(AnimWorkerObj* src) {
    if (src == NULL) {
        return 0;
    }
    if (m_collideWorker != NULL) {
        m_collideWorker->Unload();
    } else {
        m_collideWorker = new AnimWorkerObj(m_ownerCtx, m_id);
    }
    if (m_collideWorker == NULL) {
        return 0;
    }

    return m_collideWorker->Init(src->m_notify, 0);
}

// @early-stop
RVA(0x00151110, 0x35)
void CGameObject::AddLogicBump(char* key) {
    CObject* handlerOb = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(key, handlerOb);
    EnsureBumpWorker(static_cast<AnimWorkerObj*>(handlerOb));
}

// @early-stop
// cl and retail cross-jump a DIFFERENT pair of the four post-notify blocks, which moves
// the shared tail and cascades: docs/patterns/cross-jump-grouping-floors-objdiff.md.
RVA(0x00151150, 0x190)
i32 CGameObject::Play(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, void* self) {
    if (ar == NULL) {
        return 0;
    }

    switch (mode) {
        case SERIAL_PRESAVE: {
            m_carrierId = 0;
            if (m_carrier != NULL) {
                m_carrierId = m_carrier->m_objectId;
            }
            AnimWorkerObj* w3 = m_animWorker;
            if (w3 == NULL) {
                goto fail;
            }
            i32 saved3 = w3->m_actKey;
            w3->SetWorkerAct(ACT_PREPARE_SAVE);

            m_animWorker->m_notify(this);
            w3 = m_animWorker;
            if (w3->WorkerAct() == ACT_PREPARE_SAVE) {
                w3->m_actKey = saved3;
            }
            break;
        }
        case SERIAL_SAVE: {
            if (Serialize(ar) == 0) {
                return 0;
            }
            AnimWorkerObj* w4 = m_animWorker;
            if (w4 == NULL) {
                goto fail;
            }
            i32 saved4 = w4->m_actKey;
            w4->SetWorkerAct(ACT_AFTER_SAVE);

            m_animWorker->m_notify(this);
            w4 = m_animWorker;
            if (w4->WorkerAct() == ACT_AFTER_SAVE) {
                w4->m_actKey = saved4;
            }
            break;
        }
        case SERIAL_LOAD: {
            if (SerializeObjectState(ar) == 0) {
                return 0;
            }
            AnimWorkerObj* w7 = m_animWorker;
            if (w7 == NULL) {
                goto fail;
            }
            i32 saved7 = w7->m_actKey;
            w7->SetWorkerAct(ACT_AFTER_LOAD);

            m_animWorker->m_notify(this);
            w7 = m_animWorker;
            if (w7->WorkerAct() == ACT_AFTER_LOAD) {
                w7->m_actKey = saved7;
            }
            break;
        }
        case SERIAL_POSTLOAD: {
            i32 node = m_carrierId;
            if (node != 0) {
                void* found = 0;
                if (MapLookupById(OwnerMgr()->m_childGroup->m_map48, node, found) == 0) {
                    m_carrier = NULL;
                } else {

                    m_carrier = static_cast<CWwdGameObject*>(found);
                }
            } else {
                m_carrier = NULL;
            }
            AnimWorkerObj* w8 = m_animWorker;
            if (w8 == NULL) {
                goto fail;
            }
            i32 saved8 = w8->m_actKey;
            w8->SetWorkerAct(ACT_AFTER_LOAD_REFERENCES);

            m_animWorker->m_notify(this);
            w8 = m_animWorker;
            if (w8->WorkerAct() == ACT_AFTER_LOAD_REFERENCES) {
                w8->m_actKey = saved8;
            }
            break;
        }
    }
    return m_animWorker->Dispatch(ar, mode, typeId, self) != 0;
fail:
    return 0;
}

RVA(0x00151320, 0x454)
i32 CGameObject::Serialize(CFileMemBase* arParam) {
    CFileMemBase* ar = arParam;
    if (ar == NULL) {
        return 0;
    }

    ar->Write(&m_shadow, sizeof(m_shadow));

    char tmp[SERIAL_NAME_LEN];
    memset(tmp, 0, sizeof(tmp));
    strcpy(tmp, m_name);
    ar->Write(tmp, SERIAL_NAME_LEN);

    ar->Write(&m_moveMode, sizeof(m_moveMode));
    ar->Write(&m_objectType, sizeof(m_objectType));
    ar->Write(&m_hitTypeFlags, sizeof(m_hitTypeFlags));
    ar->Write(&m_attackTypeMask, sizeof(m_attackTypeMask));
    ar->Write(&m_collMask, sizeof(m_collMask));
    ar->Write(&m_strideX, sizeof(m_strideX));
    ar->Write(&m_strideY, sizeof(m_strideY));
    ar->Write(&m_reserved100, sizeof(m_reserved100));
    ar->Write(&m_spawnX, sizeof(m_spawnX));
    ar->Write(&m_spawnY, sizeof(m_spawnY));
    ar->Write(&m_spawnSortKey, sizeof(m_spawnSortKey));
    ar->Write(&m_reserved110, sizeof(m_reserved110));
    ar->Write(&m_score, sizeof(m_score));
    ar->Write(&m_points, sizeof(m_points));
    ar->Write(&m_powerup, sizeof(m_powerup));
    ar->Write(&m_damage, sizeof(m_damage));
    ar->Write(&m_smarts, sizeof(m_smarts));
    ar->Write(&m_health, sizeof(m_health));
    ar->Write(&m_direction, sizeof(m_direction));
    ar->Write(&m_faceDirection, sizeof(m_faceDirection));
    ar->Write(&m_extent.left, 0x10);
    ar->Write(&m_area.left, 0x10);
    ar->Write(&m_switchRect.left, 0x10);
    ar->Write(&m_speedX, sizeof(m_speedX));
    ar->Write(&m_speedY, sizeof(m_speedY));
    ar->Write(&m_reserved16c, sizeof(m_reserved16c));
    ar->Write(&m_reserved170, sizeof(m_reserved170));
    ar->Write(&m_deltaX, sizeof(m_deltaX));
    ar->Write(&m_deltaY, sizeof(m_deltaY));
    ar->Write(&m_reserved17c, sizeof(m_reserved17c));
    ar->Write(&m_reserved180, sizeof(m_reserved180));
    ar->Write(&m_plotDX, sizeof(m_plotDX));
    ar->Write(&m_plotDY, sizeof(m_plotDY));
    ar->Write(&m_dirty, sizeof(m_dirty));
    ar->Write(&m_stateFlags, sizeof(m_stateFlags));
    ar->Write(&m_flashCountdown, sizeof(m_flashCountdown));
    ar->Write(&m_flashInterval, sizeof(m_flashInterval));
    ar->Write(&m_drawFillCmd, sizeof(m_drawFillCmd));
    ar->Write(&m_fillFraction, sizeof(m_fillFraction));
    ar->Write(&m_drawActive, sizeof(m_drawActive));
    ar->Write(&m_clip.left, 0x10);
    ar->Write(&m_id, sizeof(m_id));
    ar->Write(&m_flags, sizeof(m_flags));
    ar->Write(&m_carrierId, sizeof(m_carrierId));

    memset(tmp, 0, sizeof(tmp));
    if (m_hitWorker != NULL) {
        strcpy(tmp, OwnerMgr()->m_workerCache->FindKeyOfValue(m_hitWorker));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    memset(tmp, 0, sizeof(tmp));
    if (m_attackWorker != NULL) {
        strcpy(tmp, OwnerMgr()->m_workerCache->FindKeyOfValue(m_attackWorker));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    memset(tmp, 0, sizeof(tmp));
    if (m_collideWorker != NULL) {
        strcpy(tmp, OwnerMgr()->m_workerCache->FindKeyOfValue(m_collideWorker));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);
    return 1;
}

RVA(0x00151780, 0x40d)
i32 CGameObject::SerializeObjectState(CFileMemBase* arParam) {
    CFileMemBase* ar = arParam;
    if (ar == NULL) {
        return 0;
    }

    ar->Read(&m_shadow, sizeof(m_shadow));

    char name[SERIAL_NAME_LEN];
    ar->Read(name, SERIAL_NAME_LEN);
    m_name = name;

    ar->Read(&m_moveMode, sizeof(m_moveMode));
    ar->Read(&m_objectType, sizeof(m_objectType));
    ar->Read(&m_hitTypeFlags, sizeof(m_hitTypeFlags));
    ar->Read(&m_attackTypeMask, sizeof(m_attackTypeMask));
    ar->Read(&m_collMask, sizeof(m_collMask));
    ar->Read(&m_strideX, sizeof(m_strideX));
    ar->Read(&m_strideY, sizeof(m_strideY));
    ar->Read(&m_reserved100, sizeof(m_reserved100));
    ar->Read(&m_spawnX, sizeof(m_spawnX));
    ar->Read(&m_spawnY, sizeof(m_spawnY));
    ar->Read(&m_spawnSortKey, sizeof(m_spawnSortKey));
    ar->Read(&m_reserved110, sizeof(m_reserved110));
    ar->Read(&m_score, sizeof(m_score));
    ar->Read(&m_points, sizeof(m_points));
    ar->Read(&m_powerup, sizeof(m_powerup));
    ar->Read(&m_damage, sizeof(m_damage));
    ar->Read(&m_smarts, sizeof(m_smarts));
    ar->Read(&m_health, sizeof(m_health));
    ar->Read(&m_direction, sizeof(m_direction));
    ar->Read(&m_faceDirection, sizeof(m_faceDirection));
    ar->Read(&m_extent.left, 0x10);
    ar->Read(&m_area.left, 0x10);
    ar->Read(&m_switchRect.left, 0x10);
    ar->Read(&m_speedX, sizeof(m_speedX));
    ar->Read(&m_speedY, sizeof(m_speedY));
    ar->Read(&m_reserved16c, sizeof(m_reserved16c));
    ar->Read(&m_reserved170, sizeof(m_reserved170));
    ar->Read(&m_deltaX, sizeof(m_deltaX));
    ar->Read(&m_deltaY, sizeof(m_deltaY));
    ar->Read(&m_reserved17c, sizeof(m_reserved17c));
    ar->Read(&m_reserved180, sizeof(m_reserved180));
    ar->Read(&m_plotDX, sizeof(m_plotDX));
    ar->Read(&m_plotDY, sizeof(m_plotDY));
    ar->Read(&m_dirty, sizeof(m_dirty));
    ar->Read(&m_stateFlags, sizeof(m_stateFlags));
    ar->Read(&m_flashCountdown, sizeof(m_flashCountdown));
    ar->Read(&m_flashInterval, sizeof(m_flashInterval));
    ar->Read(&m_drawFillCmd, sizeof(m_drawFillCmd));
    ar->Read(&m_fillFraction, sizeof(m_fillFraction));
    ar->Read(&m_drawActive, sizeof(m_drawActive));
    ar->Read(&m_clip.left, 0x10);
    ar->Read(&m_id, sizeof(m_id));
    ar->Read(&m_flags, sizeof(m_flags));
    ar->Read(&m_carrierId, sizeof(m_carrierId));

    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {
        CObject* found = 0;
        OwnerMgr()->m_workerCache->m_workers.Lookup(name, found);
        if (this->EnsureHitWorker(static_cast<AnimWorkerObj*>(found)) == 0) {
            return 0;
        }
    }

    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {
        CObject* found = 0;
        OwnerMgr()->m_workerCache->m_workers.Lookup(name, found);
        if (this->EnsureAttackWorker(static_cast<AnimWorkerObj*>(found)) == 0) {
            return 0;
        }
    }

    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {
        CObject* found = 0;
        OwnerMgr()->m_workerCache->m_workers.Lookup(name, found);
        if (this->EnsureBumpWorker(static_cast<AnimWorkerObj*>(found)) == 0) {
            return 0;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00151b90, 0x70)
i32 CGameObject::ResolveLinkedObject(i32 gate) {
    if (gate == 0) {
        return 0;
    }
    void* found = 0;
    if (m_carrierId != 0) {
        if (MapLookupById(OwnerMgr()->m_childGroup->m_map48, m_carrierId, found) == 0) {
            m_carrier = NULL;
            return 1;
        }
        m_carrier = static_cast<CWwdGameObject*>(found);
        return 1;
    }
    m_carrier = NULL;
    return 1;
}

RVA(0x00151c00, 0x118)
i32 CGameObject::WriteSnapshot(CFileMemBase* dst, LogicTypeId unused) {
    CFileMemBase* ar = dst;
    if (ar == NULL) {
        return 0;
    }
    AnimWorkerObj* w = m_animWorker;
    if (w == NULL) {
        return 0;
    }
    if (w->m_actKey == 0) {
        w->m_notify(this);
    }

    i32 serialTypeId = 0;

    if (this->GetClassId() == CLASSID_CALLBACKOBJ) {
        serialTypeId = static_cast<CWwdGameObjectSerial*>(this)->GetSerialTypeId();
    }

    w = m_animWorker;
    CUserLogic* logic = w->m_logic;
    // Seeded with 0, NOT LOGIC_NONE(-1): retail writes 0 into the record when
    // the worker has no logic, and 0 is not a member of this domain.
    LogicTypeId typeTag = LOGIC_UNSET;
    if (logic != NULL) {
        typeTag = logic->GetTypeTag();
    }

    WwdSnapshot rec;
    rec.m_id = m_id;
    rec.m_classId = this->GetClassId();
    rec.m_objectId = m_objectId;
    rec.m_screenX = m_screenX;
    rec.m_screenY = m_screenY;
    rec.m_sortKey = m_sortKey;
    rec.m_serialTypeId = serialTypeId;
    rec.m_logicTypeId = typeTag;

    {
        strcpy(rec.m_workerName, OwnerMgr()->m_workerCache->FindKeyOfValue(m_animWorker));
    }
    ar->Write(&rec, sizeof(rec));
    return 1;
}

RVA(0x00151d20, 0x3a)
i32 CGameObject::NotifyHooked(i32 arg) {
    AnimWorkerObj* p = m_animWorker;
    if (!p) {
        return 0;
    }
    i32 saved = p->m_actKey;
    p->m_actKey = arg;
    m_animWorker->m_notify(this);
    if (m_animWorker->m_actKey == arg) {
        m_animWorker->m_actKey = saved;
    }
    return 1;
}

RVA(0x00151d60, 0xb)
i32 AnimWorkerObj::IsLoaded() {
    return m_notify != NULL;
}

RVA(0x00151d70, 0x6)
LoadableClassId AnimWorkerObj::GetClassId() {
    return CLASSID_ANIMWORKER;
}

RVA_COMPGEN(0x00151d80, 0x1e, ??_GAnimWorkerObj@@UAEPAXI@Z)

RVA(0x00151da0, 0x80)
AnimWorkerObj::~AnimWorkerObj() {
    m_notify = NULL;
    if (m_payload) {
        delete[] m_payload;
        m_payload = NULL;
        m_payloadSize = 0;
    }
    if (m_logic) {
        delete m_logic;
        m_logic = NULL;
    }
    m_target = NULL;
}

RVA(0x00151e20, 0x46)
i32 AnimWorkerObj::Init(GameObjNotifyFn callback, i32 frame) {
    if (callback == NULL) {
        return 0;
    }
    m_notify = callback;
    m_flags = frame;
    m_payload = NULL;
    m_logic = NULL;
    m_timeDelay = 0;
    m_frameDelay = 0;
    m_minX = 0;
    m_minY = 0;
    m_maxX = 0;
    m_maxY = 0;
    m_positionedSound = NULL;
    m_reserved16c = 0;
    m_userFlags = 0;
    return 1;
}

RVA(0x00151e70, 0x3b)
void AnimWorkerObj::Unload() {
    m_notify = NULL;
    if (m_payload) {
        delete[] m_payload;
        m_payload = NULL;
        m_payloadSize = 0;
    }
    if (m_logic) {
        delete m_logic;
        m_logic = NULL;
    }
    m_target = NULL;
}

RVA(0x00151eb0, 0x43)
void CDDrawWorker::Unload() {
    for (i32 i = 0; i < m_items.GetSize(); i++) {
        CImage* el = static_cast<CImage*>(m_items.GetAt(i));
        if (el != NULL) {
            delete el;
        }
    }
    m_items.SetSize(0, -1);

    m_minIndex = 99999;
    m_maxIndex = 0;
}

RVA(0x00151f00, 0xa4)
CImage* CDDrawWorker::InsertFrame(void* src, i32 n, i32 mode) {
    if (n < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(n)) != NULL) {
        return 0;
    }

    CImage* worker = new CImage(n, Owner());
    if (!worker->Resolve(static_cast<CParseSource*>(src), mode)) {
        if (worker) {
            delete worker;
        }
        return 0;
    }
    m_items.SetAtGrow(n, static_cast<CObject*>(worker));
    if (n < m_minIndex) {
        m_minIndex = n;
    }
    if (n > m_maxIndex) {
        m_maxIndex = n;
    }
    return worker;
}

RVA(0x00151fb0, 0xa4)
CImage* CDDrawWorker::LoadFrame(char* path, i32 index, i32 keyed) {
    if (index < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(index)) != NULL) {
        return 0;
    }

    CImage* nf = new CImage(index, Owner());

    if (nf->Create(path, keyed) == 0) {
        if (nf != NULL) {
            delete nf;
        }
        return 0;
    }

    m_items.SetAtGrow(index, static_cast<CObject*>(nf));
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

RVA(0x00152060, 0xab)
CImage*
CDDrawWorker::CreateDescriptorFrame(PidHeader* desc, FileImageFormat mode, i32 index, u32 size) {
    if (index < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(index)) != NULL) {
        return 0;
    }

    CImage* nf = new CImage(index, Owner());

    if (nf->LoadDispatch(desc, mode, size, 1) == 0) {
        if (nf != NULL) {
            delete nf;
        }
        return 0;
    }

    m_items.SetAtGrow(index, static_cast<CObject*>(nf));
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

RVA(0x00152110, 0xa9)
CImage* CDDrawWorker::CreateBlankFrame(i32 width, i32 height, i32 index, i32 keyed) {
    if (index < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(index)) != NULL) {
        return 0;
    }

    CImage* nf = new CImage(index, Owner());

    if (nf->CreateBlankSurface(width, height, keyed) == 0) {
        if (nf != NULL) {
            delete nf;
        }
        return 0;
    }

    m_items.SetAtGrow(index, static_cast<CObject*>(nf));
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
    return nf;
}

RVA(0x001521c0, 0x2b)
void CDDrawWorker::AddFrameAt(void* elem, i32 index) {
    m_items.SetAtGrow(index, static_cast<CObject*>(elem));
    if (index < m_minIndex) {
        m_minIndex = index;
    }
    if (index > m_maxIndex) {
        m_maxIndex = index;
    }
}

RVA(0x001521f0, 0xbc)
i32 CDDrawWorker::BuildFramesFromSymTab(CSymTab* tab) {
    i32 count = 0;
    void* sym = tab->FirstSym();
    while (sym != NULL) {
        void* val = tab->NextSym2(sym);
        while (val != NULL) {
            char* p = (static_cast<CParseSource*>(val))->m_name;
            while (*p != 0) {
                if (*p >= '0' && *p <= '9') {
                    break;
                }
                p++;
            }
            i32 fi = atoi(p);
            if (InsertFrame(val, fi, 1) != NULL) {
                count++;
            }
            val = tab->NextSym3(val);
            if ((OwnerMgr()->m_flags & 0x100) && count > 0) {
                val = NULL;
            }
        }
        sym = tab->NextSym(sym);
        if ((OwnerMgr()->m_flags & 0x100) && count > 0) {
            sym = NULL;
        }
    }
    return count;
}

RVA(0x001522b0, 0xf7)
i32 CDDrawWorker::ValidateFramesFromSymTab(CSymTab* tab) {

    i32 matched = 0;
    i32 liveFrames = 0;
    i32 n = m_items.GetSize();
    for (i32 i = 0; i < n; i++) {
        CImage* el;
        if (i >= m_minIndex && i <= m_maxIndex) {
            el = static_cast<CImage*>(m_items.GetAt(i));
        } else {
            el = NULL;
        }
        if (el != NULL) {
            liveFrames++;
        }
    }
    void* sym = tab->FirstSym();
    while (sym != NULL) {
        void* val = tab->NextSym2(sym);
        while (val != NULL) {
            GZ_ENUM_RETURN(RezTypeTag, u32) tag = (static_cast<CParseSource*>(val))->GetEntryTag();
            if (tag == IMGTAG_XCP || tag == IMGTAG_PMB || tag == IMGTAG_DIR || tag == IMGTAG_DIP) {
                char* p = (static_cast<CParseSource*>(val))->m_name;
                while (*p != 0) {
                    if (*p >= '0' && *p <= '9') {
                        break;
                    }
                    p++;
                }
                i32 fi = atoi(p);
                if (0 == ReloadFrame(static_cast<CParseSource*>(val), fi, 1)) {
                    return -1;
                }
                matched++;
            }
            val = tab->NextSym3(val);
        }
        sym = tab->NextSym(sym);
    }
    return (matched >= liveFrames) ? matched : -1;
}

RVA(0x001523b0, 0x3b)
i32 CDDrawWorker::ReloadFrame(CParseSource* rec, i32 n, i32 flag) {
    CImage* el;
    if (n >= m_minIndex && n <= m_maxIndex) {
        el = static_cast<CImage*>(m_items.GetAt(n));
    } else {
        el = NULL;
    }
    if (el == NULL) {
        return 0;
    }
    return el->Reload(rec, flag) != 0;
}

// @early-stop
// The imul operand order is canonical and TU-state parity: no spelling of the
// multiply moves it, but ANY definition added before this function in the TU
// flips it to retail's. docs/patterns/commutative-operand-order-is-canonical.md
RVA(0x001523f0, 0x82)
i32 CDDrawWorker::GetMemoryUsage(i32 raw) {
    i32 sum = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame) {
            i32 size = frame->m_height * frame->m_width;
            if (frame->m_surface && frame->m_surface->m_bitDepth == BPP_RGB_16) {
                size += size;
            }
            if (frame->m_surface && frame->m_surface->m_bitDepth == BPP_RGB_24) {
                size = size * 3;
            }
            if (frame->m_owned) {
                size = frame->m_owned->m_rleLen;
            }
            if (raw == 0) {
                size += 0x34;
            }
            sum += size;
        }
    }
    return sum;
}

RVA(0x00152480, 0x4e)
i32 CDDrawWorker::SetAllTypes(ShadeMode type) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->Select(type, 0);
            count++;
        }
    }
    return count;
}

RVA(0x001524d0, 0x41)
i32 CDDrawWorker::SetAllLightLevels(i32 value) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->m_light = value;
            count++;
        }
    }
    return count;
}

RVA(0x00152520, 0x4b)
i32 CDDrawWorker::SetAllFormats(CShadeTable* format) {
    if (!format) {
        return 0;
    }
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->m_palDescr = format;
            count++;
        }
    }
    return count;
}

RVA(0x00152570, 0x24)
ShadeMode CDDrawWorker::GetFirstFrameState() {
    CImage* frame = static_cast<CImage*>(m_items.GetAt(m_minIndex));
    if (frame == NULL) {
        return SHADE_COPY;
    }
    CDDrawShadeBlit* fmt = frame->m_owned;
    if (fmt == NULL) {
        return SHADE_COPY;
    }
    return fmt->m_drawType;
}

RVA(0x001525c0, 0x76)
i32 CDDrawWorker::FindFrame(CImage* frame, char* outName, i32* outIndex) {
    if (frame) {
        for (i32 i = 0; i < m_items.GetSize(); i++) {
            CImage* cur = static_cast<CImage*>(m_items.GetAt(i));
            if (cur && cur == frame) {
                if (outName) {
                    strcpy(outName, m_name);
                }
                if (outIndex) {
                    *outIndex = i;
                }
                return 1;
            }
        }
    }
    return 0;
}
