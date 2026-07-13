// IconLoaderViews.h - the EngineLabelBacklog icon-loader holder view, shared by
// IconLoaders.cpp (BuildBootyPerfectAnimation @0x1c070) and TriggerMgr.cpp (the
// four loaders 0x78960/0x7a3f0/0x7b330/0x7c620 the dossier-10b one-TU merge
// homes there). @identity-TODO: the free EngineLabelBacklog holder matches the
// stub's mangled namespace; the real owning class is unrecovered (its `this` is
// a game-state object with the factory holder at +0x22c).
#ifndef GRUNTZ_ICONLOADER_VIEWS_H
#define GRUNTZ_ICONLOADER_VIEWS_H

#include <Gruntz/GameRegistry.h> // CSpriteFactoryHolder (the +0x22c slot type)
#include <Gruntz/UserLogic.h>    // CGameObject (the created sprite)
#include <rva.h>

struct CResourceTracker {
    char m_pad0[0x1c];
    i32 m_levelNumber; // +0x1c  (Level number source)
};

// The free EngineLabelBacklog holder (matches the stub's mangled namespace).
class EngineLabelBacklog {
public:
    i32 LoadPowerupIconSprites(i32 type, i32 geoB, i32 geoA, i32 m130, i32 warpIdx, i32 m120);
    i32 LoadExplosionSprites(i32 geoB, i32 geoA, i32 variant, i32 dummy);
    i32 LoadCameraSprite();
    i32 LoadToyBoxIcon(i32 x, i32 y, i32 a3, i32 a4, i32 a5);
    // (BuildBootyPerfectAnimation @0x1c070 is GONE from here - it is CBootyState::, proven
    // by its sole caller 0x18830 = CBootyState vtable slot 1. Its sprite lives at +0x2f8.)

    char m_pad00[0x22c];                   // +0x000
    CSpriteFactoryHolder* m_factoryHolder; // +0x22c  sprite-factory holder
    char m_pad230[0x23c - 0x230];          // +0x230
    CGameObject* m_cameraSprite;           // +0x23c  cached camera sprite
    char m_pad240[0x2f8 - 0x240];          // +0x240
    CGameObject* m_bootyPerfectSprite;     // +0x2f8  booty-perfect anim sprite
};

SIZE_UNKNOWN(CResourceTracker);
SIZE_UNKNOWN(EngineLabelBacklog);

#endif // GRUNTZ_ICONLOADER_VIEWS_H
