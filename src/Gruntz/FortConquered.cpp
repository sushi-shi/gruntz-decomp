// FortConquered.cpp - CFortressFlag::HandleFortConquered @0x03f5f0 (1318 B), the
// per-frame fort-conquest check.
//
// original TU: filename unknown (@identity-TODO). Split out of FortressFlag.cpp
// (wave3-I): the retail body's BIRTH POSITION is the lone 0x3f5f0-0x3fb16 text
// interval between the WormholeActs block (0x3f210-0x3f57d) and the wormhole trio
// (0x3fc70+), and its three private .data cells (0x20d154/0x20d168/0x20d16c) sit
// BEFORE the wormhole trio's band (0x20d194) in the 98%-monotone .data
// contribution order - so it CANNOT belong to the fortressflag obj at 0x45d30
// (whose band is 0x20d384+). The owning class may still be CFortressFlag (a
// class's methods can span TUs); candidates for the file: the WormholeActs TU's
// tail (fort conquered -> the victory wormhole spawns) or its own file. Callees
// (FindGruntAt / ResolveDeathAnimation / LoadFinishLevelSprite / CreateSprite)
// are level-end flow, consistent with either.
#include <Gruntz/FortressFlag.h>
#include <rva.h>

// @early-stop
// >512B /GX regalloc wall (carried from FortressFlag.cpp): the full-body
// reconstruction BUILDS but scores 0.0% (below the empty-stub baseline) - retail
// pins `this` to ebp with a 0x24 frame + canonical /GX prologue while the /O2
// recompile pins `this` to ebx with a 0x20 frame + a hoisted `mov eax,fs:0`, a
// this-register + frame-slot divergence cascading through all 1318 B. Kept as the
// empty (highest-%) stub per the >512B REVERT rule; final-sweep leaf-first redo
// needs the callee set + a matching regalloc modeled first. Structure decoded:
// the +0x1a0 sub-clock tick, the g_gameReg->m_134 mode gate, HitTestCell +
// dedup vs owner->m_124, the 5-CString "<A> was conquered by <B>!" HUD message,
// the config re-tag, the two handler-type re-home list walks + a g_freeList pop,
// and a per-object GAME_EXPLOSION3 eye-candy spawn.
RVA(0x0003f5f0, 0x526)
void CFortressFlag::HandleFortConquered() {}
