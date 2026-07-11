// FreeNodePool.cpp - the typed intrusive free-list pool's Push (0x0311b0), the grunt
// coord-node recycler's canonical method. Re-homed from src/Stub/DiscoveredSmall.cpp:
// Push is a self-contained pointer-arithmetic leaf (RTTI-named ?Push@FreeNodePool)
// with no external calls. See <Gruntz/FreeNodePool.h> for the shared class shape.
//
// NOTE (waveP): the former 0x082da0 block of this unit (the four g_str6455xx CString
// _$E init thunks + ResetCoordPool/ClearCoordPool) was FOLDED into src/Gruntz/GameText.cpp
// - it is the RVA-contiguous tail of GameText's obj (0x82d20 + 0x80 = 0x82da0) whose
// g_str6455xx globals share one contiguous .data run (0x645514..0x645530) with GameText's.
// Push (0x0311b0) STAYS here: it is a COMDAT-pooled leaf sitting in the battlezmapconfig
// interval among OTHER units' pooled leaves (zvec/wwdfile/ddrawsubmgr @0x310f0..0x312a0),
// called from many classes (CGrunt/CTriggerMgr/CRollingBall/CUserLogic per xref) with no
// single owning obj - proximity attribution is a dead end (boundary-proximity doctrine).
#include <Gruntz/FreeNodePool.h>
#include <rva.h>

// FreeNodePool::Push - subtract the node's link offset (m_c) from the freed element
// pointer and chain the raw node onto the free-list head (m_4). __thiscall, 1 stack arg.
RVA(0x000311b0, 0x14)
void FreeNodePool::Push(void* p) {
    char* node = (char*)p - m_c;
    *(void**)node = m_4;
    m_4 = node;
}
