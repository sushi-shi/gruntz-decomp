// GruntPuddle.cpp - CGruntPuddle::SetBute only. The CGruntPuddle class body
// (dtor/ctor/FireActivation/Place/Remove/Serialize, 0x010d10 + 0x40490-0x40fc0)
// lives in the ORIGINAL wormhole TU (Wormhole.cpp: CWormhole + CGruntPuddle +
// CTeleporter, one obj - see its header for the one-TU evidence).
//
// @identity-TODO: SetBute's birth position is the lone 0x7d810 interval, right
// before the gruntselectedsprite init-frag run (i353 @0x7d8a0) at the tail of the
// TriggerMgr region - NOT the wormhole TU. Its owning original TU is unrecovered
// (candidates: the GruntSelectedSprite TU's head, or a COMDAT-at-usage emission);
// it stays here as a single-fn unit pending the TriggerMgr-tail partition.
#include <Gruntz/GruntPuddle.h>
#include <rva.h>

RVA(0x0007d810, 0x25)
void CGruntPuddle::SetBute(char* key) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(key);
}
