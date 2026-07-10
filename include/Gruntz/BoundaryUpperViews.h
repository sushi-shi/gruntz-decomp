// BoundaryUpperViews.h - shared referent/owner views for the upper-half
#include <Wap32/Object.h> // CObject grand-base (real virtual dtor)
// (RVA >= 0x133370) engine_boundary leaf methods reconstructed in BoundaryUpper.cpp
// (DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). Formerly per-TU inline views; consolidating them here is pure code
// motion (matching-neutral) and gives the /GX EH-frame sibling (BoundaryUpperEh.cpp)
// / a final sweep one definition to reuse.
#ifndef GRUNTZ_BOUNDARYUPPERVIEWS_H
#define GRUNTZ_BOUNDARYUPPERVIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Blk6c.h> // the 0x6c-byte CImageOwned transform descriptor

// Embedded base-subobject vptr restamp (member dtor of the grand-base): the 7-byte
// `mov [this],&g_wapObjectDtorVtbl; ret` leaf. Three distinct leaf classes share it.
struct SW_161460 : CObject {};
SIZE_UNKNOWN(SW_161460);
struct SW_161560 : CObject {};
SIZE_UNKNOWN(SW_161560);
struct SW_163a10 : CObject {};
SIZE_UNKNOWN(SW_163a10);

// (0x1413c0 re-homed to CDDSurface::Scale in src/Image/Image.cpp; view dissolved.)

// 0x1614b0 - `if(m_14) RezFree(m_14); m_14 = 0;` (CImageSet1-area buffer release).
struct B_1614b0 {
    i32 _0[5];
    void* m_14; // 0x14
    void Release();
};
SIZE_UNKNOWN(B_1614b0);

// (0x137300 SoundDevice::GetPrimary re-homed to src/Dsndmgr/DirectSoundMgr.cpp;
// SoundDevice view dissolved onto the canonical <Dsndmgr/SoundDevice.h>.)

// (0x1433d0 Compare + the 0x1434c0/70/143510/90 mode searches re-homed to
// CDirectDrawMgr::Compare/Find* in src/DDrawMgr/DirectDrawMgr.cpp; DdOb/DdEntry/
// ModeArr/Pair2 views dissolved onto CDdMode/CDdModePair + m_poolItems there.)

// (0x1847a0 re-homed to CMenuItem2::SetFrame in src/Gruntz/MenuItem2.cpp; view dissolved.)

// (0x17fc40 re-homed to C17f9f0::Free (src/Stub/BoundaryUpperEh.cpp); view dissolved.)

// (0x134360/0x1346d0 device-config teardowns re-homed to DirectInputMgr2.cpp onto
// CDeviceConfigB/CDeviceConfigC; the DevCfg view is dissolved.)

// (0x1413b0 re-homed to CDDSurface::UnlockThunk in src/Image/Image.cpp; the Obj1413
// manual-vtable view IS the real m_8 IDirectDrawSurface (Unlock @slot 0x80); dissolved.)

// (ModeArr/DdEntry re-homed - see the Compare note above.)

// 0x13dee0 - `m_1c = v; if(v > 0) m_28 = 1000 / v;` (CFileImage frame timing).
struct B_13dee0 {
    char _0[0x1c];
    i32 m_1c; // 0x1c
    char _20[0x28 - 0x20];
    i32 m_28; // 0x28
    void Set(i32 v);
    i32 TrySet(i32 v); // 0x13df00
};
SIZE_UNKNOWN(B_13dee0);

// 0x13ee30 - COM wait-flip loop (IDirectDrawSurface-style manual vtable, slot 0x48).
struct IDDS_ee30 { // real polymorphic; Flip is slot 18 (+0x48)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual u32 __stdcall Flip(i32); // slot 18 (+0x48)
};
SIZE_UNKNOWN(IDDS_ee30);
struct B_13ee30 {
    char _0[8];
    IDDS_ee30* m_8; // 0x8
    void WaitFlip();
};
SIZE_UNKNOWN(B_13ee30);

// (0x151e70 re-homed to AnimWorkerObj::Clear in src/DDrawMgr/DDrawWorkerCache.cpp; view dissolved.)

// (0x166810 re-homed to CWwdGameObjectB::Clear_166810 in src/Wwd/WwdGameObject.cpp; view dissolved.)

// (0x13c8a0 CRezItm::Scan re-homed to src/Rez/RezMgr.cpp; RezItm/RezOwner views
// dissolved onto CRezItm/CRezItmOwner.)

// (0x13c8f0 CRezItm::Check re-homed to src/Rez/RezMgr.cpp; RezDir view dissolved onto
// the canonical CRezItm.)

// (0x138f20 re-homed to CGruntzSoundInnerZ::Retrigger in src/Dsndmgr/GruntzSoundZ.cpp; view dissolved.)

// 0x16be60 - ButeMgr helper append.
struct C16be60 {
    i32 Ready();                     // 0x16bd10
    void Emit(const void* s, i32 a); // 0x16c2d0
    void Flush();                    // 0x16bd90
    C16be60* Append(i32 arg);
};
SIZE_UNKNOWN(C16be60);

// 0x151d20 - notify a hooked callback (stash/replace m_7c->m_1c).
struct Cb151d20 {
    char _0[0x10];
    void(__cdecl* fn)(void*); // +0x10
    char _14[0x1c - 0x14];
    void* m_1c; // 0x1c
};
SIZE_UNKNOWN(Cb151d20);
struct B_151d20 {
    char _0[0x7c];
    Cb151d20* m_7c; // 0x7c
    i32 Notify(void* arg);
};
SIZE_UNKNOWN(B_151d20);

// (ClearImageCache/ClearModeArray tail-forwards re-homed to Image.cpp / DirectDrawMgr.cpp;
// their CImageCache / CDdObArray views live on those TUs' own headers now.)

// (ImgOwned apply/setup cluster re-homed: 0x13e0a0 = CDDSurface::Init1 (slot 2,
// Image.cpp), 0x148b50/0x148cc0 = CPoolItemAB8/AE8::Init1 overrides (DDrawPtrCollections.cpp).
// The view IS CDDSurface; dissolved onto the real classes + their Init1 decls.)

// (0x13dec0 RezMgr::SpinWaitUntil re-homed to src/Rez/RezMgr.cpp; RezMgr view dissolved.)

// --- vtable catalog ---

#endif // GRUNTZ_BOUNDARYUPPERVIEWS_H
