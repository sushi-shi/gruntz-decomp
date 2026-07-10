// BoundaryUpper.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog. Most of
// the original 24 @orphan leaves have been RE-HOMED to their real classes (see the
// "re-homed to ..." notes below; MSVC5 has no /OPT:ICF, so none were COMDAT-folded
// duplicates - each was a real function with one real home, found via xref --tree
// through the ILT thunks + the RTTI vtable-slot data-refs). Only OFFSETS + the code
// shape are load-bearing (campaign doctrine). Non-EH (base /O2) bodies only; the /GX
// EH-frame siblings live in BoundaryUpperEh.cpp. The residual leaves each carry a
// HARD reason (genuinely no caller = dead/inlined, or a deferred divergent-view /
// class-TBD fold); the per-use views live in <Gruntz/BoundaryUpperViews.h>.
#include <rva.h>
#include <Mfc.h> // superset of Win32.h: WINAPI + MFC CObArray (the +0x1dc array RemoveAll)
#include <Gruntz/BoundaryUpperViews.h> // owner/referent views for this TU (pulls Blk6c.h)

// The engine __cdecl deallocator (operator delete; reloc-masked rel32). 0x1b9b82.
extern "C" void RezFree(void* p);

// The wap-object teardown grand-base vtable (0x5e8cb4); stamped by address.

// ---------------------------------------------------------------------------
// Embedded base-subobject vptr restamp (member dtor of the grand-base): the
// 7-byte `mov [this],&g_wapObjectDtorVtbl; ret` leaf. Three distinct leaf
// classes share the identical shape (each called from its own scalar-deleting
// dtor - e.g. 0x161460 <- ??_G @0x161440, confirmed by xref).
// TERMINAL manual stamps (not convertible to `: public CObject`): retail
// keeps these as THREE separate un-COMDAT-folded copies at 0x161460/0x161560/
// 0x163a10; letting cl emit ~CObject would fold them to ONE COMDAT and
// drop two of the three RVA pins. The reloc-masked stamp is already byte-exact.
// ---------------------------------------------------------------------------

// (0x1413c0 re-homed to src/Image/Image.cpp as CDDSurface::Scale (m_pitch * n) -
// this IS CDDSurface (from CDDSurface::DecodeRun8 @0x140aa0); decl in DDSurface.h.)

// ---------------------------------------------------------------------------
// 0x1614b0 - `if(m_14) RezFree(m_14); m_14 = 0;` buffer release.
// ---------------------------------------------------------------------------
// @orphan: ATTRIBUTED (data-ref ~??_7CImageSet3@@6B@+0x18 = vtable slot 6), but there
// are TWO divergent CImageSet3 classes (the non-polymorphic pixel-probe in
// src/Image/ImageSet3.cpp vs the polymorphic `: public CObject` in
// src/DDrawMgr/DDrawChildGroup.cpp). The slot-6 owner is the polymorphic one; folding
// needs the two same-named classes reconciled/split first (a conflation), so the
// buffer-release is left here pending that split. Deferred, not un-attributable.
RVA(0x001614b0, 0x1c)
void B_1614b0::Release() {
    if (m_14) {
        RezFree(m_14);
    }
    m_14 = 0;
}

// (0x137300 SoundDevice::GetPrimary re-homed to src/Dsndmgr/DirectSoundMgr.cpp, next
// to its CreatePrimaryBuffer @0x137260 / StartPrimary siblings.)

// (0x1433d0 + the 0x1434c0/0x143470/0x143510/0x143590 mode searches + 0x141c80
// g_modeArray teardown re-homed to src/DDrawMgr/DirectDrawMgr.cpp as CDirectDrawMgr::
// Compare / FindIndex / FindLast / FindFwd / FindBack + ClearModeArray_141c80 - ModeArr
// IS CDirectDrawMgr (m_4b8/m_4bc = m_poolItems.m_pData/m_nSize @+0x4b8/+0x4bc);
// SetupCaps drives Compare, CGruntzMgr::CheckDisplayBoundsA/B drive FindFwd/FindBack.)

// (0x1847a0 re-homed to src/Gruntz/MenuItem2.cpp as CMenuItem2::SetFrame - vtable
// slot 14 (~??_7CMenuItem2@@6B@+0x38); decl already in MenuItem2.h, m_70 its field.)

// (0x17fc40 re-homed to src/Stub/BoundaryUpperEh.cpp as C17f9f0::Free - the
// ~C17f9f0 fader dtor (FaderBase-derived) called it via `((B_17fc40*)this)->Free()`,
// proving the view IS C17f9f0; the cast is dissolved onto a real +0x50-buffer method.)

