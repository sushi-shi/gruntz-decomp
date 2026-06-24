// TileGridCommand.cpp - Gruntz CTileGridCommand (C:\Proj\Gruntz).
//
// A tile-grid command object (type tag @ +0x04, coords @ +0x08/+0x0c, container
// back-pointer @ +0x20, game-clock snapshot @ +0x24).  RecordMove captures the
// game clock and hands the command back to its container; Serialize streams the
// command's fields through a CSerialStream.
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (verified by the +0x20
// container back-pointer and the +0x04 type tag, not the switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/TileGridCommand.h>

// ---------------------------------------------------------------------------
// CTileGridCommand::RecordMove
// Captures the running game clock into +0x24, then hands this command to its
// owning container (m_20->MoveList1ToList2(this)).
// ---------------------------------------------------------------------------
RVA(0x00112880, 0x12)
void CTileGridCommand::RecordMove() {
    m_24 = g_645588;
    m_20->MoveList1ToList2(this);
}

// ---------------------------------------------------------------------------
// CTileGridCommand::Serialize
// Returns 0 if the stream is null or the active game-manager (g_gameReg+0x30) is
// null; otherwise transfers the command's scalar fields then a 24-dword grid
// block through the stream's Transfer (vtable slot 12) and returns 1.
// ---------------------------------------------------------------------------
RVA(0x00113ae0, 0xe8)
int CTileGridCommand::Serialize(TgcStream* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    s->Transfer(&m_08, 4);
    s->Transfer(&m_0c, 4);
    s->Transfer(&m_10, 4);
    s->Transfer(&m_14, 4);
    s->Transfer(&m_18, 4);
    s->Transfer(&m_1c, 4);
    s->Transfer(&m_28, 4);
    s->Transfer(&m_2c, 4);
    s->Transfer(&m_30, 4);
    s->Transfer(&m_34, 4);
    s->Transfer(&m_38, 4);
    s->Transfer(&m_24, 4);
    int* p = m_grid;
    for (int i = 0; i < 24; i++) {
        s->Transfer(p, 4);
        p++;
    }
    return 1;
}
