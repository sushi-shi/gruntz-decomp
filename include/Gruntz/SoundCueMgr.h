// SoundCueMgr.h - the Dsndmgr UI-sound-cue / DirectSound-buffer manager
// (C:\Proj\Dsndmgr). ONE class, ONE definition.
//
// The rtti/heuristic labeling gave this the mangled name "CStatusBarMgr", but it is
// a GENUINELY DIFFERENT class from the Gruntz in-game status-bar builder
// (CStatusBarMgr, src/Gruntz/StatusBarMgr.cpp) - proven by two incompatible layout
// facts, so it is renamed here (same-name-genuinely-different -> RENAME):
//   * +0x10 is a POINTER to the live-surface owner (deref m_10->m_78) here, but an
//     int base-x coordinate (bx = m_10; r.left = bx + 0x18 ...) in the HUD builder;
//   * the item-pool list head is an embedded SBList at +0x58, which OVERLAPS the HUD
//     builder's per-tab CPtrList at +0x48 (a CPtrList spans 0x48..0x64).
// The two live in different modules (Dsndmgr 0x135xxx-0x136xxx vs Gruntz 0x102xxx).
//
// It plays UI sound cues by pulling a pooled DirectSound buffer (CStatusBarItem2)
// out of its +0x58 list (GetItem / Create @0x135d70/0x135c20) and pushing cue
// parameters into it (ConfigureItem @0x1360d0). Only offsets + code bytes are
// load-bearing; field names are placeholders for the recovered engine identities.
#ifndef GRUNTZ_SOUNDCUEMGR_H
#define GRUNTZ_SOUNDCUEMGR_H

#include <Ints.h>
#include <rva.h>

// The live-surface owner at CSoundCueMgr+0x10 (m_78 = the "surface is free" gate).
struct CStatusBarSurface {
    char m_pad00[0x78];
    i32 m_78; // +0x78  live-surface gate
};
SIZE_UNKNOWN(CStatusBarSurface);

// Intrusive list link embedded in each item at +0x44.
struct SBLink {
    void* m_0;
    void* m_4;
};
SIZE_UNKNOWN(SBLink);

// The pooled DirectSound buffer wrapper GetItem manages / ConfigureItem drives (the
// real DSNDMGR buffer, src/Dsndmgr/DirectSoundMgr.cpp); every method is external
// (no body) so its __thiscall `call rel32` reloc-masks.
class CStatusBarItem2 {
public:
    i32 Sub3f0();       // 0x1353f0  IsPlaying
    i32 Inner560(i32);  // 0x135560  SetVolume
    i32 Inner740(i32);  // 0x135740  SetPan
    i32 Inner880(i32);  // 0x135880  SetFrequency
    i32 SetField0(i32); // 0x1355c0  (ret 4)
    i32 SetField1(i32); // 0x1357a0  (ret 4)
    i32 SetField2(i32); // 0x135920  (ret 4)
    i32 SetField3(i32); // 0x135510  (ret 4, result ignored)
    i32 Finalize();     // 0x136270  (ret 0)

    char m_pad0[0x44];         // +0x00..0x43
    SBLink m_link44;           // +0x44  intrusive list link (Unlink/Append)
    char m_pad4c[0x50 - 0x4c]; // +0x4c..0x4f
    i32 m_50;                  // +0x50  play key
};
SIZE_UNKNOWN(CStatusBarItem2);

// Traversal node: next @+0, item @+8.
struct SBNode {
    SBNode* m_0; // next
    char m_pad4[4];
    CStatusBarItem2* m_8; // item
};
SIZE_UNKNOWN(SBNode);

// Embedded 2-pointer list head (head @+0, tail @+4).
struct SBList {
    SBNode* m_head;
    SBNode* m_tail;
    void Unlink(SBLink*); // 0x1391e0 thiscall
    void Append(SBLink*); // 0x139110 thiscall
};
SIZE_UNKNOWN(SBList);

class CSoundCueMgr {
public:
    i32 ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1360d0
    CStatusBarItem2* GetItem();                        // 0x135d70

    char m_pad00[0x10];
    CStatusBarSurface* m_10; // +0x10  live-surface owner
    char m_pad14[0x18 - 0x14];
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
    i32 m_20; // +0x20
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28  cue duration
    char m_pad2c[0x58 - 0x2c];
    SBList m_58; // +0x58  embedded item-pool list head

private:
    CStatusBarItem2* Create(i32); // 0x135c20 thiscall
};
SIZE_UNKNOWN(CSoundCueMgr);

#endif // GRUNTZ_SOUNDCUEMGR_H
