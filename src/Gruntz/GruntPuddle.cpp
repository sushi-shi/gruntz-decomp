// @identity-TODO
// This isolated contribution has no file anchor or data reference proving its owner TU.
#include <Gruntz/GruntPuddle.h>
#include <rva.h>

RVA(0x0007d810, 0x25)
void CGruntPuddle::SetBute(char* key) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(key);
}