// ---------------------------------------------------------------------------
// 0x184fb0 - __cdecl forward `G(0, a, b);` to 0x184fd0.
// ---------------------------------------------------------------------------
void Sub_184fd0(i32, i32, i32); // 0x184fd0, no body
// @orphan: genuinely NO caller anywhere (xref --tree: its only callers are Gap_184e60
// / Gap_184f30, which are themselves uncalled) - a dead / fully-inlined __cdecl 3-arg
// forward thunk. No class receiver. Legitimate dead-code leave (traced hard).
RVA(0x00184fb0, 0x15)
void Fwd_184fb0(i32 a, i32 b) {
    Sub_184fd0(0, a, b);
}

// (0x134360 CDeviceConfigB::Free360 + 0x1346d0 CDeviceConfigC::Free6d0 re-homed to
// src/DinMgr2/DirectInputMgr2.cpp onto the real device-config classes - m_2a0/m_2a4 are
// the inherited CInputDevRoot::m_stateBuffer/m_stateBufferSize, ReleaseBase is
// CInputDevBase::ReleaseDevices. The DevCfg view is dissolved.)

// (0x145e00 re-homed to src/Image/WarpTextureBlit.cpp as WarpIsPow2 - the is-pow2
// gate its sole caller WarpTextureBlit @0x146a20 already declared reloc-masked there.)

// (0x1413b0 re-homed to src/Image/Image.cpp as CDDSurface::UnlockThunk (m_8->Unlock(0),
// IDirectDrawSurface slot 0x80) - this IS CDDSurface (from DecodeRun8); decl in
// DDSurface.h. The Obj1413 manual-vtable view IS the real m_8 IDirectDrawSurface.)

// (0x1434c0 FindIndex + 0x143470 FindLast re-homed to CDirectDrawMgr - see the
// mode-search note above.)

// ---------------------------------------------------------------------------
// 0x13dee0 - `m_1c = v; if(v > 0) m_28 = 1000 / v;` (CFileImage frame timing).
// __thiscall, 1 arg.
// ---------------------------------------------------------------------------
// @orphan: genuinely NO caller anywhere (xref --tree: its only caller Gap_13df00 is
// itself uncalled) - a dead / fully-inlined frame-timing setter (m_1c=fps, m_28=ms
// per frame). Not a CDDSurface method (0x13dee0 absent from DDSurface.h). Traced hard.
RVA(0x0013dee0, 0x1b)
void B_13dee0::Set(i32 v) {
    m_1c = v;
    if (v > 0) {
        m_28 = 1000 / v;
    }
}

