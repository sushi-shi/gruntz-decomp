// InputState.h - the game-registry +0x54 object: ONE real class (proven), reached
// through two facets that used to live as separate per-TU views.
//
// SINGLE-OBJECT IDENTITY (proven, not a per-mode union): CGruntzMgr::LoadWorldMode
// (0x091a40) NEWs this object as RezAlloc(0x30) and, in one ctor, sets +0x00 = 0,
// +0x04 = 0x64, constructs the embedded CObList at +0x08 (CObList(0xa)) and later
// writes +0x24 = 1 (Arm) when ambient sound is enabled. The random-ambient-sound TU
// then reads the SAME physical object: its +0x24 as the "playable"/object-count gate
// and its +0x08 CObList as the spatial-sound voice list it unlinks live voices from
// (RemoveAt @0x1b4ac7). So the manager's "input/state" facet (Flush/Arm/Disarm/
// InitInput/Store) and the ambient TU's "active level" facet (m_armed gate + spatial
// voice list) are ONE object - not a genuine per-mode-reused slot. It is torn down by
// CGruntzMgr::Close (m_inputState->Teardown()) and re-created on each video-mode reload.
//
// Field names are placeholders; only offsets + code bytes are load-bearing. All engine
// methods are reloc-masked (no body).
#ifndef GRUNTZ_GRUNTZ_INPUTSTATE_H
#define GRUNTZ_GRUNTZ_INPUTSTATE_H

#include <Ints.h>
#include <rva.h> // SIZE/SIZE_UNKNOWN

// The embedded CObList at +0x08 of the +0x54 object: the spatial-sound voice node
// list. The manager ctor's Init(0xa) is the CObList(nBlockSize=10) ctor and Dtor its
// destructor; the random-ambient-sound placement path unlinks a live voice node with
// RemoveAt. Accessed via `(CObListSub*)((char*)m_inputState + 8)` (the deliberate
// embedded-sub-object offset the manager's /GX bodies already bake in).
SIZE_UNKNOWN(CObListSub);
struct CObListSub {
    void Init(i32 cap);       // CObList ctor (this = obj+8, 0xa) reloc-masked
    void Dtor();              // ~CObList-family (this = obj+8) reloc-masked
    i32 RemoveAt(void* node); // 0x1b4ac7 unlink a spatial voice node (ambient TU)
};

// The +0x54 object. m_armed (+0x24) is the manager's ambient-arm flag AND the
// ambient TU's "playable"/object-count gate (the manager writes 1 here via Arm when
// m_isAmbientEnabled, and the ambient TU gates its play on it being non-zero). The
// two parameterless thiscalls toggle its active state; Flush() is its +0 teardown;
// InitInput wires it to the world's +0x28 sub-controller. All reloc-masked. The
// real object is 0x30 bytes (RezAlloc(0x30)); only +0x08/+0x24 are modeled.
SIZE_UNKNOWN(CInput54);
struct CInput54 {
    char m_pad0[0x24];
    i32 m_armed;   // +0x24  ambient-arm flag (== the ambient TU's object-count/playable gate)
    void Flush();  // 0x1082-thunk (this) reloc-masked
    void Arm();    // FUN_0040bcf0 (this) reloc-masked (ghidra "Resume"; ILT thunk 0x18e8)
    void Disarm(); // FUN_0040bc80 (this) reloc-masked (ghidra "Stop"; ILT thunk 0x29b9)
    i32 InitInput(void* worldSub28, i32 inputFlag); // FUN_0040b5e0 (this, sub28, flag)
    void StoreFlag(i32 v);                          // FUN_004385e0-family (this, v)
    void Teardown();                                // (this) reloc-masked (Close)
};

#endif // GRUNTZ_GRUNTZ_INPUTSTATE_H
