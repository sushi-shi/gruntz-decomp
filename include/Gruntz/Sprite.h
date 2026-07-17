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

// ============================================================================
// CSprite IS CDDrawWorker - stage 4 of the fold, DONE. It survives as a typedef so
// the ~177 call sites keep reading in their own domain language (the tree's own
// CMoviePlayer precedent: CDDScreen/CDDPageMgr are typedefs of it), while there is
// exactly ONE type, ONE layout and ONE vtable behind them.
//
// The ex `CFrameArray` at +0x10 is GONE: it was the real MFC ::CObArray all along -
// this view even cast its own array to `(CObArray*)` to call SetAtGrow @0x1b5822.
// Frame access now goes through the real API ((CImage*)x->m_items.GetAt(i) /
// GetSize()), the spelling WwdGameObject.cpp already matched with, because MFC's
// m_pData/m_nSize are protected.
//
// The ex `m_c`/`m_owner` (+0x0c) is CLoadable::m_0c, reached through the typed
// CDDrawWorker::Owner() accessor - see the note there for why the type lives on the
// leaf and not on the shared base.
//
// (CImageSet is the SAME object a third time - stage 5, still open. See the
// fold-state note below.)
#include <DDrawMgr/DDrawWorker.h> // the ONE real class (vtbl 0x1efbe8, CLoadable-derived)

typedef CDDrawWorker CSprite;

#endif // GRUNTZ_SPRITE_H
