// BattlezData.cpp - CBattlezData, the multiplayer (Battlez) progress/score
// tracker held at g_gameReg->m_7c (see include/Gruntz/BattlezData.h). A 0x388-
// byte, non-polymorphic helper the game manager owns: a ctor + accessors over
// two 4x4 int grids (a head-to-head win matrix and a flag matrix) and an array
// of per-map records, plus a flat Serialize.
#include <Gruntz/BattlezData.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Globals.h>

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). Minimal local
// view: FillRecord folds reg->m_118 into each record. The DATA pin reloc-masks
// the `mov ds:g_gameReg` load against the already-named symbol.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

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
    m_scoreValue = 0;
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
        if (m_records[i].m_populated == 0) {
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

// 0xfccf0 - "every record is populated and within its own bounds": walk all
// 0x20 records, fail (return 0) on the first whose +0x00/+0x28 is zero or whose
// progress field falls below the matching bound. The per-record tests mirror
// InBounds' member-band comparisons, applied to each record's own fields.
RVA(0x000fccf0, 0x57)
i32 CBattlezData::AllRecordsInBounds() {
    i32 i = 0;
    BattlezRecord* r = m_records;
    for (; i < 0x20; i++, r++) {
        if (r->m_populated == 0) {
            return 0;
        }
        if (r->m_scoreValue == 0) {
            return 0;
        }
        if (r->m_10 < r->m_30) {
            return 0;
        }
        if (r->m_0c < r->m_2c) {
            return 0;
        }
        if (r->m_1c < r->m_34) {
            return 0;
        }
        if (r->m_20 < r->m_38) {
            return 0;
        }
        if (r->m_24 < r->m_3c) {
            return 0;
        }
    }
    return 1;
}

// 0xfcd70 - gated "within bounds" test: only meaningful when m_scoreValue is set; then
// the m_30..m_40 band must each stay <= the m_14..m_2c band. Takes one (unused)
// stack argument (retail cleans 4 bytes on return).
RVA(0x000fcd70, 0x61)
i32 CBattlezData::InBounds(i32 unused) {
    if (m_scoreValue == 0) {
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

// 0xfce00 - ratio over the current group of 4 records: (Sum m_24) / (Sum m_3c),
// or 0 when the m_3c sum is zero. Both sums are accumulated in float (fild);
// g_zeroF (0.0f) seeds the accumulators and the divide-by-zero guard.
RVA(0x000fce00, 0x56)
float CBattlezData::GroupRatio() {
    float den = g_zeroF;
    float num = g_zeroF;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = 0; i < 4; i++) {
        den += m_records[g + i].m_3c;
        num += m_records[g + i].m_24;
    }
    if (g_zeroF == den) {
        return g_zeroF;
    }
    return num / den;
}

// 0xfce80 - true iff all 4 records in the current group are scored (m_28 != 0).
RVA(0x000fce80, 0x32)
i32 CBattlezData::GroupAllScored() {
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = 0; i < 4; i++) {
        if (m_records[g + i].m_scoreValue == 0) {
            return 0;
        }
    }
    return 1;
}

// 0xfcf20 - sum field m_0c over the 4 records in the current group.
RVA(0x000fcf20, 0x37)
i32 CBattlezData::SumGroupField0c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_0c;
    }
    return sum;
}

// 0xfcf70 - sum field m_2c over the 4 records in the current group.
RVA(0x000fcf70, 0x37)
i32 CBattlezData::SumGroupField2c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_2c;
    }
    return sum;
}

// 0xfcfc0 - sum field m_10 over the 4 records in the current group.
RVA(0x000fcfc0, 0x37)
i32 CBattlezData::SumGroupField10() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_10;
    }
    return sum;
}

// 0xfd010 - sum field m_30 over the 4 records in the current group.
RVA(0x000fd010, 0x37)
i32 CBattlezData::SumGroupField30() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_30;
    }
    return sum;
}

// 0xfd060 - sum field m_1c over the 4 records in the current group.
RVA(0x000fd060, 0x37)
i32 CBattlezData::SumGroupField1c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_1c;
    }
    return sum;
}

// 0xfd0b0 - sum field m_34 over the 4 records in the current group.
RVA(0x000fd0b0, 0x37)
i32 CBattlezData::SumGroupField34() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_34;
    }
    return sum;
}

// 0xfd100 - sum field m_20 over the 4 records in the current group.
RVA(0x000fd100, 0x37)
i32 CBattlezData::SumGroupField20() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_20;
    }
    return sum;
}

// 0xfd150 - sum field m_38 over the 4 records in the current group.
RVA(0x000fd150, 0x37)
i32 CBattlezData::SumGroupField38() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_38;
    }
    return sum;
}

// 0xfd1a0 - sum field m_24 over the 4 records in the current group.
RVA(0x000fd1a0, 0x37)
i32 CBattlezData::SumGroupField24() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_24;
    }
    return sum;
}

// 0xfd1f0 - sum field m_3c over the 4 records in the current group.
RVA(0x000fd1f0, 0x37)
i32 CBattlezData::SumGroupField3c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_3c;
    }
    return sum;
}

// 0xfd240 - sum field m_18 over the 4 records in the current group.
RVA(0x000fd240, 0x37)
i32 CBattlezData::SumGroupField18() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_18;
    }
    return sum;
}

// 0xfd290 - sum field m_14 over the 4 records in the current group.
RVA(0x000fd290, 0x37)
i32 CBattlezData::SumGroupField14() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_14;
    }
    return sum;
}

// 0xfd2e0 - sum field m_08 over the 4 records in the current group.
RVA(0x000fd2e0, 0x37)
i32 CBattlezData::SumGroupField08() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_08;
    }
    return sum;
}

// 0xfced0 - the record's win/score value at the wrap index, or m_scoreValue when the
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
        return m_scoreValue;
    }
    return m_records[idx].m_scoreValue;
}

// 0xfd330 - fill the record at `index` (0x40-byte stride, biased -0x40) from the
// current m_10..m_scoreValue band; phase 0 writes the head fields (+m_118 from the
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
        rec[10] = m_scoreValue;
        rec[1] = g_gameReg->m_isEasyMode;
    } else {
        rec[11] = m_30;
        rec[12] = m_34;
        rec[13] = m_38;
        rec[14] = m_3c;
        rec[15] = m_40;
    }
}

// 0xfd3f0 - flat serialize: op 7 reads, op 4 writes. The 17 leading scalars
// (m_count..m_scoreValue) are streamed UNROLLED; the m_48 band and the four nested 4xN
// grids/bands are streamed in counted loops. The op==4 (write) test is the
// forward `je`; op==7 (read) is the fall-through block.
// @early-stop
// ~92%, logic byte-exact. Residual is the shrink-wrapped prologue
// (docs/patterns/shrink-wrapped-callee-save-push.md): retail defers push
// edi/ebx past the null guard (pushing only ecx/ebp/esi upfront), which shifts
// every spill-slot offset (`[esp+0x18]` vs `[esp+0x14]`) downstream. Not source-steerable.
RVA(0x000fd3f0, 0x425)
i32 CBattlezData::Serialize(CSerialArchive* s, i32 op, i32 a2, i32 a3) {
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
            s->Read(&m_scoreValue, 4);
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
    s->Write(&m_scoreValue, 4);
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

SIZE_UNKNOWN(BattlezRecord);
SIZE_UNKNOWN(CBattlezData);