// 0x13df00 - TrySet: if the timing is already configured (m_1c > 0) clear it and
// fail (return 0); otherwise configure to v and succeed (return 1). Its only caller
// is itself uncalled (orphan) - a fully-inlined frame-timing guard. __thiscall, 1 arg.
RVA(0x0013df00, 0x25)
i32 B_13dee0::TrySet(i32 v) {
    if (m_1c > 0) {
        Set(0);
        return 0;
    }
    Set(v);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x13ee30 - COM wait-flip loop: `while(m_8->Flip(2) == DDERR_WASSTILLDRAWING);`.
// IDirectDrawSurface-style manual vtable, slot 0x48. __thiscall.
// ---------------------------------------------------------------------------
// @orphan: genuinely NO caller AND no vtable data-ref (xref --tree + rva: none) - a
// dead / fully-inlined surface flip-wait (m_8 is an IDirectDrawSurface-style surface,
// Flip(2) retry on WASSTILLDRAWING). Not a declared CDDSurface method (Flip is 0x13e850).
// Traced hard; owning surface-wrapper class carries no recoverable edge.
RVA(0x0013ee30, 0x29)
void B_13ee30::WaitFlip() {
    while (m_8->Flip(2) == 0x8876021c) {
    }
}

// (0x151e70 re-homed to src/DDrawMgr/DDrawWorkerCache.cpp as AnimWorkerObj::Clear -
// vtable slot 7 (~??_7AnimWorkerObj@@6B@+0x1c); decl already in that TU, m_14/m_18
// retyped to the owned buffer / killable sub-object.)

// (0x166810 re-homed to src/Wwd/WwdGameObject.cpp as CWwdGameObjectB::Clear_166810
// - this=CWwdGameObjectB (dtor stamps 0x5f00e8 then calls it on this); the Node/
// payload walk-view IS the +0x1dc CObList's raw CNode / CDDrawGroupChild pair.)

// (0x13c8a0 CRezItm::Scan + 0x13c8f0 CRezItm::Check re-homed to src/Rez/RezMgr.cpp as
// CRezItm methods - vtable slots 6/7, RTTI-confirmed. RezDir view + RezDirLookup extern
// dissolved onto the canonical CRezItm.)

// (0x143510 FindFwd + 0x143590 FindBack re-homed to CDirectDrawMgr - see the
// mode-search note above. Both stay @early-stop (regalloc iterator/entry swap).)

// (0x138f20 re-homed to src/Dsndmgr/GruntzSoundZ.cpp as CGruntzSoundInnerZ::Retrigger
// - vtable slot 13 (~??_7CGruntzSoundInnerZ@@6B@+0x34); decl already in GruntzSoundZ.h.
// v8=IsStarted, Helper=IsBusy, v9=Play, m_44/m_48/m_4c=m_pauseDepth/m_playMode/m_playDriver.)

// ---------------------------------------------------------------------------
// 0x16be60 - append helper: if Ready(), Emit(&g_emptyString, arg) then Flush();
// return this. __thiscall, 1 arg. Called by ButeMgr::ParseAttributeFile.
// ---------------------------------------------------------------------------
extern "C" char g_emptyString[]; // _g_emptyString @0x6293f4
// @orphan: a ButeMgr-region writer whose body directly calls three sub-methods
// (Ready 0x16bd10 / Emit 0x16c2d0 / Flush 0x16bd90) - so it is NOT CString::operator+=
// (that ButeMgr.cpp comment is a mislabel; a += is a single ConcatInPlace, not three
// calls). Its owning writer class (the receiver of Ready/Emit/Flush) is not yet
// modeled; class-recovery TODO (the .att token emitter), not un-attributable.
RVA(0x0016be60, 0x2a)
C16be60* C16be60::Append(i32 arg) {
    if (Ready()) {
        Emit(g_emptyString, arg);
        Flush();
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x151d20 - notify a hooked callback: stash/replace m_7c->m_1c with arg, invoke
// the +0x10 callback(this), restore m_1c if unchanged. __thiscall, 1 arg.
// ---------------------------------------------------------------------------
// @orphan: genuinely NO caller AND no data-ref (xref --tree + rva: none) - a dead /
// fully-inlined callback-notify (stash/replace m_7c->m_1c, invoke the +0x10 hook,
// restore). Traced hard; the +0x7c hook-owner class carries no recoverable edge.
RVA(0x00151d20, 0x3a)
i32 B_151d20::Notify(void* arg) {
    Cb151d20* p = m_7c;
    if (!p) {
        return 0;
    }
    void* saved = p->m_1c;
    p->m_1c = arg;
    m_7c->fn(this);
    if (m_7c->m_1c == arg) {
        m_7c->m_1c = saved;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Global-object tail-forwards: load the global's address into ecx and tail-jump
// the shared array teardown (0x1b4f0b). The DATA globals are named; the shared
// callee is reloc-masked.
// ---------------------------------------------------------------------------
// (0x13e070 ClearImageCache re-homed to src/Image/Image.cpp - g_imageCache
// (CImageCache @0x653c88) is that TU's own datum.)

// (0x141c80 ClearModeArray re-homed to src/DDrawMgr/DirectDrawMgr.cpp - see the
// mode-search note above; g_modeArray is that TU's own datum.)

// (0x13e0a0 + 0x148b50 + 0x148cc0 re-homed: the "ambiguous CDDSurface::Init1" is
// RESOLVED - the disasm proves arg1=collection-context h (->slot-8 v20), arg2=a
// (a 0x6c-byte Blk6c* handle, rep-movs'd into the +0x10 DDSURFACEDESC scratch). So
// 0x13e0a0 IS CDDSurface::Init1 (slot 2, -> src/Image/Image.cpp), and 0x148b50 /
// 0x148cc0 are its CPoolItemAB8::Init1 / CPoolItemAE8::Init1 overrides (call the base
// then InstallColorFormat / boolify) -> src/DDrawMgr/DDrawPtrCollections.cpp. The
// ImgOwned view IS CDDSurface; dissolved.)

// (0x13dec0 RezMgr::SpinWaitUntil re-homed to src/Rez/RezMgr.cpp, right after its sole
// caller RezMgr::UpdateClock @0x13ddc0 - the ms frame-pacing busy-wait.)
