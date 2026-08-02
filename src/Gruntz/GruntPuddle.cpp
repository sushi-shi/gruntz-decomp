// @identity-TODO
// This isolated contribution has no file anchor or data reference proving its owner TU.

#include <rva.h>

#include <Gruntz/GruntPuddle.h>

RVA(0x0007d810, 0x25)
void CGruntPuddle::SetBute(char* key) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(key);
}
