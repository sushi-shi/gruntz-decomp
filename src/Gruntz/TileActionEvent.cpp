// TileActionEvent.cpp - the per-tile game-action event record (trace placeholder
// ClassUnknown_108). Methods in ascending retail-RVA order. The record shape and
// the CTileActionArchive serializer come from <Gruntz/TileActionEvent.h>; the game
// registry singleton (g_gameReg) is modeled here with only the offsets these paths
// touch. All engine callees are reloc-masked (no body).
//
// BANKED (code byte-exact, 100% fuzzy): ResetFlag (0x112d80), SetActionCode
//   (0x112da0), MorphByTool (0x113420), Serialize (0x113f10), SerializeFields
//   (0x113f60). The big Process (0x112ee0) is a complete logical reconstruction
//   parked at the two-jump-table wall (@early-stop) for the final sweep.
#include <Win32.h>

#include <Gruntz/TileActionEvent.h>

#include <rva.h>

// ---------------------------------------------------------------------------
// The game registry singleton (?g_gameReg@@3PAUWwdGameRegZ@@A at VA 0x64556c).
// Only the offsets this cluster reaches are modeled; reloc-masked DIR32.
// ---------------------------------------------------------------------------

// The action-occupancy tile grid reached as g->m_30->m_24->m_5c: a flat cell array
// at +0x20, a per-row start-index table at +0x24. cell = m_20[ m_24[y] + x ].
struct CActionGrid {
    char m_pad0[0x20];
    i32* m_20; // +0x20  flat cell array (action codes per tile)
    i32* m_24; // +0x24  per-row start-index table (m_24[y])
};

struct WwdGrViewport {
    char m_pad0[0x5c];
    CActionGrid* m_5c; // +0x5c
};

// The sprite factory the brick-break path spawns through (g->m_30->m_8). Process
// also reads g->m_30->m_28->m_30 (a one-shot impact-sound gate). External slots.
struct CSpriteMaker {
    void* CreateSprite(i32 a0, i32 x, i32 y, i32 z, void* name, i32 flags); // 0x1597b0
};
struct CSpriteMakerSub {
    char m_pad0[0x30];
    i32 m_30; // +0x30  impact-sound already-played gate
};
struct WwdGrSprHolder {
    char m_pad0[0x8];
    CSpriteMaker* m_8; // +0x08  sprite factory (CreateSprite)
    char m_pad0c[0x24 - 0xc];
    WwdGrViewport* m_24;   // +0x24  the SetActionCode grid viewport
    CSpriteMakerSub* m_28; // +0x28  impact-gate holder
};

// The message/effect sink (g->m_68): PostTileEvent posts coordinate-tagged events
// to the game; external/no-body so the dispatch reloc-masks.
struct CTileEventSink {
    void PostA(i32 x, i32 y, i32 a2, i32 a3, i32 a4); // 0x40400c  (edi==0x138 path)
    void PostB(i32 x, i32 y, i32 a2, i32 a3);         // 0x402fb3  (edi==0x144 path)
};

struct WwdGameRegZ {
    char m_pad0[0x30];
    WwdGrSprHolder* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    CTileEventSink* m_68; // +0x68  tile-event sink
    char m_pad6c[0x70 - 0x6c];
    void* m_70; // +0x70  the action-grid manager (RefreshTile)
    char m_pad74[0x13c - 0x74];
    i32 m_13c; // +0x13c view min X
    i32 m_140; // +0x140 view min Y
    i32 m_144; // +0x144 view max X
    i32 m_148; // +0x148 view max Y
};

DATA(0x0064556c)
extern WwdGameRegZ* g_gameReg;

// The shared global at DAT_00644c54 (VA 0x644c54): used here as the per-player /
// active-slot index into m_18[]. Already named g_tileKindMagic by the
// leveltilevalidation unit; reuse that name so the DIR32 reloc pairs by symbol.
extern i32 g_tileKindMagic;

