// TileTriggerFactory.cpp - the tile-trigger logic factory @0x117800 (proximity
// CTileTriggerContainer / CTileTriggerSwitchLogic). A __thiscall(reader, 7, a2, a3)
// ret 0x10 that reads a 4-byte type id off the serialized `reader` (vtable slot 11 =
// +0x2c), then dispatches a dense `switch(id)` (ids 1..26, the MSVC compact byte-index
// + jump-table idiom: index table @0x517cbc, jump table @0x517c80) to build the matching
// trigger-logic object. Each case Rez-allocates the object (0x8c / 0x9c / 0xc8 bytes),
// runs its 0-arg ctor thunk, then a 4-arg register thunk (0x277f for ids 1..8, 0x1abe
// for ids 23..26 + 21, 0x1d39 for id 22), stamps the owner + id, and returns it. id 21
// additionally resolves the board tile under the object and, on a 0x67/0x68 tile, latches
// the object into this->m_70.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing. The ctor /
// register thunks are reloc-masked externals (no body); the type id map (id-1):
//   0..7 -> cases 0..7   8..19 -> default(0)   20..25 -> cases 8..13
#include <rva.h>

#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the serialized reader is the shared CSerialArchive (Read @ +0x2c)
#include <new> // Rez heap throwing operator new / nothrow delete (0x1b9b46 / 0x1b9b82, both __cdecl)
i32 __stdcall Gate113860(void* a, i32 b, i32 c, i32 d); // 0x113860 (TtcTrigElem::Reg* view)

// The serialized type id is read off the shared CSerialArchive stream (Read @
// vtable slot 11, +0x2c); `mov eax,[r]; call [eax+0x2c]` falls out with no cast.

// A board tile-object reached via g_mgrSettings->m_world->m_24->m_4c[tile]; slot 8 (+0x20)
// returns the tile's gameplay type id. Reloc-masked virtual.
struct CTileObj {
    virtual void s0();
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual i32 TypeId(); // slot 8 (+0x20)
};
SIZE_UNKNOWN(CTileObj);

// The board geometry (g_mgrSettings->m_world->m_24): m_5c->m_28 / m_5c->m_2c are the x/y
// bounds, m_5c->m_24 the row base, m_5c->m_20 the cell->tile map, m_4c the tile-object
// table. Reached by raw offset (engine struct, modeled minimally).
struct CTrigBoardGeo {
    char m_pad00[0x24];
    i32* m_24row;                     // +0x24  row base (cell index = m_24row[y] + x)
    char m_pad28[0x20 - 0x28 + 0x20]; // pad to +0x20-relative kept raw below
};
SIZE_UNKNOWN(CTrigBoardGeo);
struct CTrigBoard {
    char m_pad00[0x4c];
    CTileObj** m_4c; // +0x4c  tile-object table (indexed by the resolved tile id & 0xffff)
    char m_pad50[0x5c - 0x50];
    i32* m_5c; // +0x5c  board geometry block (bounds @0x28/0x2c, row-base @0x24, cell-map @0x20)
};
SIZE_UNKNOWN(CTrigBoard);
struct CTrigMgrInner {
    char m_pad00[0x24];
    CTrigBoard* m_24; // +0x24
};
SIZE_UNKNOWN(CTrigMgrInner);
struct CTrigMgr {
    char m_pad00[0x30];
    CTrigMgrInner* m_world; // +0x30
};
SIZE_UNKNOWN(CTrigMgr);
extern CTrigMgr* g_mgrSettings; // ?g_mgrSettings (0x64556c)

// The built trigger-logic object. The 14 distinct ctor thunks (0-arg __thiscall,
// returning the object) and the 3 register thunks (4-arg __thiscall, returning success)
// are reloc-masked externals reached through the incremental-link thunk table.
//
// DISPOSITION (CTrigLogic8c/9c/C8): these are NOT single classes - each is a SIZE
// BUCKET (0x8c/0x9c/0xc8 bytes) that the switch allocates for SEVERAL distinct real
// leaf classes (e.g. CTrigLogic8c covers 6 different trigger-logic leaves). A single
// real class name can't stand for a size bucket, so the buckets stay size-tagged;
// but each per-id ctor selector IS one real RTTI class, resolved by ILT-jmp target
// and now named for it (the tag/New* pairs below -> CTileTriggerSwitchLogic /
// CTileMultiTriggerSwitchLogic / ... / CGiantRockLogic, defs in
// TileTriggerDerivedCtors.cpp + the two base ctors 0x110430/0x1107f0).
struct CTileTriggerFactory; // the owning factory (this); back-stamped into m_20/m_24

