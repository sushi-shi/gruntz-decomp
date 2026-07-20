#include <rva.h>

#include <Gruntz/ChatBox.h>

RVA(0x000a0360, 0x64)
CChatBox::~CChatBox() {
    Reset();
}
