// Grunt.h - the engine's CGrunt (the player/enemy grunt entity), the HUD
// sprite-creator cluster only. Field names are placeholders (m_<hexoffset>);
// ONLY the OFFSETS + code bytes are load-bearing (campaign doctrine). CGrunt is
// huge; this header models ONLY the member offsets the 7 HUD sprite creators
// touch + the small external classes they call into.
//
// The 7 creators (CGrunt::Create*Sprite @0x04d130..0x04d730) all share one
// shape: bail if the target sprite slot is already populated (some also gate on
// a stat/flags member), then ask the global HUD sprite factory to build a named
// sprite (the factory is reached via *0x64556c -> +0x30 -> +0x8, a __thiscall
// taking the sprite class-name string + a couple of HUD-geometry values derived
// from this->m_10 (+0x5c / +0x60)), store the new sprite into the slot, run its
// slot-0x10 init virtual, then register it into the grunt's sprite collection
// (sprite->m_7c->m_18 . Add*(...)). If the register call fails, set a flag bit
// (0x10000) on the collection's record, null the slot, and return 0; else 1.
#ifndef SRC_GRUNTZ_GRUNT_H
#define SRC_GRUNTZ_GRUNT_H

// ---------------------------------------------------------------------------
// The receiver the Add* registration call runs on: edi = sprite->m_7c->m_18.
// On failure the creators do `edi = edi->m_38; edi->m_8 |= 0x10000`. The Add*
// methods are unmatched engine methods (reached via incremental-link thunks);
// declared external/no-body so their `call rel32` displacements reloc-mask.
//   AddA(a, b, c) @0x07f0d0  (Health/Stamina/ToyTime/WingzTime; 3 args)
//   AddB(a, b)    @0x07f920  (Toy; 2 args)
//   AddC(a, b, c) @0x080380  (Powerup; 3 args)
//   AddD(a, b)    @0x07e9c0  (Selected; 2 args)
// ---------------------------------------------------------------------------
struct CSpriteRegRecord {
    char m_pad0[0x8];
    unsigned m_8;       // +0x08  failure flag word (|= 0x10000)
};

class CSpriteRegistrar {
public:
    int AddA(int a, int b, int c);  // 0x07f0d0
    int AddB(int a, int b);         // 0x07f920
    int AddC(int a, int b, int c);  // 0x080380
    int AddD(int a, int b);         // 0x07e9c0

    char m_pad0[0x38];
    CSpriteRegRecord *m_38;         // +0x38  (failure-flag record holder)
};

// ---------------------------------------------------------------------------
// The HUD sprite the factory builds. The creators read sprite->m_7c (an inner
// object), call sprite->m_7c->[0x10](sprite) (init), and reach the registrar at
// sprite->m_7c->m_18. Layout-free apart from m_7c.
// ---------------------------------------------------------------------------
struct CSpriteInner {
    char m_pad0[0x10];
    void (*m_init)(void *self);     // +0x10  init virtual (called with sprite)
    char m_pad14[0x18 - 0x14];
    CSpriteRegistrar *m_18;         // +0x18  the registrar
};

struct CHudSprite {
    char m_pad0[0x7c];
    CSpriteInner *m_7c;             // +0x7c
};

// ---------------------------------------------------------------------------
// this->m_10 (a HUD/level geometry source). The factory's two geometry args are
// m_10->m_5c and m_10->m_60 (the latter optionally minus a per-sprite constant).
// ---------------------------------------------------------------------------
struct CGruntHud {
    char m_pad0[0x5c];
    int m_5c;       // +0x5c
    int m_60;       // +0x60
};

// ---------------------------------------------------------------------------
// The global HUD sprite factory, reached via *(void**)0x64556c -> +0x30 -> +0x8.
// CreateSprite is __thiscall(this, 0, geoB, geoA, hint, name, kind) ret 0x18
// (@0x1597b0). Modeled as a method on the registry singleton so the call shape
// (factory-this = g->m_30->m_8) + the 6-arg push fall out; external/no-body so
// the `call rel32` reloc-masks.
// ---------------------------------------------------------------------------
struct CSpriteFactory {
    CHudSprite *CreateSprite(int kind, int geoB, int geoA,
                             int hint, const char *name, int flags); // 0x1597b0
};

struct CSpriteFactoryHolder {
    char m_pad0[0x8];
    CSpriteFactory *m_8;    // +0x08
};

struct CGameRegistry {
    char m_pad0[0x30];
    CSpriteFactoryHolder *m_30;     // +0x30
};

// The global manager pointer at binary 0x64556c.
extern CGameRegistry *g_pGameRegistry;  // *(CGameRegistry**)0x64556c

// ---------------------------------------------------------------------------
// CGrunt - only the members the HUD sprite creators touch. CGrunt is large;
// this is a deliberately partial model (load-bearing offsets only).
//   +0x10   m_10      CGruntHud* (factory geometry source)
//   +0x1b8  m_selectedSprite   (CreateSelectedSprite)
//   +0x1bc  m_toySprite        (CreateToySprite)
//   +0x1c4  m_healthSprite     (CreateHealthSprite)
//   +0x1c8  m_staminaSprite    (CreateStaminaSprite; ToyTime clears it)
//   +0x1cc  m_toyTimeSprite    (CreateToyTimeSprite; WingzTime clears it)
//   +0x1d0  m_wingzTimeSprite  (CreateWingzTimeSprite; ToyTime clears it)
//   +0x1d4  m_powerupSprite    (CreatePowerupSprite)
//   +0x1ec  m_1ec / +0x1f0 m_1f0  (Add* args)
//   +0x238  m_238     (WingzTime gate)
//   +0x3ec  m_3ec     (Health stat / Add arg)
//   +0x3f0  m_3f0     (Stamina stat / gate / Add arg)
//   +0x3f4  m_3f4     (ToyTime gate / Add arg)
//   +0x3f8  m_3f8     (WingzTime gate / Add arg)
// ---------------------------------------------------------------------------
class CGrunt {
public:
    int CreateHealthSprite();       // 0x04d130
    int CreateToySprite();          // 0x04d220
    int CreateStaminaSprite();      // 0x04d2f0
    int CreateToyTimeSprite();      // 0x04d3e0
    int CreateWingzTimeSprite();    // 0x04d520
    int CreatePowerupSprite(int a); // 0x04d650 (ret 4)
    int CreateSelectedSprite();     // 0x04d730

    char        m_pad0[0x10];
    CGruntHud  *m_10;                       // +0x10
    char        m_pad14[0x1b8 - 0x14];
    CHudSprite *m_selectedSprite;           // +0x1b8
    CHudSprite *m_toySprite;                // +0x1bc
    char        m_pad1c0[0x1c4 - 0x1c0];
    CHudSprite *m_healthSprite;             // +0x1c4
    CHudSprite *m_staminaSprite;            // +0x1c8
    CHudSprite *m_toyTimeSprite;            // +0x1cc
    CHudSprite *m_wingzTimeSprite;          // +0x1d0
    CHudSprite *m_powerupSprite;            // +0x1d4
    char        m_pad1d8[0x1ec - 0x1d8];
    int         m_1ec;                      // +0x1ec
    int         m_1f0;                      // +0x1f0
    char        m_pad1f4[0x238 - 0x1f4];
    int         m_238;                      // +0x238
    char        m_pad23c[0x3ec - 0x23c];
    int         m_3ec;                      // +0x3ec
    int         m_3f0;                      // +0x3f0
    int         m_3f4;                      // +0x3f4
    int         m_3f8;                      // +0x3f8
};

#endif // SRC_GRUNTZ_GRUNT_H
