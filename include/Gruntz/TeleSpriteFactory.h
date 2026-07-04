// CTeleSpriteFactory.h - the HUD/visual sprite factory reached as mgr->m_30->m_8.
// CreateSprite (0x1597b0) spawns the named teleporter sprite. The produced sprite
// is the shared engine game object (CGameObject; both UserLogic.cpp and
// CTeleporter.cpp cast the result to CGameObject*), forward-declared here as an
// opaque return type. External/no-body so the call reloc-masks; the member mangling
// excludes the return type, so the caller-side cast is a no-op.
#ifndef GRUNTZ_GRUNTZ_CTELESPRITEFACTORY_H
#define GRUNTZ_GRUNTZ_CTELESPRITEFACTORY_H

#include <rva.h>

struct CTeleSprite; // the produced sprite (typed per caller)

struct CTeleSpriteFactory {
    CTeleSprite* CreateSprite(i32 a, i32 x, i32 y, i32 b, const char* key, i32 flags); // 0x1597b0
};

#endif // GRUNTZ_GRUNTZ_CTELESPRITEFACTORY_H
