// ChatBoxDtor.cpp - CChatBox::~CChatBox in its own /GX (eh) TU. It is the only
// CChatBox method needing the exception frame (the CString members m_row0Key/
// m_row1Key + the CPtrList m_nodeList are unwound on teardown); the frameless
// siblings stay in ChatBox.cpp under base flags so this frame is not forced onto them.
#include <rva.h>

#include <Gruntz/ChatBox.h>

// Reset(), then the member dtors (m_48/m_44 CStrings, m_24 list) under the /GX frame.
RVA(0x000a0360, 0x64)
CChatBox::~CChatBox() {
    Reset();
}
