#include <rva.h>
// CGruntzMgr.cpp - the remaining engine-label stubs for CGruntzMgr. The class and
// most of its methods are reconstructed in src/Gruntz/GruntzMgr.cpp (CGruntzMgr :
// public WAP32::CGameMgr, 0xa30); the ctor, ~CGruntzMgr, the ??_G scalar-deleting
// destructor, UnknownClose, AccrueScoreTime (0x861e0), OnCheckpointReached (0x8e6c0),
// DelayedQuit (0x8f530), RunModalDialog (0x90260) and LoadSaveMessageSprite (0x92420)
// now live there.
//
// 0x092f00 is the save-as name dialog (495 B, /GX, TWO destructible stack locals - a
// modal dialog AND a result CString built with a "custom_" prefix). The compound /GX
// frame (per-local EH-state numbering + the inline CString construction) is deferred to
// the final sweep; still stubbed here.
//
// 0x08fea0 is NOT a CGruntzMgr method at all: it constructs a fresh object (CFileIO
// at +0x124, CByteArray at +0x138, srand(time(0))), which contradicts the CGruntzMgr
// ctor's int store at +0x138 and overwrites +0x00 (a vptr) - a mis-attributed label
// kept here pending its true owner class.

class CGruntzMgr {
public:
    void InitCFileIOMember();
    i32 winapi_092f00_PostMessageA();
};

// @confidence: med
// @source: call-xref
// @stub
// NOTE: this 0x8fea0 method writes a *fresh* object (stores 0 to this+0/4/8 and
// to +0x134, constructs a CFileIO at +0x124 and CByteArray at +0x138, then
// srand(time(0)), returning `this`) - i.e. it is a constructor whose CByteArray
// member at +0x138 contradicts the CGruntzMgr ctor's int store at +0x138, so it is
// NOT a method on the 0xa30 CGruntzMgr layout. Kept stubbed pending the true
// (mis-attributed) owner class.
RVA(0x0008fea0, 0x6d)
void CGruntzMgr::InitCFileIOMember() {}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x00092f00, 0x1ef)
i32 CGruntzMgr::winapi_092f00_PostMessageA() {
    return 0;
}
