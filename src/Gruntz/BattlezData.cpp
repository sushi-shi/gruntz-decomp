// BattlezData.cpp - CBattlezData, the multiplayer (Battlez) progress/score
// tracker held at g_gameReg->m_7c (see include/Gruntz/BattlezData.h). A 0x388-
// byte, non-polymorphic helper the game manager owns: a ctor + accessors over
// two 4x4 int grids (a head-to-head win matrix and a flag matrix) and an array
// of per-map records, plus a flat Serialize.
#include <Gruntz/BattlezData.h>
#include <rva.h>

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). Minimal local
// view: FillRecord folds reg->m_118 into each record. The DATA pin reloc-masks
// the `mov ds:g_gameReg` load against the already-named symbol.
struct CGameReg {
    char m_pad00[0x118];
    i32 m_118; // +0x118
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// 0xfc9c0 - the (re)initialize-with-records entry: run Init() on this, then bind
// the record array. (The object is raw-`operator new`d by the game manager and
// driven through these two plain Init methods; there is no C++ ctor.)
RVA(0x000fc9c0, 0x17)
i32 CBattlezData::InitWithRecords(void* records) {
    Init();
    m_records = (BattlezRecord*)records;
    return 1;
}

// 0xfca10 - Init: zero the scalar band + the matrices, then the four large
// zeroed bands (each a rep-stosd run Init open-codes).
// @early-stop
// ~81%, logic byte-exact. Zero-register pinning wall (docs/patterns/zero-register-pinning.md):
// retail holds the zero in eax (re-zeroing after ClearFlags/ClearWins), we pin
// it in callee-saved edi; pure regalloc coin-flip, no source lever flips it.
RVA(0x000fca10, 0x8a)
void CBattlezData::Init() {
    m_count = 0;
    m_08 = 0;
    m_allDone = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 0;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_2c = 0;
    m_44 = 0;
    m_34 = 0;
    m_30 = 0;
    m_38 = 0;
    m_3c = 0;
    m_40 = 0;
    ClearFlags();
    ClearWins();
    m_48[0] = 0;
    m_48[1] = 0;
    m_48[2] = 0;
    m_48[3] = 0;
    i32 i;
    for (i = 0; i < 88; i++) {
        m_band_d8[i] = 0;
    }
    for (i = 0; i < 40; i++) {
        m_band_238[i] = 0;
    }
    for (i = 0; i < 28; i++) {
        m_band_2d8[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        m_band_348[i] = 0;
    }
}

// 0xfcad0 - set the record count; flag whether the current group-of-4 of records
// is fully populated (each record's +0x00 non-zero). Counts above 0x24 force the
// flag off.
RVA(0x000fcad0, 0x53)
void CBattlezData::SetCount(i32 count) {
    m_count = count;
    if (count > 0x24) {
        m_allDone = 0;
        return;
    }
    i32 base = (count - 1) / 4 * 4;
    i32 flag = 1;
    for (i32 i = base; i < base + 4; i++) {
        if (m_records[i].m_00 == 0) {
            flag = 0;
        }
    }
    m_allDone = flag;
}

// 0xfcb50 - set flags[y][x] = 1 (both bounded to 0..4 inclusive).
RVA(0x000fcb50, 0x2b)
void CBattlezData::MarkFlag(i32 y, i32 x) {
    if (y >= 0 && y <= 4 && x >= 0 && x <= 4) {
        m_flags[x + y * 4] = 1;
    }
}

// 0xfcb90 - clear the flag matrix.
RVA(0x000fcb90, 0x12)
void CBattlezData::ClearFlags() {
    for (i32 i = 0; i < 16; i++) {
        m_flags[i] = 0;
    }
}

// 0xfcbc0 - sum all 16 flags (the argument is range-gated but unused in the sum).
RVA(0x000fcbc0, 0x3a)
i32 CBattlezData::SumFlags(i32 y) {
    if (y < 0 || y > 4) {
        return 0;
    }
    i32 sum = 0;
    i32* p = m_flags;
    for (i32 r = 0; r < 4; r++) {
        for (i32 c = 0; c < 4; c++) {
            sum += *p++;
        }
    }
    return sum;
}

// 0xfcc10 - read flags[x][y] (both bounded 0..4). The row base is computed first
// (this + x*0x10 + 0x98) and indexed by y, matching the retail addressing.
RVA(0x000fcc10, 0x2f)
i32 CBattlezData::GetFlag(i32 x, i32 y) {
    if (x >= 0 && x <= 4 && y >= 0 && y <= 4) {
        return *(i32*)((char*)m_flags + x * 0x10 + y * 4);
    }
    return 0;
}

// 0xfcc50 - bump wins[y][x] for off-diagonal (y!=x) cells (both bounded 0..4).
RVA(0x000fcc50, 0x2a)
void CBattlezData::BumpWin(i32 y, i32 x) {
    if (y >= 0 && y <= 4 && x >= 0 && x <= 4 && y != x) {
        m_wins[x + y * 4]++;
    }
}

// 0xfcc90 - clear the win matrix.
RVA(0x000fcc90, 0xf)
void CBattlezData::ClearWins() {
    for (i32 i = 0; i < 16; i++) {
        m_wins[i] = 0;
    }
}

// 0xfccb0 - sum row y of the win matrix (4 cells). The row base is this + y*0x10
// + 0x58, matching the retail `shl y,4; lea` addressing.
RVA(0x000fccb0, 0x21)
i32 CBattlezData::SumWinRow(i32 y) {
    i32 sum = 0;
    i32* p = (i32*)((char*)m_wins + y * 0x10);
    for (i32 c = 0; c < 4; c++) {
        sum += *p++;
    }
    return sum;
}

// 0xfcd70 - gated "within bounds" test: only meaningful when m_44 is set; then
// the m_30..m_40 band must each stay <= the m_14..m_2c band. Takes one (unused)
// stack argument (retail cleans 4 bytes on return).
RVA(0x000fcd70, 0x61)
i32 CBattlezData::InBounds(i32 unused) {
    if (m_44 == 0) {
        return 0;
    }
    if (m_30 > m_14) {
        return 0;
    }
    if (m_34 > m_18) {
        return 0;
    }
    if (m_38 > m_24) {
        return 0;
    }
    if (m_3c > m_28) {
        return 0;
    }
    return m_40 <= m_2c;
}

// 0xfced0 - the record's win/score value at the wrap index, or m_44 when the
// wrapped index lands on the last record.
// @early-stop
// ~76%, logic byte-exact. Two non-steerable codegen choices: retail's `lea
// esi,[eax-1]` (vs our mov+dec for `last=m_count-1`) and the base/index SIB
// canonicalization `[eax+ecx+0x28]` vs `[ecx+eax+0x28]` (same address).
RVA(0x000fced0, 0x31)
i32 CBattlezData::GetRecordValue(i32 b) {
    i32 last = m_count - 1;
    i32 idx = b + last / 4 * 4;
    if (idx == last) {
        return m_44;
    }
    return m_records[idx].m_28;
}

// 0xfd330 - fill the record at `index` (0x40-byte stride, biased -0x40) from the
// current m_10..m_44 band; phase 0 writes the head fields (+m_118 from the
// registry), any other phase writes the tail.
RVA(0x000fd330, 0x84)
void CBattlezData::FillRecord(i32 index, i32 phase) {
    i32* rec = (i32*)((char*)m_records + index * 0x40 - 0x40);
    if (phase == 0) {
        rec[0] = 1;
        rec[2] = m_10;
        rec[3] = m_14;
        rec[4] = m_18;
        rec[5] = m_1c;
        rec[6] = m_20;
        rec[7] = m_24;
        rec[8] = m_28;
        rec[9] = m_2c;
        rec[10] = m_44;
        rec[1] = g_gameReg->m_118;
    } else {
        rec[11] = m_30;
        rec[12] = m_34;
        rec[13] = m_38;
        rec[14] = m_3c;
        rec[15] = m_40;
    }
}

// 0xfd3f0 - flat serialize: op 7 reads, op 4 writes. The 17 leading scalars
// (m_count..m_44) are streamed UNROLLED; the m_48 band and the four nested 4xN
// grids/bands are streamed in counted loops. The op==4 (write) test is the
// forward `je`; op==7 (read) is the fall-through block.
// @early-stop
// ~92%, logic byte-exact. Residual is the shrink-wrapped prologue
// (docs/patterns/shrink-wrapped-callee-save-push.md): retail defers push
// edi/ebx past the null guard (pushing only ecx/ebp/esi upfront), which shifts
// every spill-slot offset (`[esp+0x18]` vs `[esp+0x14]`) downstream. Not source-steerable.
RVA(0x000fd3f0, 0x425)
i32 CBattlezData::Serialize(BattlezStream* s, i32 op, i32 a2, i32 a3) {
    i32* p;
    i32 i;
    i32 r;
    i32 c;
    if (s == 0) {
        return 0;
    }
    if (op != 4) {
        if (op == 7) {
            s->Read(&m_count, 4);
            s->Read(&m_08, 4);
            s->Read(&m_allDone, 4);
            s->Read(&m_10, 4);
            s->Read(&m_14, 4);
            s->Read(&m_18, 4);
            s->Read(&m_1c, 4);
            s->Read(&m_20, 4);
            s->Read(&m_24, 4);
            s->Read(&m_28, 4);
            s->Read(&m_2c, 4);
            s->Read(&m_30, 4);
            s->Read(&m_34, 4);
            s->Read(&m_38, 4);
            s->Read(&m_3c, 4);
            s->Read(&m_40, 4);
            s->Read(&m_44, 4);
            for (p = m_48, i = 0; i < 4; i++, p++) {
                s->Read(p, 4);
            }
            p = m_wins;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_flags;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_band_d8;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 22; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_band_238;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 10; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_band_2d8;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 7; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_band_348;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, 4);
                }
            }
        }
        return 1;
    }
    s->Write(&m_count, 4);
    s->Write(&m_08, 4);
    s->Write(&m_allDone, 4);
    s->Write(&m_10, 4);
    s->Write(&m_14, 4);
    s->Write(&m_18, 4);
    s->Write(&m_1c, 4);
    s->Write(&m_20, 4);
    s->Write(&m_24, 4);
    s->Write(&m_28, 4);
    s->Write(&m_2c, 4);
    s->Write(&m_30, 4);
    s->Write(&m_34, 4);
    s->Write(&m_38, 4);
    s->Write(&m_3c, 4);
    s->Write(&m_40, 4);
    s->Write(&m_44, 4);
    for (p = m_48, i = 0; i < 4; i++, p++) {
        s->Write(p, 4);
    }
    p = m_wins;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_flags;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_band_d8;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 22; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_band_238;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 10; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_band_2d8;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 7; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_band_348;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++, p++) {
            s->Write(p, 4);
        }
    }
    return 1;
}
