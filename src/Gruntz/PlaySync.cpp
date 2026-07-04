// PlaySync.cpp - CPlay::SyncState (0x0d7520): the play-state serialize/round-trip.
// Bail on a null archive, run the header serialize, then a mode pre-step (mode 4 =
// write, mode 7 = read, mode 8 = re-init the ambient-sound cue). Then round-trip a
// sequence of 64-bit timer blocks through the archive (mode 4 -> Write vtbl[0x30],
// mode 7 -> Read vtbl[0x2c]) interleaved with three child sync sub-objects
// (m_guts / m_frameMarker / m_beginMarker). The mode-8 ambient path reuses the
// BeginGridWalk sprite-lookup idiom (world config array + sprite-factory retry).
//
// The archive Read/Write, the header/pre-step serializes and the child syncs are
// external (reloc-masked). Field names are placeholders; only offsets + emitted
// bytes are load-bearing (campaign doctrine).
#include <Dsndmgr/GruntzSoundZ.h> // CWorld::m_48 zoned sound bank (PlayByName @0x138840)
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h> // shared CSerialArchive stream (Read @ +0x2c, Write @ +0x30)

// Round-trip a 16-byte (two 4-int halves) timer block through the archive; `p` is
// an i32* into the block, so `p + 2` is the second 8-byte half. The mode-4 (write)
// body is emitted out-of-line (retail checks mode==4 first via je, keeps the
// mode-7 read body inline).
#define SYNC_PAIR(ar, mode, p)                                                                     \
    if ((mode) != 4) {                                                                             \
        if ((mode) == 7) {                                                                         \
            (ar)->Read((p), 8);                                                                    \
            (ar)->Read((p) + 2, 8);                                                                \
        }                                                                                          \
    } else {                                                                                       \
        (ar)->Write((p), 8);                                                                       \
        (ar)->Write((p) + 2, 8);                                                                   \
    }

// ===========================================================================
// CPlay::SyncState  (0x0d7520)
// ===========================================================================
RVA(0x000d7520, 0x3b9)
i32 CPlay::SyncState(CSerialArchive* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    if (!HeaderSerialize(ar, mode, a2, a3)) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!SyncWrite19fb(ar)) {
                return 0;
            }
            break;
        case 7:
            if (!SyncRead2f7c(ar)) {
                return 0;
            }
            break;
        case 8: {
            if (m_gridHasSprite) {
                CWorld* w = m_4w();
                i32 id = g_644c54;
                void* spr = w->m_74->LoadSprite(*(void**)(w->m_158 + (id * 0x47) * 8), 0);
                if (spr == 0) {
                    spr = ((CWorld::SpriteLoader*)g_64556c->m_74)->LoadSprite((void*)1, (i32)spr);
                }
                m_grid->SetDelay(0xa);
                m_grid->SetSprite(spr);
            }
            char buf[0x40];
            wsprintfA(buf, "AMBIENT%d", GetAmbientId());
            if (g_64556c->m_14) {
                m_4w()->m_48->PlayByName(buf, 1);
            }
            m_ambientInitDone = 1;
            break;
        }
    }

    i32* p;
    p = &m_syncTimerLo;
    SYNC_PAIR(ar, mode, p);
    if (!m_guts->Sync(ar, mode, a2, a3)) {
        return 0;
    }
    if (!m_frameMarker->Sync(ar, mode, a2, a3)) {
        return 0;
    }
    p = &m_cueTimerLo;
    SYNC_PAIR(ar, mode, p);
    if (!m_beginMarker->Sync(ar, mode, a2, a3)) {
        return 0;
    }
    p = &m_region0TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region1TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_snapBaseLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region2TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region3TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_bootyTimerLo;
    SYNC_PAIR(ar, mode, p);
    return 1;
}