struct CTrigLogic {
    struct TileTriggerSwitchLogicTag {};
    struct TileMultiTriggerSwitchLogicTag {};
    struct TileExclusiveTriggerSwitchLogicTag {};
    struct TileSecretTriggerSwitchLogicTag {};
    struct TileTimeTriggerSwitchLogicTag {};
    struct CheckpointTriggerSwitchLogicTag {};
    struct TileTriggerLogicTag {};
    struct GiantRockLogicTag {};
    struct TileTimeTriggerLogicTag {};
    struct TileSecretTriggerLogicTag {};
    struct CoveredPowerupLogicTag {};

    i32 m_00; // +0x00
    i32 m_04; // +0x04  type id (the switch id 1..26)
    i32 m_08; // +0x08  (id 21: board x)
    i32 m_0c; // +0x0c  (id 21: board y)
    char m_pad10[0x20 - 0x10];
    CTileTriggerFactory* m_20; // +0x20  owner (1abe / 1d39 group)
    CTileTriggerFactory* m_24; // +0x24  owner (277f group)
    char m_pad28[0x74 - 0x28]; // +0x28..+0x73 (folds the unused +0x70 owner slot)

    // Each thunk (ILT 0xNNNN -> the real leaf ctor RVA) constructs one RTTI class;
    // resolved from the ILT jmp target (TileTriggerDerivedCtors.cpp / the base ctors).
    CTrigLogic*
    NewTileTriggerSwitchLogic(); // ILT 0x3206 -> ??0CTileTriggerSwitchLogic 0x110430 (ids 1,2,5)
    CTrigLogic*
    NewTileMultiTriggerSwitchLogic(); // ILT 0x3eb3 -> ??0CTileMultiTriggerSwitchLogic 0x111f10 (id 3)
    CTrigLogic*
    NewTileExclusiveTriggerSwitchLogic(); // ILT 0x4192 -> ??0CTileExclusiveTriggerSwitchLogic 0x112050 (id 4)
    CTrigLogic*
    NewTileSecretTriggerSwitchLogic(); // ILT 0x2db5 -> ??0CTileSecretTriggerSwitchLogic 0x112790 (id 6)
    CTrigLogic*
    NewTileTimeTriggerSwitchLogic(); // ILT 0x332d -> ??0CTileTimeTriggerSwitchLogic 0x1127c0 (id 7)
    CTrigLogic*
    NewCheckpointTriggerSwitchLogic(); // ILT 0x2f72 -> ??0CCheckpointTriggerSwitchLogic 0x1127f0 (id 8)
    CTrigLogic* NewTileTriggerLogic(); // ILT 0x43b3 -> ??0CTileTriggerLogic 0x1107f0 (ids 21,24)
    CTrigLogic* NewGiantRockLogic();   // ILT 0x2c3e -> ??0CGiantRockLogic 0x112210 (id 22)
    CTrigLogic*
    NewTileTimeTriggerLogic(); // ILT 0x18de -> ??0CTileTimeTriggerLogic 0x112270 (id 23)
    CTrigLogic*
    NewTileSecretTriggerLogic(); // ILT 0x310c -> ??0CTileSecretTriggerLogic 0x112760 (id 25)
    CTrigLogic* NewCoveredPowerupLogic(); // ILT 0x2a4f -> ??0CCoveredPowerupLogic 0x112240 (id 26)
    i32 Reg277f(void* r, i32 k, i32 a2, i32 a3); // 0x277f (ids 1..8)
    i32 Reg1abe(void* r, i32 k, i32 a2, i32 a3); // 0x1abe (ids 21,23..26)
    i32 Reg1d39(void* r, i32 k, i32 a2, i32 a3); // 0x1d39 (id 22)
};
SIZE_UNKNOWN(CTrigLogic);

struct CTrigLogic8c : public CTrigLogic {
    inline CTrigLogic8c(CTrigLogic::TileTriggerSwitchLogicTag) {
        NewTileTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileMultiTriggerSwitchLogicTag) {
        NewTileMultiTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileExclusiveTriggerSwitchLogicTag) {
        NewTileExclusiveTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileSecretTriggerSwitchLogicTag) {
        NewTileSecretTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileTimeTriggerSwitchLogicTag) {
        NewTileTimeTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::CheckpointTriggerSwitchLogicTag) {
        NewCheckpointTriggerSwitchLogic();
    }

    char m_sizePad[0x8c - 0x74];
};
SIZE_UNKNOWN(CTrigLogic8c);

struct CTrigLogic9c : public CTrigLogic {
    inline CTrigLogic9c(CTrigLogic::TileTriggerLogicTag) {
        NewTileTriggerLogic();
    }
    inline CTrigLogic9c(CTrigLogic::TileTimeTriggerLogicTag) {
        NewTileTimeTriggerLogic();
    }
    inline CTrigLogic9c(CTrigLogic::TileSecretTriggerLogicTag) {
        NewTileSecretTriggerLogic();
    }
    inline CTrigLogic9c(CTrigLogic::CoveredPowerupLogicTag) {
        NewCoveredPowerupLogic();
    }

