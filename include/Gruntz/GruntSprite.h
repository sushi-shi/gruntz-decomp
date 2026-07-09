// GruntSprite.h - the canonical sprite/anim-player facet classes (C:\Proj\Gruntz).
//
// A created game sprite (CSpriteFactory::CreateSprite @0x1597b0) is one object; the
// engine reaches its frame-cache + geometry behaviour through two facet classes whose
// methods live in SpriteResource.cpp:
//
//   CGruntSprite      - the frame-cache facet: looks a named sprite up through the
//                       resource holder m_c and caches the frame/pointer.
//   CGruntAnimPlayer  - the geometry facet (derives CGruntSprite): applies a looked-up
//                       or direct geometry source to the +0x1a0 geometry sub-player.
//
// Layout is byte-proven from the method bodies: CGruntSprite's frame fields sit at
// +0x190..+0x198 and CGruntAnimPlayer's geometry fields at +0x19c..+0x1a0, so the
// facets are a clean single-inheritance chain (base = CGruntSprite). NB: there is NO
// retail RTTI for these names - they are the reconstruction's chosen facet names for
// the shared game-sprite object (the polymorphic CGameObject / CGrunt game logic reach
// the SAME methods but cannot cleanly DERIVE these non-polymorphic facets because their
// own vptr@0 would displace the base; those consumers remain a separate reconciliation).
#ifndef GRUNTZ_GRUNTSPRITE_H
#define GRUNTZ_GRUNTSPRITE_H

#include <Ints.h>

class CResMgr; // +0x0c the resource holder (image/sprite/anim registries)
class CSprite; // the looked-up frame-data sprite

// The +0x1a0 geometry sub-player (a CDDrawBlitParam / CAniAdvanceCursor is cast onto
// its address in the method bodies); data-less facet, only its address is taken.
class CGruntAnimSub2 {
public:
};

// ---------------------------------------------------------------------------
// CGruntSprite - the frame-cache facet (base).
// ---------------------------------------------------------------------------
class CGruntSprite {
public:
    void CacheFirstFrame(const char* name);       // 0x150540
    void CacheFrame(const char* name, i32 frame); // 0x1504d0

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c  resource holder (Lookup source)
    char m_pad10[0x190 - 0x10];
    i32 m_frameNum;    // +0x190  cached first/requested frame number
    CSprite* m_sprite; // +0x194  the looked-up sprite
    i32* m_framePtr;   // +0x198  the frame's pointer (or 0)
};

// ---------------------------------------------------------------------------
// CGruntAnimPlayer - the geometry facet (adds the geometry sub-player fields).
// ---------------------------------------------------------------------------
class CGruntAnimPlayer : public CGruntSprite {
public:
    i32 ApplyLookupGeometry(const char* name, i32 applyDefault); // 0x1505b0
    void ApplyGeometryDirect(i32 srcSprite, i32 applyDefault);   // 0x58b60
    i32 LookupAnimSprite(const char* name);                      // 0x150610

    CSprite* m_19c;       // +0x19c  cached looked-up sprite (geometry path)
    CGruntAnimSub2 m_1a0; // +0x1a0  geometry sub-player
};

#endif // GRUNTZ_GRUNTSPRITE_H
