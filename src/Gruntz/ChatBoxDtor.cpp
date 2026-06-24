// ChatBoxDtor.cpp - the CChatBox destructor, split into its own /GX (eh) TU.
//
// 0xa0360 is the ONLY CChatBox method that needs the /GX exception-handling frame
// (its CString members m_44/m_48 + the CPtrList m_24 are unwound on teardown). The
// rest of the class is frameless and lives in ChatBox.cpp under base flags, so the
// destructor is isolated here to give it /GX without forcing a (wrong) frame onto
// the frameless siblings (e.g. Find).
#include <rva.h>

#include <Gruntz/ChatBox.h>

// 0xa0360 - destructor: Reset(), then the member dtors (m_48/m_44 CStrings, m_24
// list) under the /GX EH frame.
RVA(0x000a0360, 0x64)
CChatBox::~CChatBox() {
    Reset();
}
