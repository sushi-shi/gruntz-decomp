// ResMgr.h - the ONE shape for the game resource/level manager (CResMgr) and the
// three named-object registries it owns. Previously ActionOptionsMenuBar.cpp,
// SpriteResource.cpp, SpriteLoaders.cpp and CPlay.cpp each carried a divergent
// partial view of the same retail CResMgr; this is the single reconstructed layout.
//
// CResMgr holds three DISTINCT registry classes (their Has()/Register()/Install()
// helpers sit at different retail addresses, so they are genuinely different
// types, not one generic manager): the image/tile registry at +0x10, the sound
// registry at +0x28 and the animation registry at +0x2c. Each registry embeds the
// same name->object hash table at ITS +0x10 (m_10map). The sprite/HUD loaders
// reach a sprite through <registry>->m_10map.Lookup(); the level loader (CPlay)
// drives the registries' install helpers.
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_RESMGR_H
#define GRUNTZ_RESMGR_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Sprite.h> // CSpriteHashTable (each registry's +0x10 map)

struct CMenuViewObj; // the level/view object at CResMgr+0x24 (ActionOptionsMenuBar-local)

// The active draw surface at CResMgr+0x04: +0x14 is the draw context.
SIZE_UNKNOWN(CDrawTarget);
struct CDrawTarget {
    char m_pad00[0x14];
    i32 m_drawContext; // +0x14  draw context
};

// The world/key lookup table at CResMgr+0x08 the timer-expiry path probes
// (FindByKey / engine "Lookup" @0x1b8760, reached at +0x48). Modeled NO-body.
SIZE_UNKNOWN(CKeyTable);
struct CKeyTable {
    i32 FindByKey(i32 key, i32* outFound);
};

// The image/tile registry at CResMgr+0x10: a virtual Install at vtable slot 18
// (+0x48) plus non-virtual Has/Register helpers, and the name->sprite hash table
// embedded at its own +0x10. All methods external/no-body so the calls reloc-mask.
SIZE_UNKNOWN(CImageRegistry);
struct CImageRegistry {
    i32 Has(char* szName);                    // 0x155550 __thiscall, ret found
    void Register(char* szName, char* szKey); // 0x155360 __thiscall
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void Install(void* set, char* szName, char* szKey); // slot 18 (+0x48)

    char m_pad04[0x10 - 0x4]; // vptr occupies +0x00..+0x03
    CSpriteHashTable m_10map; // +0x10  the name->sprite hash table
};

// The sound registry at CResMgr+0x28 (plain non-virtual helpers) + its +0x10 map.
SIZE_UNKNOWN(CSoundRegistry);
struct CSoundRegistry {
    i32 Has(char* szName);                              // 0x1583c0 __thiscall, ret found
    void Register(char* szName, char* szKey);           // 0x157c70 __thiscall
    void Install(void* set, char* szName, char* szKey); // 0x157ee0 __thiscall
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
};

// The animation registry at CResMgr+0x2c (plain non-virtual helpers) + its +0x10 map.
SIZE_UNKNOWN(CAnimRegistry);
struct CAnimRegistry {
    i32 Has(char* szName);                              // 0x152c50 __thiscall, ret found
    void Install(void* set, char* szName, char* szKey); // 0x152ad0 __thiscall
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
};

// The game's resource/level manager. Different callers pull the manager they need
// out of its slots: the draw target (+0x04), the world key table (+0x08), the
// image registry (+0x10), a second sprite-set registry used by CreateSprite (+0x14),
// the level/view object (+0x24), the sound registry (+0x28) and the anim registry
// (+0x2c).
SIZE_UNKNOWN(CResMgr);
struct CResMgr {
    char m_pad00[0x4];
    CDrawTarget* m_drawTarget; // +0x04  active draw surface
    CKeyTable* m_8;            // +0x08  world/key lookup table
    char m_pad0c[0x10 - 0xc];
    CImageRegistry* m_10; // +0x10  image/tile registry (sprite lookups + install)
    CImageRegistry* m_14; // +0x14  sprite-set registry (CreateSprite lookup)
    char m_pad18[0x24 - 0x18];
    CMenuViewObj* m_view; // +0x24  level/view object
    CSoundRegistry* m_28; // +0x28  sound registry
    CAnimRegistry* m_2c;  // +0x2c  animation registry
};

#endif // GRUNTZ_RESMGR_H