    char m_sizePad[0x9c - 0x74];
};
SIZE_UNKNOWN(CTrigLogic9c);

struct CTrigLogicC8 : public CTrigLogic {
    inline CTrigLogicC8(CTrigLogic::GiantRockLogicTag) {
        NewGiantRockLogic();
    }

    char m_sizePad[0xc8 - 0x74];
};
SIZE_UNKNOWN(CTrigLogicC8);

// The factory container (this): the built object's owner; id 21 latches the object
// into this->m_70.
struct CTileTriggerFactory {
    char m_pad00[0x70];
    CTrigLogic* m_70; // +0x70  id-21 latches the built object here

    void* Build(CSerialArchive* reader, i32 kind, i32 a2, i32 a3); // 0x117800
};
SIZE_UNKNOWN(CTileTriggerFactory);

// Build the 277f-group object: alloc 0x8c, run `ctor`, register, stamp owner+id.
static void* Reg277fTail(
    CTileTriggerFactory* self,
    CTrigLogic* obj,
    CSerialArchive* reader,
    i32 a2,
    i32 a3,
    i32 id
) {
    if (Gate113860((void*)reader, 7, a2, a3) == 0) {
        return 0;
    }
    obj->m_24 = self;
    obj->m_04 = id;
    return obj;
}

// Build the 1abe-group object tail: register, stamp owner+id.
static void* Reg1abeTail(
    CTileTriggerFactory* self,
    CTrigLogic* obj,
    CSerialArchive* reader,
    i32 a2,
    i32 a3,
    i32 id
) {
    if (Gate113860((void*)reader, 7, a2, a3) == 0) {
        return 0;
    }
    obj->m_20 = self;
    obj->m_04 = id;
    return obj;
}

// @early-stop
// 0x47f (1151 B) /GX compact-switch factory. The body reproduces the reader read, the
// dense id 1..26 switch (the documented MSVC byte-index + jump-table wall: id->case map
// recovered from 0x517cbc/0x517c80), every per-case Rez-alloc + ctor + register + owner
// stamp, and the id 21 board-tile gate. The plateau is the jump-table/reloc-typing wall
// + the per-`new` /GX trylevel state machine (each case carries its own EH state, which
// MSVC tail-merges differently from the helper-factored spelling here) + the differently
// -named ctor/register reloc operands. Logic complete; byte-match parked for the final sweep.
RVA(0x00117800, 0x47f)
void* CTileTriggerFactory::Build(CSerialArchive* reader, i32 kind, i32 a2, i32 a3) {
    if (reader == 0) {
        return 0;
    }
    if (kind != 7) {
        return 0;
    }
    i32 id;
    reader->Read(&id, 4);
    switch (id) {
        case 1:
        case 2:
        case 5: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 3: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileMultiTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 4: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileExclusiveTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 6: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileSecretTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 7: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileTimeTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 8: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::CheckpointTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 21: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileTriggerLogicTag());
            if (Gate113860((void*)reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_20 = this;
            obj->m_04 = id;
            // resolve the board tile under the object; latch on a 0x67/0x68 tile.
            CTrigBoard* board = g_mgrSettings->m_world->m_24;
            i32 x = obj->m_08;
            i32 y = obj->m_0c;
            i32* geo = board->m_5c;
            if (x < 0) {
                x = 0;
            } else if (x >= geo[0x28 / 4]) {
                x = geo[0x28 / 4] - 1;
            }
            if (y < 0) {
                y = 0;
            } else if (y >= geo[0x2c / 4]) {
                y = geo[0x2c / 4] - 1;
            }
            i32* rowbase = (i32*)geo[0x24 / 4];
            i32 cell = rowbase[y] + x;
            i32 tile = ((i32*)geo[0x20 / 4])[cell];
            i32 type;
            if (tile == (i32)0xeeeeeeee || tile == -1) {
                type = 0;
            } else {
                type = board->m_4c[tile & 0xffff]->TypeId();
            }
            if (type == 0x67 || type == 0x68) {
                this->m_70 = obj;
            }
            return obj;
        }
        case 22: {
            CTrigLogic* obj = new CTrigLogicC8(CTrigLogic::GiantRockLogicTag());
            if (Gate113860((void*)reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_20 = this;
            obj->m_04 = id;
            return obj;
        }
        case 23: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileTimeTriggerLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        case 24: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileTriggerLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        case 25: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileSecretTriggerLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        case 26: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::CoveredPowerupLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        default:
            return 0;
    }
}

// --- vtable catalog ---
VTBL(CTileObj, 0x001e8cb4);
