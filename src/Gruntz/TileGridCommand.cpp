// TileGridCommand.cpp - Gruntz CTileGridCommand (C:\Proj\Gruntz).
//
// A tile-grid command object (type tag @ +0x04, coords @ +0x08/+0x0c, container
// back-pointer @ +0x20, game-clock snapshot @ +0x24).  RecordMove captures the
// game clock and hands the command back to its container; Serialize streams the
// command's fields through a CSerialStream; Classify drives the on/off duty cycle
// off the game clock; BumpCell / ApplyMove edit the active tile layer's cell.
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
i32 CTileGridCommand::Serialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_08, 4);
    s->Write(&m_0c, 4);
    s->Write(&m_10, 4);
    s->Write(&m_14, 4);
    s->Write(&m_18, 4);
    s->Write(&m_1c, 4);
    s->Write(&m_28, 4);
    s->Write(&m_2c, 4);
    s->Write(&m_30, 4);
    s->Write(&m_34, 4);
    s->Write(&m_dutyOn, 4);
    s->Write(&m_24, 4);
    i32* p = m_grid;
    for (i32 i = 0; i < 24; i++) {
        s->Write(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::Deserialize
// The read counterpart of Serialize: same null/registry guard, same field list,
// but each transfer goes through the stream's read slot (+0x2c) instead of the
// write slot (+0x30). Reads the 12 scalar fields then the 24-dword grid block.
// ---------------------------------------------------------------------------
RVA(0x00113c10, 0xe8)
i32 CTileGridCommand::Deserialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_08, 4);
    s->Read(&m_0c, 4);
    s->Read(&m_10, 4);
    s->Read(&m_14, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_28, 4);
    s->Read(&m_2c, 4);
    s->Read(&m_30, 4);
    s->Read(&m_34, 4);
    s->Read(&m_dutyOn, 4);
    s->Read(&m_24, 4);
    i32* p = m_grid;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::Classify
// Drives the command's on/off duty cycle off the running game clock: while the
// elapsed time is within the lead-in (m_2c) it stays active (+1); past it, the
// remainder modulo the on+off period (m_28+m_30) selects the on or off phase,
// firing the slot-0 tick and latching m_dutyOn on each edge.  Returns +1 (active),
// 0 (just turned on, one-shot of type 0x18) or -1 (just turned off, not 0x17).
// ---------------------------------------------------------------------------
// @early-stop
// entropy-tail (~96%): logic + the single-ret1 convergence match; only the last
// type==0x17 case's ret1 is tail-duplicated instead of merged into the shared tail.
RVA(0x00112970, 0xad)
i32 CTileGridCommand::Classify(i32 arg) {
    u32 elapsed = g_645588 - m_24;
    if (elapsed <= m_2c) {
        goto ret1;
    }
    elapsed -= m_2c;
    {
        u32 period = m_30 + m_28;
        if (elapsed > period) {
            if (m_typeTag == 0x18) {
                Tick();
                return 0;
            }
            if (m_typeTag != 0x17) {
                if (m_dutyOn == 1) {
                    Tick();
                }
                return -1;
            }
        }
        u32 rem = elapsed % period;
        if (rem < m_28) {
            if (m_dutyOn != 0) {
                goto ret1;
            }
            Tick();
            m_dutyOn = 1;
            if (m_typeTag == 0x18) {
                return 0;
            }
            goto ret1;
        }
        if (m_dutyOn != 1) {
            goto ret1;
        }
        Tick();
        m_dutyOn = 0;
        if (m_typeTag == 0x17) {
            goto ret1;
        }
        return -1;
    }
ret1:
    return 1;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::BumpCell
// Reads the active tile layer's cell at (m_08,m_0c), stores value+1 back, marks
// the cell dirty for redraw, and latches m_14.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// addressing-mode wall (~73%): logic identical; retail indexes the row table with
// a scale-4 address mode (`[rowtbl+y*4]`), the recompile pre-shifts y (shl 2) and
// uses scale-1, propagating through both cell accesses.
RVA(0x00112b70, 0x5a)
i32 CTileGridCommand::BumpCell() {
    CGameRegistry* reg = g_gameReg;
    CViewport* layer = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
    i32 v = layer->m_cells[m_08 + layer->m_rowBase[m_0c]] + 1;
    CViewport* layer2 = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
    layer2->m_cells[m_08 + layer2->m_rowBase[m_0c]] = v;
    ((TgcRedraw*)reg->m_tileGrid)->MarkCell(m_08, m_0c, v);
    m_14 = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::ApplyMove
// Edits the (m_08,m_0c) tile cell of the active layer: an explicit override m_34
// when set, else by the verb (0x1e->0x5a, 0x1f->0x5b, 0x21->cell+1), marks the
// cell dirty, flags the surrounding screen rect, and (when m_2c is set) posts an
// in-game-text record stamped with m_2c.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/addressing wall (~70%): logic + the subtract-chain switch + the
// shared px/py reuse match; the four duplicated grid-access blocks allocate
// registers differently from retail (same scale-4 vs pre-shift split as BumpCell).
RVA(0x00112590, 0x166)
i32 CTileGridCommand::ApplyMove(i32 verb) {
    i32 v;
    if (m_34 != 0) {
        CGameRegistry* reg = g_gameReg;
        CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
        L->m_cells[L->m_rowBase[m_0c] + m_08] = m_34;
        v = m_34;
        ((TgcRedraw*)reg->m_tileGrid)->MarkCell(m_08, m_0c, v);
    } else {
        switch (verb) {
            case 0x22: {
                CGameRegistry* reg = g_gameReg;
                CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                v = L->m_cells[L->m_rowBase[m_0c] + m_08] + 1;
                CViewport* L2 = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                L2->m_cells[L2->m_rowBase[m_0c] + m_08] = v;
                ((TgcRedraw*)reg->m_tileGrid)->MarkCell(m_08, m_0c, v);
                break;
            }
            case 0x1f: {
                CGameRegistry* reg = g_gameReg;
                CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                L->m_cells[L->m_rowBase[m_0c] + m_08] = 0x5b;
                ((TgcRedraw*)reg->m_tileGrid)->MarkCell(m_08, m_0c, 0x5b);
                break;
            }
            case 0x1e: {
                CGameRegistry* reg = g_gameReg;
                CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                L->m_cells[L->m_rowBase[m_0c] + m_08] = 0x5a;
                ((TgcRedraw*)reg->m_tileGrid)->MarkCell(m_08, m_0c, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGameRegistry* reg = g_gameReg;
    i32 py = (m_0c << 5) + 0x10;
    i32 px = (m_08 << 5) + 0x10;
    ((TgcRegion*)reg->m_cmdGrid)->MarkRect(m_28, px, py, m_30, 1, 0);
    if (m_2c != 0) {
        CGameObject* rec =
            reg->m_world->m_8->CreateSprite(0, px, py, 95000, "InGameText", 0x40003);
        if (rec != 0) {
            rec->m_124 = m_2c;
        }
    }
    return 1;
}
