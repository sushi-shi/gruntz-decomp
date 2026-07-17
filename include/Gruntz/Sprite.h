// Sprite.h - the ONE shape for the sprite value-object family (CSprite) and its
// name->object hash table (CMapStringToOb), shared across the sprite/HUD/timer
// loaders. Previously each TU (ActionOptionsMenuBar / KitchenSlime / SpriteResource
// / StatusBarUpdaters / SpriteLoaders) carried its own divergent partial view of
// the same retail classes; this is the single reconstructed layout.
//
// Two distinct retail classes were both spelled "CSprite" across the tree:
//   * the FRAME-DATA sprite here (the value CMapStringToOb::Lookup returns) --
//     a CObArray of frame-workers at +0x10 and an inclusive valid frame range
//     [m_64..m_68], reached by all the name-lookup loaders; and
//   * the factory-created HUD/anim OWNER sprite (config block +0x114..+0x130, an
//     init sub-table at +0x7c), which reads its +0xc as the resource manager --
//     that one is IconLoaders.cpp's CIconSprite / Grunt.h's CHudSprite, NOT this.
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
// ============================================================================
// FOLD IN PROGRESS - CSprite / CImageSet / CDDrawWorker ARE ONE RETAIL CLASS.
// Stage 1-2 (naming) done; the type substitution is the remaining work.
//
// PROOF (all three read from the binary, no prose):
//   * CDDrawWorker's vtable is ??_7CDDrawWorker@@6B@ @RVA 0x1efbe8 = VA 0x5efbe8,
//     and ImageSet.h independently recorded CImageSet's vtable as @0x5efbe8 - the
//     SAME datum (that line was prose nobody had re-measured).
//   * That vtable's own slots are held by BOTH views: [11] 0x152110 / [12] 0x152060 /
//     [13] 0x151fb0 are CImageSet::CreateFrame24/28/30 and [14] 0x151f00 is
//     CSprite::InsertFrame - which is why 4 WIRING rows sit in the slot-binding
//     baseline (the bodies exist but are declared on the views, so the vtable's
//     relocs dangle onto non-virtual symbols of another class).
//   * SIZE 0x6c is annotated independently on CDDrawWorker and CImageSet, and all
//     three layouts agree offset-for-offset (owner +0x0c, CObArray +0x10 with
//     m_pData +0x14 / m_nSize +0x18, name buffer +0x24, [min,max] +0x64/+0x68).
//   * The seed pattern proves the range's meaning: DeleteAll seeds +0x64 = 99999
//     and +0x68 = 0 (min=+inf / max=0) so the first insert widens [min,max].
//
// REMAINING (measured USR-exact with gruntz.analysis.rename_member, NOT grep):
//   CSprite  177 member sites / 16 files;  CImageSet 188 / 17.  Plus 11 RVA-bound
//   methods to re-home and two decisions:
//     (a) frame access must go through the REAL MFC ::CObArray API
//         ((CImage*)m_items.GetAt(i) / m_items.GetSize()) because MFC's m_pData is
//         protected - the spelling WwdGameObject.cpp already matches with;
//     (b) +0x0c is CLoadable::m_0c (i32) on the canonical but CImageParent* on both
//         views - retyping the shared base would hit every CLoadable derivative, so
//         the merged class needs its own typed accessor, not a cast at 4 sites.
// ============================================================================
#ifndef GRUNTZ_SPRITE_H
#define GRUNTZ_SPRITE_H

#include <Ints.h>
#include <Mfc.h> // the registries' +0x10 map IS the real MFC CMapStringToOb
#include <rva.h>

struct CSprite;
class CImage; // the frame element IS the real CImage (Image/CImage.h)
struct CObject;
// (the ex-`CFrameGrid` map value IS the canonical CImageSet - CPlay::m_grid; see Play.h)

// The engine string-keyed sprite-set hash table embedded at a registry's +0x10
// (the `add ecx,0x10` before the call addresses it). Lookup() hashes the class-name
// key and writes the found object through *ppOut, returning a found-flag. Modeled
// with NO body so the `ecx=<map>; call <helper>` shape reloc-masks (engine 0x1b8008).
// The map stores CObject-derived values; overloads type the out-ptr per consumer so
// the found value is typed cast-free (same reloc-masked call either way).
// (The ex-`CMapStringToOb` view is DISSOLVED: it was an EMPTY phantom whose only
// "method" was a fake alias of the MFC library CMapStringToOb::Lookup @0x1b8438, so every
// call through it needed an object-cast AND bound to nothing. The registries' +0x10 map
// members are now the real CMapStringToOb.)

// The CSprite frame table is a CObArray of CImage frame-workers living AT
// CSprite+0x10 (so its m_pData is +0x14, its m_nSize +0x18). The frame INSERT
// (0x151f00) grows it via SetAtGrow and the frame READ indexes m_pData directly.
// SetAtGrow(index, element): __thiscall ret 8, no body -> the call reloc-masks
// against the engine CObArray helper @0x1b5822.
SIZE_UNKNOWN(CFrameArray);
struct CFrameArray {
    char _vft0[4];   // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    CImage** m_pData; // +0x04  frame-pointer table (the element IS the real CImage)
    i32 m_nSize;     // +0x08  element count
    i32 m_nMaxSize; // +0x0c
    i32 m_nGrowBy;  // +0x10
    void SetAtGrow(i32 index, CObject* element); // 0x1b5822
};

// The engine sprite (animation frame-data) object: the value the sprite hash
// table resolves. m_c is the parent context handed to each frame worker; the
// frame CObArray lives at +0x10; the inclusive valid frame range is [m_64..m_68].
// The ex "gated-lookup table" views CRegTypeTable/CStatzGlyphMap/CMgrLookupRec
// (elements @+0x14, name @+0x24, bounds @+0x64/+0x68) were all THIS class,
// resolved from the same m_imageRegistry->m_10map the SBI_Image loader uses.
class CImageParent; // the parent context handed to each frame worker (== CImage::m_parent)

SIZE_UNKNOWN(CSprite);
struct CSprite {
    char m_pad00[0xc];
    CImageParent* m_owner; // +0x0c  parent context handed to each frame worker (CImage frame m_parent)
    CFrameArray m_items;     // +0x10  frame CObArray (m_pData @+0x14, m_nSize @+0x18)
    char m_name[0x64 - 0x24]; // +0x24  registry/config name (the sprite's lookup key)
    i32 m_minIndex;         // +0x64  first valid frame number
    i32 m_maxIndex;          // +0x68  last valid frame number

    // Insert a frame worker at frame number `n` (0x151f00); bounds-read a frame
    // pointer (0x15cc30). Bodies live in the spriteresource unit.
    CImage* InsertFrame(void* src, i32 n, i32 mode); // 0x151f00
    i32 GetFrame(i32 n);                             // 0x15cc30 (out-of-line)
};

#endif // GRUNTZ_SPRITE_H