// The impact-sound sink param (DAT_0061ab24): Process plays the impact one-shot
// through it (Play(sink, 0,0,0)). Already named g_scrollDelta by the chatbox unit;
// reuse that name so the DIR32 reloc pairs by symbol (no competing DATA - chatbox
// owns the address; Process is deferred so its pairing is non-critical anyway).
extern i32 g_scrollDelta;

// The grid manager method the action-set path runs after stamping a tile
// (thunk_FUN_00477790, __thiscall ret 0). External/no-body -> reloc-masked.
struct CActionGridMgr {
    void RefreshTile();
};

// The brick / tile-object passed as Process's arg (ebx). External methods modeled
// no-body so their __thiscall dispatch reloc-masks.
struct CBrickTile {
    void Detonate(i32 a0, i32 a1, i32 a2, i32 a3); // 0x403bd9 (edi==0x132 path)
    i32 m_8;   // +0x08  flag word (|= 0x10000 on uncached default break)
    i32 m_198; // +0x198 cache-state gate
    i32 m_1e4; // +0x1e4 cleared on edi==0x132
    i32 m_1ec; // +0x1ec brick-color / slot discriminator (==5 -> all-slots)
};

// The spawned brick-break sprite (CreateSprite result): ApplyAnim selects the
// break animation by name, CacheFrame caches its first frame. External no-body.
struct CBreakSprite {
    void ApplyAnim(void* name, i32 a1); // 0x1505b0
    void CacheFrame(void* name);        // 0x150540
    i32 m_8;                            // +0x08  flag word
    i32 m_198;                          // +0x198 cache-state gate
};

// The impact one-shot emitter vtable slot used on g->m_30->m_28->m_30==0.
struct CImpactSound {
    void Play(void* sink, i32 a1, i32 a2, i32 a3); // 0x4025fe
};

// The sound-bank lookup by name (thunk_FUN_0045b7e0, __cdecl). External no-body.
extern CImpactSound* Eng_FindSound(const char* name);

// ===========================================================================
// CTileActionEvent::ResetFlag  (0x112d80) - __thiscall
// ===========================================================================
// Zero the m_10 flag word and return this (MSVC moves ecx->eax for the return).
RVA(0x00112d80, 0xa)
CTileActionEvent* CTileActionEvent::ResetFlag() {
    m_10 = 0;
    return this;
}

// ===========================================================================
// CTileActionEvent::SetActionCode  (0x112da0) - __thiscall, ret 4
// ===========================================================================
// Stamp the action code into m_0, fold it to a canonical kind (0x12f/0x130/0x131)
// via the dense byte-mapped remap switch unless this player's slot is already
// active, then write it into the action grid cell (g->m_30->m_24->m_5c flat cell
// = m_20[ m_24[y] + x ]); return 0 if the cell already held the code (no-op), else
// stamp it + run the grid-manager RefreshTile and return 1.
RVA(0x00112da0, 0x9e)
i32 CTileActionEvent::SetActionCode(i32 code) {
    m_0 = code;
    if (((i32*)((char*)this + 0x18))[g_tileKindMagic] == 0 && (u32)(code - 0x12f) <= 0x1a) {
        switch (code) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                code = 0x12f;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                code = 0x130;
                break;
            case 0x131:
            case 0x135:
            case 0x136:
            case 0x137:
            case 0x13b:
            case 0x13c:
            case 0x13d:
            case 0x141:
            case 0x142:
            case 0x143:
            case 0x147:
            case 0x148:
            case 0x149:
                code = 0x131;
                break;
        }
    }
    {
        CActionGrid* grid = g_gameReg->m_30->m_24->m_5c;
        i32* cell = &grid->m_20[grid->m_24[m_8] + m_4];
        if (*cell == code) {
            return 0;
        }
        *cell = code;
        ((CActionGridMgr*)g_gameReg->m_70)->RefreshTile();
        return 1;
    }
}

