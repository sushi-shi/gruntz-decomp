// DemoSetup.cpp - the attract/demo-mode actor seeding (C:\Proj\Gruntz).
//
// SetupDemoActors @0x3c070 spawns the two demo HUD sprites ("DemoMover",
// "DemoSign") through the bound world's sprite factory. Field names are
// placeholders; only OFFSETS + code bytes are load-bearing.
#include <rva.h>

// The global HUD sprite factory (CSpriteFactory::CreateSprite @0x1597b0, 6-arg
// __thiscall, modeled NO-body so the call reloc-masks; see SpriteResource.cpp).
struct CSprite;
struct CSpriteFactory {
    CSprite* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
};

// this->m_c is the bound world object; its +0x8 is the sprite factory.
struct CDemoWorld {
    char m_pad0[0x8];
    CSpriteFactory* m_8; // +0x8
};

class CDemoSetup {
public:
    i32 SetupDemoActors(); // 0x3c070
    char m_pad0[0xc];
    CDemoWorld* m_c; // +0xc  bound world
};

// 0x3c070: seed the two attract-mode demo sprites via the world's sprite factory,
// then report success.
RVA(0x0003c070, 0x47)
i32 CDemoSetup::SetupDemoActors() {
    m_c->m_8->CreateSprite(1, 0, 0, 0, "DemoMover", 0x40003);
    m_c->m_8->CreateSprite(1, 0, 0, 0x270f, "DemoSign", 0x40003);
    return 1;
}
