#include <Gruntz/BattlezData.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Globals.h>

DATA(0x001eab40)
float g_zeroF = 0.0f; // 0x5eab40

RVA(0x000fc9c0, 0x17)
i32 CBattlezData::InitWithRecords(void* records) {
    Init();
    m_records = static_cast<BattlezRecord*>(records);
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
    m_score = 0;
    m_toyzCount = 0;
    m_weaponCount = 0;
    m_1c = 0;
    m_20 = 0;
    m_powerupCount = 0;
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
    m_counts[0] = 0;
    m_counts[1] = 0;
    m_counts[2] = 0;
    m_counts[3] = 0;
    i32 i;
    for (i = 0; i < 88; i++) {
        m_weaponPickupz[i] = 0;
    }
    for (i = 0; i < 40; i++) {
        m_toyPickupz[i] = 0;
    }
    for (i = 0; i < 28; i++) {
        m_powerupPickupz[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        m_miscPickupz[i] = 0;
    }
}

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

RVA(0x000fcb50, 0x2b)
void CBattlezData::MarkFlag(i32 y, i32 x) {
    if (y >= 0 && y <= 4 && x >= 0 && x <= 4) {
        m_flags[x + y * 4] = 1;
    }
}

RVA(0x000fcb90, 0x12)
void CBattlezData::ClearFlags() {
    for (i32 i = 0; i < 16; i++) {
        m_flags[i] = 0;
    }
}

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

RVA(0x000fcc10, 0x2f)
i32 CBattlezData::GetFlag(i32 x, i32 y) {
    if (x >= 0 && x <= 4 && y >= 0 && y <= 4) {
        return *reinterpret_cast<i32*>((reinterpret_cast<char*>(m_flags) + x * 0x10 + y * 4));
    }
    return 0;
}

RVA(0x000fcc50, 0x2a)
void CBattlezData::BumpWin(i32 y, i32 x) {
    if (y >= 0 && y <= 4 && x >= 0 && x <= 4 && y != x) {
        m_wins[x + y * 4]++;
    }
}

RVA(0x000fcc90, 0xf)
void CBattlezData::ClearWins() {
    for (i32 i = 0; i < 16; i++) {
        m_wins[i] = 0;
    }
}

RVA(0x000fccb0, 0x21)
i32 CBattlezData::SumWinRow(i32 y) {
    i32 sum = 0;
    i32* p = reinterpret_cast<i32*>((reinterpret_cast<char*>(m_wins) + y * 0x10));
    for (i32 c = 0; c < 4; c++) {
        sum += *p++;
    }
    return sum;
}

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

RVA(0x000fcd70, 0x61)
i32 CBattlezData::InBounds(i32 unused) {
    if (m_scoreValue == 0) {
        return 0;
    }
    if (m_30 > m_toyzCount) {
        return 0;
    }
    if (m_34 > m_weaponCount) {
        return 0;
    }
    if (m_38 > m_powerupCount) {
        return 0;
    }
    if (m_3c > m_28) {
        return 0;
    }
    return m_40 <= m_2c;
}

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

RVA(0x000fcf20, 0x37)
i32 CBattlezData::SumGroupField0c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_0c;
    }
    return sum;
}

RVA(0x000fcf70, 0x37)
i32 CBattlezData::SumGroupField2c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_2c;
    }
    return sum;
}

RVA(0x000fcfc0, 0x37)
i32 CBattlezData::SumGroupField10() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_10;
    }
    return sum;
}

RVA(0x000fd010, 0x37)
i32 CBattlezData::SumGroupField30() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_30;
    }
    return sum;
}

RVA(0x000fd060, 0x37)
i32 CBattlezData::SumGroupField1c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_1c;
    }
    return sum;
}

RVA(0x000fd0b0, 0x37)
i32 CBattlezData::SumGroupField34() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_34;
    }
    return sum;
}

RVA(0x000fd100, 0x37)
i32 CBattlezData::SumGroupField20() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_20;
    }
    return sum;
}

RVA(0x000fd150, 0x37)
i32 CBattlezData::SumGroupField38() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_38;
    }
    return sum;
}

RVA(0x000fd1a0, 0x37)
i32 CBattlezData::SumGroupField24() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_24;
    }
    return sum;
}

RVA(0x000fd1f0, 0x37)
i32 CBattlezData::SumGroupField3c() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_3c;
    }
    return sum;
}

RVA(0x000fd240, 0x37)
i32 CBattlezData::SumGroupField18() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_18;
    }
    return sum;
}

RVA(0x000fd290, 0x37)
i32 CBattlezData::SumGroupField14() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_14;
    }
    return sum;
}

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

RVA(0x000fd330, 0x84)
void CBattlezData::FillRecord(i32 index, i32 phase) {
    i32* rec = reinterpret_cast<i32*>((reinterpret_cast<char*>(m_records) + index * 0x40 - 0x40));
    if (phase == 0) {
        rec[0] = 1;
        rec[2] = m_score;
        rec[3] = m_toyzCount;
        rec[4] = m_weaponCount;
        rec[5] = m_1c;
        rec[6] = m_20;
        rec[7] = m_powerupCount;
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
// (m_count..m_scoreValue) are streamed UNROLLED; the m_counts band and the four nested 4xN
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
            s->Read(&m_score, 4);
            s->Read(&m_toyzCount, 4);
            s->Read(&m_weaponCount, 4);
            s->Read(&m_1c, 4);
            s->Read(&m_20, 4);
            s->Read(&m_powerupCount, 4);
            s->Read(&m_28, 4);
            s->Read(&m_2c, 4);
            s->Read(&m_30, 4);
            s->Read(&m_34, 4);
            s->Read(&m_38, 4);
            s->Read(&m_3c, 4);
            s->Read(&m_40, 4);
            s->Read(&m_scoreValue, 4);
            for (p = m_counts, i = 0; i < 4; i++, p++) {
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
            p = m_weaponPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 22; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_toyPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 10; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_powerupPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 7; c++, p++) {
                    s->Read(p, 4);
                }
            }
            p = m_miscPickupz;
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
    s->Write(&m_score, 4);
    s->Write(&m_toyzCount, 4);
    s->Write(&m_weaponCount, 4);
    s->Write(&m_1c, 4);
    s->Write(&m_20, 4);
    s->Write(&m_powerupCount, 4);
    s->Write(&m_28, 4);
    s->Write(&m_2c, 4);
    s->Write(&m_30, 4);
    s->Write(&m_34, 4);
    s->Write(&m_38, 4);
    s->Write(&m_3c, 4);
    s->Write(&m_40, 4);
    s->Write(&m_scoreValue, 4);
    for (p = m_counts, i = 0; i < 4; i++, p++) {
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
    p = m_weaponPickupz;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 22; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_toyPickupz;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 10; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_powerupPickupz;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 7; c++, p++) {
            s->Write(p, 4);
        }
    }
    p = m_miscPickupz;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++, p++) {
            s->Write(p, 4);
        }
    }
    return 1;
}

