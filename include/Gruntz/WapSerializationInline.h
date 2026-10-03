#ifndef GRUNTZ_WAPSERIALIZATIONINLINE_H
#define GRUNTZ_WAPSERIALIZATIONINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>

#include <string.h>

// @early-stop
RVA(0x00008c00, 0x152)
inline i32 CWapX::SerializeAnimationState(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId unusedTypeId,
    CGameObject* object
) {
    char name[SERIAL_NAME_LEN];

    if (archive == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            archive->Read(name, SERIAL_NAME_LEN);
            archive->Read(m_blob, 0x10);
            m_gameObject = object;
            m_wwdObject = static_cast<CWwdSpriteObject*>(object);
            m_ownerLogicRecord = object->GetLogicRecord();
            if (strlen(name) == 0) {
                m_value = NULL;
            } else {
                CMapStringToPtr* map =
                    &m_ownerLogicRecord->m_ownerCtx->m_animRegistry->m_animations;
                CAniElement* value = MapFind<CAniElement>(*map, name);
                m_value = value;
            }
            break;
        }
        case SERIAL_SAVE: {

            memset(name, 0, sizeof(name));
            if (m_value != NULL) {
                strcpy(
                    name,
                    static_cast<const char*>(
                        m_ownerLogicRecord->m_ownerCtx->m_animRegistry->FindAnimationKey(m_value)
                    )
                );
            }
            archive->Write(name, SERIAL_NAME_LEN);
            archive->Write(m_blob, 0x10);
            break;
        }
    }
    return 1;
}

#endif // GRUNTZ_WAPSERIALIZATIONINLINE_H
