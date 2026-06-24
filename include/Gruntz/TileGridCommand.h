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

#include <Gruntz/TileTriggerContainer.h>

// The running game clock (DAT_00645588); reloc-masked DIR32 datum.
extern u32 g_645588;

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c); only +0x30 (the active
// game-manager pointer) is read here.  Reloc-masked DIR32.
struct TgcGameReg {
    char _pad00[0x30];
    void* m_30; // +0x30  game-manager pointer (null-checked)
};
extern TgcGameReg* g_gameReg;

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
    virtual void Slot2C();
    virtual void Transfer(void* buf, i32 n); // +0x30
};

class CTileGridCommand {
public:
    void RecordMove();           // 0x112880
    i32 Serialize(TgcStream* s); // 0x113ae0

    void* m_vptr;                // +0x00
    i32 m_04;                    // +0x04  type tag
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
    i32 m_38;                    // +0x38
    i32 m_grid[24];              // +0x3c..+0x9b  (24-dword block, serialized in a loop)
};

#endif // SRC_GRUNTZ_TILEGRIDCOMMAND_H