// ===========================================================================
// CTileActionEvent::Process  (0x112ee0) - __thiscall, ret 4
// ===========================================================================
// @early-stop
// 918-byte two-jump-table dispatch wall (outer remap switch on m_0 + inner
// brick-color switch on the derived effect code, four shl-5/add-0x10 coordinate
// scalings under heavy register pressure ebp=this/ebx=arg/esi=newCode/edi=effect).
// Logic complete + decoded; byte-match deferred to the final sweep.
//
// Outer switch(m_0): derive `effect` (edi, 0=none) and the canonical re-fire code
// `newCode` (esi). First-half: fire the per-effect game action on the brick arg.
// Second-half (always): if the tile is on-screen, spawn the brick-break sprite and
// pick its colored break animation by `effect`. Finally re-fire SetActionCode with
// `newCode` if it changed; return (newCode == 0x12d).
RVA(0x00112ee0, 0x35e)
i32 CTileActionEvent::Process(i32 arg) {
    i32 newCode = m_0;
    i32 effect = 0;
    switch (m_0) {
        case 0x12f:
            newCode = 0x12d;
            break;
        case 0x130:
            newCode = 0x12f;
            break;
        case 0x131:
            newCode = 0x130;
            break;
        case 0x132:
            effect = 0x132;
            newCode = 0x12d;
            break;
        case 0x133:
            newCode = 0x132;
            break;
        case 0x134:
            effect = 0x132;
            newCode = 0x12f;
            break;
        case 0x135:
            newCode = 0x133;
            break;
        case 0x136:
            newCode = 0x134;
            break;
        case 0x137:
            effect = 0x132;
            newCode = 0x130;
            break;
        case 0x138:
            effect = 0x138;
            newCode = 0x12d;
            break;
        case 0x139:
            newCode = 0x138;
            break;
        case 0x13a:
            effect = 0x138;
            newCode = 0x12f;
            break;
        case 0x13b:
            newCode = 0x139;
            break;
        case 0x13c:
            newCode = 0x13a;
            break;
        case 0x13d:
            effect = 0x138;
            newCode = 0x130;
            break;
        case 0x13e:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x12d;
            break;
        case 0x13f:
            newCode = 0x13e;
            break;
        case 0x140:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x12f;
            break;
        case 0x141:
            newCode = 0x13f;
            break;
        case 0x142:
            newCode = 0x140;
            break;
        case 0x143:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x130;
            break;
        case 0x144:
            effect = 0x144;
            newCode = 0x12d;
            break;
        case 0x145:
            newCode = 0x144;
            break;
        case 0x146:
            effect = 0x144;
            newCode = 0x12f;
            break;
        case 0x147:
            newCode = 0x145;
            break;
        case 0x148:
            newCode = 0x146;
            break;
        case 0x149:
            effect = 0x144;
            newCode = 0x130;
            break;
    }

    CBrickTile* brick = (CBrickTile*)arg;
    if (effect != 0 && brick != 0) {
        if (effect == 0x132) {
            brick->Detonate(0, 1, 0, 0);
            brick->m_1e4 = 0;
        } else if (effect == 0x138) {
            g_gameReg->m_68->PostA((m_4 << 5) + 0x10, (m_8 << 5) + 0x10, 1, 2, -1);
        } else if (effect == 0x13e) {
            i32 px = (m_4 << 5) + 0x10;
            i32 py = (m_8 << 5) + 0x10;
            if (px < g_gameReg->m_144 && px >= g_gameReg->m_13c && py < g_gameReg->m_148
                && py >= g_gameReg->m_140 && g_gameReg->m_30->m_28->m_30 == 0) {
                CImpactSound* snd = (CImpactSound*)Eng_FindSound("GRUNTZ_NORMALGRUNT_IMPACTMM3");
                if (snd != 0) {
                    snd->Play((void*)g_scrollDelta, 0, 0, 0);
                }
            }
            if (brick->m_1ec == 5) {
                m_18 = 1;
                m_1c = 1;
                m_20 = 1;
                m_24 = 1;
                SetActionCode(m_0);
                return 0;
            }
            (&m_18)[brick->m_1ec] = 1;
            SetActionCode(m_0);
            return 0;
        } else if (effect == 0x144) {
            g_gameReg->m_68->PostB((m_4 << 5) + 0x10, (m_8 << 5) + 0x10, -1, 2);
        }
    }

    i32 px = (m_4 << 5) + 0x10;
    i32 py = (m_8 << 5) + 0x10;
    if (px < g_gameReg->m_144 && px >= g_gameReg->m_13c && py < g_gameReg->m_148
        && py >= g_gameReg->m_140) {
        CBreakSprite* spr =
            (CBreakSprite*)
                g_gameReg->m_30->m_8->CreateSprite(0, px, py, 0xcf84f, (void*)"Particlez", 0x40003);
        if (spr != 0) {
            spr->ApplyAnim((void*)"GAME_BRICKBREAK", 0);
            // Inner dense byte-mapped switch on (effect - 0x132) -> the colored break
            // animation; effect 0x138->RED, 0x13d->BLUE, 0x142->GOLD, the remaining
            // mapped slots->BLACK, anything off-table->default GAME_BRICKBREAK (which
            // also sets the +0x8 uncached flag). The exact slot-0 (effect 0x132) and
            // 0x144 color assignments are the deferred byte-match residual.
            switch (effect) {
                case 0x138:
                    spr->CacheFrame((void*)"GAME_REDBRICKBREAK");
                    break;
                case 0x13d:
                    spr->CacheFrame((void*)"GAME_BLUEBRICKBREAK");
                    break;
                case 0x142:
                    spr->CacheFrame((void*)"GAME_GOLDBRICKBREAK");
                    break;
                default:
                    if (effect >= 0x133 && effect <= 0x144) {
                        spr->CacheFrame((void*)"GAME_BLACKBRICKBREAK");
                        break;
                    }
                    spr->CacheFrame((void*)"GAME_BRICKBREAK");
                    if (spr->m_198 == 0) {
                        spr->m_8 |= 0x10000;
                    }
                    break;
            }
        }
    }

    if (newCode != m_0) {
        SetActionCode(newCode);
    }
    return newCode == 0x12d;
}

