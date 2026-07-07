// Sprite.h - the ONE shape for the sprite value-object family (CSprite) and its
// name->object hash table (CSpriteHashTable), shared across the sprite/HUD/timer
// loaders. Previously each TU (ActionOptionsMenuBar / KitchenSlime / SpriteResource
// / StatusBarUpdaters / SpriteLoaders) carried its own divergent partial view of
// the same retail classes; this is the single reconstructed layout.
//
// Two distinct retail classes were both spelled "CSprite" across the tree:
//   * the FRAME-DATA sprite here (the value CSpriteHashTable::Lookup returns) --
//     a CObArray of frame-workers at +0x10 and an inclusive valid frame range
//     [m_64..m_68], reached by all the name-lookup loaders; and
//   * the factory-created HUD/anim OWNER sprite (config block +0x114..+0x130, an
//     init sub-table at +0x7c), which reads its +0xc as the resource manager --
//     that one is IconLoaders.cpp's CIconSprite / Grunt.h's CHudSprite, NOT this.
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_SPRITE_H
#define GRUNTZ_SPRITE_H

#include <Ints.h>
#include <rva.h>

struct CSprite;
struct CFrameWorker;
struct CObject;
struct CFrameGrid; // the frame-grid value the image registry's map yields (CPlay::m_grid)

// The engine string-keyed sprite-set hash table embedded at a registry's +0x10
// (the `add ecx,0x10` before the call addresses it). Lookup() hashes the class-name
// key and writes the found object through *ppOut, returning a found-flag. Modeled
// with NO body so the `ecx=<map>; call <helper>` shape reloc-masks (engine 0x1b8008).
// The map stores CObject-derived values; overloads type the out-ptr per consumer so
// the found value is typed cast-free (same reloc-masked call either way).
class CSpriteHashTable {
public:
    // Lookup @0x1b8438 IS CMapStringToOb::Lookup; cast at each call.
};

// The CSprite frame table is a CObArray of CImage frame-workers living AT
// CSprite+0x10 (so its m_pData is +0x14, its m_nSize +0x18). The frame INSERT
// (0x151f00) grows it via SetAtGrow and the frame READ indexes m_pData directly.
// SetAtGrow(index, element): __thiscall ret 8, no body -> the call reloc-masks
// against the engine CObArray helper @0x1b5822.
SIZE_UNKNOWN(CFrameArray);
struct CFrameArray {
    void* m_vptr;   // +0x00  CObject vftable
    i32** m_pData;  // +0x04  frame-pointer table
    i32 m_nSize;    // +0x08  element count
    i32 m_nMaxSize; // +0x0c
    i32 m_nGrowBy;  // +0x10
    // (SetAtGrow @0x1b5822 IS MFC CObArray::SetAtGrow - the frame array IS a CObArray;
    // reached via a CObArray cast at the one call site, SpriteResource.cpp.)
};

// The engine sprite (animation frame-data) object: the value the sprite hash
// table resolves. m_c is the parent context handed to each frame worker; the
// frame CObArray lives at +0x10; the inclusive valid frame range is [m_64..m_68].
SIZE_UNKNOWN(CSprite);
struct CSprite {
    char m_pad00[0xc];
    void* m_c;                // +0x0c  parent context handed to each frame worker
    CFrameArray m_frames;     // +0x10  frame CObArray (m_pData @+0x14, m_nSize @+0x18)
    char m_name[0x64 - 0x24]; // +0x24  registry/config name (the sprite's lookup key)
    i32 m_firstFrame;         // +0x64  first valid frame number
    i32 m_lastFrame;          // +0x68  last valid frame number

    // Insert a frame worker at frame number `n` (0x151f00); bounds-read a frame
    // pointer (0x15cc30). Bodies live in the spriteresource unit.
    CFrameWorker* InsertFrame(void* src, i32 n, i32 mode); // 0x151f00
    i32 GetFrame(i32 n);                                   // 0x15cc30
};

#endif // GRUNTZ_SPRITE_H
