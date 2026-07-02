// TileGridCommand.h - Gruntz CTileGridCommand (C:\Proj\Gruntz).
//
// A tile-grid command object: a type tag at +0x04, grid coords at +0x08/+0x0c,
// a flag at +0x14, a back-pointer to its owning 3-list container at +0x20, and a
// captured game-clock snapshot at +0x24.  It serializes its fields through a
// CSerialStream and edits the game registry's tile grid (g_gameReg) on apply.
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (type tag @ +0x04, coords
// @ +0x08/+0x0c, the +0x20 container back-pointer - not the +0x04 child-list
// switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_TILEGRIDCOMMAND_H
#define SRC_GRUNTZ_TILEGRIDCOMMAND_H

#include <Ints.h>
#include <Gruntz/CGameRegistry.h>
#include <rva.h> // SIZE_UNKNOWN class-metadata macros used below

#include <Gruntz/TileTriggerContainer.h>

// The running game clock (DAT_00645588); reloc-masked DIR32 datum.
extern u32 g_645588;

// One indexed tile layer: a flat cell array at +0x20 and a per-row base-offset
// table at +0x24, so cell (x,y) is m_20[m_24[y] + x].
struct TgcLayer {
    char _pad00[0x20];
    i32* m_20; // +0x20  flat cell array
    i32* m_24; // +0x24  per-row base offsets
};
SIZE_UNKNOWN(TgcLayer);

// The tile map: m_5c is the active layer.
struct TgcMap {
    char _pad00[0x5c];
    TgcLayer* m_5c; // +0x5c  active layer
};
SIZE_UNKNOWN(TgcMap);

// A report record posted by the in-game text manager; +0x124 latches a serial.
struct TgcReport {
    char _pad00[0x124];
    i32 m_124; // +0x124
};
SIZE_UNKNOWN(TgcReport);

// The in-game floating-text manager (gamemgr->m_08): Report posts a formatted
// status line and returns the new record.  __thiscall engine callee, reloc-masked.
struct TgcTextMgr {
    TgcReport* Report(i32 a1, i32 x, i32 y, i32 strId, const char* fmt, i32 flags); // 0x1597b0
};
SIZE_UNKNOWN(TgcTextMgr);

// The active game manager: m_08 is the in-game text manager; m_24 the tile map.
struct TgcGameMgr {
    char _pad00[0x08];
    TgcTextMgr* m_08; // +0x08  in-game text manager
    char _pad0c[0x24 - 0x0c];
    TgcMap* m_24; // +0x24  the tile map
};
SIZE_UNKNOWN(TgcGameMgr);

// A redraw-region helper (g_gameReg->m_70): MarkCell pushes a dirty cell so the
// renderer repaints it.  __thiscall engine callee, reloc-masked.
struct TgcRedraw {
    void MarkCell(i32 x, i32 y, i32 val); // 0x33f0
};
SIZE_UNKNOWN(TgcRedraw);

// A pixel-region dirty helper (g_gameReg->m_68): MarkRect flags a screen rect for
// repaint.  __thiscall engine callee, reloc-masked.
struct TgcRegion {
    void MarkRect(i32 a1, i32 x, i32 y, i32 span, i32 a5, i32 a6); // 0x152d
};
SIZE_UNKNOWN(TgcRegion);

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c); +0x30 is the active game
// manager, +0x68 the rect-dirty helper, +0x70 the redraw helper.
SIZE_UNKNOWN(CGameRegistry);
extern CGameRegistry* g_gameReg;

// A serialization stream: Transfer (vtable slot 12, +0x30) copies n bytes
// to/from a buffer.  Only the slot offset matters; reloc-masked virtual call.
class TgcStream {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual i32 Read(void* buf, i32 n);      // +0x2c (deserialize transfer)
    virtual void Transfer(void* buf, i32 n); // +0x30
};
SIZE_UNKNOWN(TgcStream);

class CTileGridCommand {
public:
    // slot 0 (+0x00): the duty-edge tick virtual (real polymorphic; was fired via a
    // TgcTickView cast of a manual-vptr command -> mov eax,[this]; call [eax]).
    virtual void Tick();

    void RecordMove();             // 0x112880
    i32 Serialize(TgcStream* s);   // 0x113ae0
    i32 Deserialize(TgcStream* s); // 0x113c10

    // Time-driven duty-cycle classifier: returns +1 while inside the on/off span,
    // 0 on the rising edge of a one-shot, -1 on the falling edge.  __thiscall.
    i32 Classify(i32 arg); // 0x112970

    // Sets the tile cell (m_08,m_0c) of the active layer to its value+1 and marks
    // it dirty; latches m_14.  __thiscall.
    i32 BumpCell(); // 0x112b70

    // Edits the tile grid according to a verb arg (set/clear/notify), then reports
    // the move into the in-game text log.  __thiscall.
    i32 ApplyMove(i32 verb); // 0x112590

    // +0x00  implicit vptr (real virtual Tick above; was an explicit void* m_vptr)
    i32 m_typeTag;               // +0x04  type tag (0x17/0x18 duty-cycle discriminant)
    i32 m_08;                    // +0x08  coord x
    i32 m_0c;                    // +0x0c  coord y
    i32 m_10;                    // +0x10
    i32 m_14;                    // +0x14  flag
    i32 m_18;                    // +0x18
    i32 m_1c;                    // +0x1c
    CTileTriggerContainer* m_20; // +0x20  owning container
    u32 m_24;                    // +0x24  captured game clock
    i32 m_28;                    // +0x28
    i32 m_2c;                    // +0x2c
    i32 m_30;                    // +0x30
    i32 m_34;                    // +0x34
    i32 m_dutyOn;                // +0x38  duty-cycle on/off latch (1 = currently on)
    i32 m_grid[24];              // +0x3c..+0x9b  (24-dword block, serialized in a loop)
};
SIZE_UNKNOWN(CTileGridCommand);

#endif // SRC_GRUNTZ_TILEGRIDCOMMAND_H
