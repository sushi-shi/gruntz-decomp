// SpriteFactory.h - the global HUD/game sprite factory (C:\Proj\Gruntz).
//
// ONE class, ONE definition (previously duplicated with three divergent inline
// views across SpriteResource.cpp / DemoSetup.cpp / IconLoaders.cpp).
// CreateSprite (@0x1597b0) looks a sprite TEMPLATE up by class-NAME (the 5th arg)
// in the factory's sprite-set table (m_c->m_14->map) and forwards the build args +
// the resolved template to CreateSpriteImpl (@0x159600), which `new`s the
// 0x1dc-byte game-sprite INSTANCE and constructs it. The created instance is
// modeled as CIconSprite (the rich HUD/anim sprite; IconLoaders.cpp defines it):
// that is the created object's real type, so CreateSprite returns CIconSprite*.
// The lookup RESULT (the frame-data template) is the separate CSprite value type.
//
// The factory also owns the live-icon list at +0x14 (walked by LoadToyBoxIcon to
// de-dup by class + tile). Only offsets + code bytes are load-bearing; the field
// names are placeholders for the engine identities recovered from the call sites.
//
// FOLLOW-UP (not this batch): <Gruntz/Grunt.h> still carries an independent
// caller-decl of CSpriteFactory whose CreateSprite returns CHudSprite* - a second
// placeholder NAME for the same created instance this header calls CIconSprite. It
// is not a layout conflict (fieldless caller-view), only a created-instance
// type-NAME divergence; unifying CHudSprite / CIconSprite across the CGrunt TUs that
// include Grunt.h is a separate naming sweep, so Grunt.h is left as-is for now.
#ifndef GRUNTZ_SPRITEFACTORY_H
#define GRUNTZ_SPRITEFACTORY_H

#include <Ints.h>
#include <rva.h>

struct CResMgr;     // resource mgr at +0xc (m_14 = the factory's sprite-set registry)
struct CSprite;     // frame-data template value (the lookup RESULT)
struct CIconSprite; // the created 0x1dc-byte game-sprite INSTANCE (IconLoaders.cpp)
// AttachSprite's already-allocated target; __single_inheritance keeps the init PMF a
// 4-byte code pointer (else MSVC's general PMF emits a this-adjust thunk).
struct __single_inheritance CSprite2;

// An entry in the factory's live-icon list (m_liveIcons): next-link @+0, sprite @+8.
struct CSpriteIconNode {
    CSpriteIconNode* next; // +0x00
    char m_pad4[0x8 - 0x4];
    CIconSprite* m_sprite; // +0x08
};
SIZE_UNKNOWN(CSpriteIconNode);

class CSpriteFactory {
public:
    // Public entry: look the template up by class-NAME, forward to the impl. __thiscall,
    // ret 0x18. Returns the created instance (or 0 if the template is missing).
    CIconSprite* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
    // The real allocator/ctor @0x159600 (external/no-body so the call reloc-masks).
    CIconSprite* CreateSpriteImpl(i32 kind, i32 geoB, i32 geoA, i32 hint, CSprite* tmpl, i32 flags);
    // Initialise an already-allocated sprite against a named template. __thiscall, ret 0x18.
    i32 AttachSprite(CSprite2* obj, i32 a1, i32 a2, i32 a3, const char* name, i32 flags);
    void AddChild(CSprite2* obj, i32 flag); // 0x159e40 (external/no-body)

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c
    char m_pad10[0x14 - 0x10];
    CSpriteIconNode* m_liveIcons; // +0x14  live-icon list head
};
SIZE_UNKNOWN(CSpriteFactory);

#endif // GRUNTZ_SPRITEFACTORY_H
