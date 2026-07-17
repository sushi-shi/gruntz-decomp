// ImageSet.h - CImageSet, the engine's sparse image-frame collection.
//
// A CImageSet owns an array of frame pointers addressed by a signed frame index
// that runs over the inclusive range [m_minIndex, m_maxIndex]; the set carries an
// inline name buffer and a CObArray-style backing store at +0x10. Only the three
// leaf accessors in ImageSet.cpp are matched here - the FUN_* labels carry no real
// name, so the class name (CImageSet) and field names are descriptive placeholders
// recovered from usage; the OFFSETS + code bytes are the load-bearing facts (object
// size 0x6c, vtable @0x5efbe8, frame array @0x14 / count @0x18 / inline name @0x24 /
// index range @0x64,0x68). NOTE: that "vtable @0x5efbe8" is CDDrawWorker's own
// ??_7 (RVA 0x1efbe8) - this class IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
// see the fold-state note in <Gruntz/Sprite.h>.
//
// The frame element IS the RTTI CImage (`.?AVCImage@@`, vtable @0x5eaa2c). The old
// "+0x30 does NOT line up" wall (which kept a CImageFrame/CImageFormat/
// CImageFrameSurface placeholder trio in a now-deleted Image/ImageFrame.h) is
// FALSIFIED: the frame's "+0x30 format helper" IS CImage's +0x30 owned object, the
// CDDrawShadeBlit shaded sprite -
//   format +0x10 "decoded byte count"  == CDDrawShadeBlit::m_rleLen  (RLE byte count;
//        GetMemoryUsage's own comment already called it "the owned object's count")
//   format +0x14 "shade/type state"    == m_drawType (ctor default 1; the minor-cheat
//        toggles set it to 2/3 via SetAllTypes -> the row-convert selector)
//   format +0x18 "SetAllField18 field" == m_light   (ctor default 0x80; CheatEclipse
//        writes rand() % 256 into it - a random LIGHT LEVEL, 0..255)
//   format +0x1c "resolved format"     == m_palDescr (a ShadeDescr*; every SetAllFormats
//        caller casts a shade-table POINTER into the i32 param)
//   and the helper's SetType @0x14dd90 IS the same __thiscall body the blitter calls
//        (it writes [ecx+0x14]=mode and [ecx+0x1c]=descr - m_drawType/m_palDescr).
// Likewise the frame's held +0x2c surface is the real CDDSurface (its +0xa8 bit depth
// was the whole of the CImageFrameSurface placeholder).
#ifndef SRC_IMAGE_IMAGESET_H
#define SRC_IMAGE_IMAGESET_H
#include <rva.h>

#include <Ints.h>

#include <Image/CImage.h> // the frame element IS the RTTI CImage (was the CImageFrame view)

// Object size 0x6c (recorded in the header note above; the body computes exactly that:
// m_maxIndex at +0x68). The SIZE line used to sit on the GameLevel.h class of the same
// name - that class is CTileImageSet now, so it lives here, on the class it describes.
// ============================================================================
// CImageSet IS CDDrawWorker - stage 5 of the fold, DONE. The third view of the same
// 0x6c object is dissolved; it survives as a typedef so call sites keep reading in
// their own domain language while ONE type/layout/vtable stands behind them.
//
// This header's own prose had recorded the proof without it ever being acted on:
// "vtable @0x5efbe8" IS ??_7CDDrawWorker@@6B@ (RVA 0x1efbe8). The three slots this
// view declared as CreateFrame24/28/30 are that vtable's own [11]/[12]/[13], which is
// why they sat in the slot-binding baseline as WIRING rows; they are real virtuals of
// CDDrawWorker now, and those rows are retired.
//
// The +0x10/+0x14/+0x18 trio (m_array/m_frames/m_count) was the real MFC ::CObArray
// all along - vptr/m_pData/m_nSize - so frame access goes through the real API
// ((CImage*)x->m_items.GetAt(i) / GetSize()); m_pData is protected. The +0x0c m_owner
// is reached through CDDrawWorker::Owner(). SIZE 0x6c was already annotated on BOTH
// classes independently - the same object, recorded twice.
#include <DDrawMgr/DDrawWorker.h> // the ONE real class (vtbl 0x1efbe8, CLoadable-derived)

typedef CDDrawWorker CImageSet;

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // SRC_IMAGE_IMAGESET_H