// ===========================================================================
// CTileActionEvent::MorphByTool  (0x113420) - __thiscall, ret 8
// ===========================================================================
// Apply a tool/key (toolId 0x22..0x26 - the five brick-painting tools) to the
// current action code m_0: an `if/else-if` chain on toolId, each branch a dense
// jump-table switch(m_0) over the 0x12f..0x149 brick-code range that advances m_0
// to the tool's next code (table mappings recovered from the five byte-indexed
// switch tables at 0x513650/0x513694/0x5136d8/0x51371c/0x513760). A switch default
// (no transition for this tool+code) returns 0; any other unmatched toolId falls
// straight through to the shared tail. The tail zeroes the 4-slot per-player flag
// array (m_18..m_24), marks this player's slot (or all four when playerSlot==5),
// then re-commits the new code via SetActionCode and returns 1.
//
// Code byte-exact (objdiff fuzzy == base, same as the banked sibling SetActionCode);
// the only residual is the inline .rdata jump-table scoring artifact (the five dense
// switch tables' DIR32 base relocs pair against $L labels vs the self-symbol, and the
// SetActionCode call goes through the ILT thunk) - docs/patterns/jumptable-data-overlap.md.
// No code divergence; the metric undercount is the documented tooling wall.
RVA(0x00113420, 0x1f2)
i32 CTileActionEvent::MorphByTool(i32 toolId, i32 playerSlot) {
    if (toolId == 0x22) {
        switch (m_0) {
            case 0x12f: m_0 = 0x130; break;
            case 0x132: m_0 = 0x133; break;
            case 0x138: m_0 = 0x139; break;
            case 0x13e: m_0 = 0x13f; break;
            case 0x144: m_0 = 0x145; break;
            case 0x130: m_0 = 0x131; break;
            case 0x133: m_0 = 0x135; break;
            case 0x134: m_0 = 0x136; break;
            case 0x139: m_0 = 0x13b; break;
            case 0x13a: m_0 = 0x13c; break;
            case 0x13f: m_0 = 0x141; break;
            case 0x140: m_0 = 0x142; break;
            case 0x145: m_0 = 0x147; break;
            case 0x146: m_0 = 0x148; break;
            default: return 0;
        }
    } else if (toolId == 0x23) {
        switch (m_0) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144: m_0 = 0x134; break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146: m_0 = 0x137; break;
            default: return 0;
        }
    } else if (toolId == 0x24) {
        switch (m_0) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144: m_0 = 0x13a; break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146: m_0 = 0x13d; break;
            default: return 0;
        }
    } else if (toolId == 0x26) {
        switch (m_0) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144: m_0 = 0x146; break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146: m_0 = 0x149; break;
            default: return 0;
        }
    } else if (toolId == 0x25) {
        switch (m_0) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144: m_0 = 0x140; break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146: m_0 = 0x143; break;
            default: return 0;
        }
    }

    m_18 = 0;
    m_1c = 0;
    m_20 = 0;
    m_24 = 0;
    if (playerSlot == 5) {
        m_18 = 1;
        m_1c = 1;
        m_20 = 1;
        m_24 = 1;
    } else {
        (&m_18)[playerSlot] = 1;
    }
    SetActionCode(m_0);
    return 1;
}

