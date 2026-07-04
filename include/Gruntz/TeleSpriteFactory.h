// TeleSpriteFactory.h - the HUD/visual sprite factory reached as mgr->m_30->m_8.
// This IS the canonical CSpriteFactory (CreateSprite @0x1597b0, <Gruntz/SpriteFactory.h>):
// CTeleSpriteFactory is kept only as a name alias for the teleporter call sites
// (UserLogic.cpp's CTeleResHolder::m_8). The produced sprite is the shared engine game
// object (both callers cast the CreateSprite result to CGameObject*).
#ifndef GRUNTZ_GRUNTZ_CTELESPRITEFACTORY_H
#define GRUNTZ_GRUNTZ_CTELESPRITEFACTORY_H

#include <Gruntz/SpriteFactory.h>

typedef CSpriteFactory CTeleSpriteFactory;

#endif // GRUNTZ_GRUNTZ_CTELESPRITEFACTORY_H