// ===========================================================================
// CTileActionEvent::Serialize  (0x113f10) - __thiscall, ret 0x10
// ===========================================================================
// The dispatcher: mode 4 writes the record fields, mode 7 reads them back; any
// other mode (or a null archive arg) is a no-op returning 1. The two field-stream
// helpers share the (edi=this record, esi=archive) shape.
RVA(0x00113f10, 0x3b)
i32 CTileActionEvent::Serialize(void* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (SerializeFields(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (DeserializeFields(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ===========================================================================
// CTileActionEvent::SerializeFields  (0x113f60) - __thiscall, ret 4
// ===========================================================================
// Stream the 9 record fields (m_0,m_4,m_8,m_c,m_10,m_18,m_1c,m_20,m_24 - m_14 is
// skipped) through the archive ar's vtable slot +0x30 (write, 4 bytes each).
// Returns 0 if ar or the registry's +0x30 sub-object is null, else 1.
RVA(0x00113f60, 0xa2)
i32 CTileActionEvent::SerializeFields(void* ar) {
    CTileActionArchive* a = (CTileActionArchive*)ar;
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    a->Write(&m_0, 4);
    a->Write(&m_4, 4);
    a->Write(&m_8, 4);
    a->Write(&m_c, 4);
    a->Write(&m_10, 4);
    a->Write(&m_18, 4);
    a->Write(&m_1c, 4);
    a->Write(&m_20, 4);
    a->Write(&m_24, 4);
    return 1;
}

// ===========================================================================
// CTileActionEvent::DeserializeFields  (0x114040) - __thiscall, ret 4
// ===========================================================================
// The mode-7 read counterpart of SerializeFields: read the same 9 record fields
// (m_0,m_4,m_8,m_c,m_10,m_18,m_1c,m_20,m_24 - m_14 skipped) back through the archive
// ar's vtable slot +0x2c (read, 4 bytes each). Returns 0 if ar or the registry's
// +0x30 sub-object is null, else 1.
RVA(0x00114040, 0xa2)
i32 CTileActionEvent::DeserializeFields(void* ar) {
    CTileActionArchive* a = (CTileActionArchive*)ar;
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    a->Read(&m_0, 4);
    a->Read(&m_4, 4);
    a->Read(&m_8, 4);
    a->Read(&m_c, 4);
    a->Read(&m_10, 4);
    a->Read(&m_18, 4);
    a->Read(&m_1c, 4);
    a->Read(&m_20, 4);
    a->Read(&m_24, 4);
    return 1;
}
