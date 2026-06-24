#include <rva.h>
#include <string.h> // inlined memset / strcpy in CTimer::Serialize (rep stos / rep movs)
// SpriteLoaders.cpp - two sibling HUD/UI sprite loaders that pull a named sprite
// out of the engine's string-keyed sprite-set hash table and cache individual
// animation frames off it (C:\Proj\Gruntz). Both share the same idiom:
//   1. look the sprite up by class-name string through the matched sprite-set
//      hash table (CSpriteHashTable::Lookup, external);
//   2. for each wanted frame number N, extract the frame pointer from the
//      sprite's frame table ONLY when N lies inside the sprite's valid frame
//      range [m_firstFrame(+0x64) .. m_lastFrame(+0x68)], else cache 0.
//
//   LoadTimerSprite      - the in-game TIMER
//       sprite ("GAME_TIMER"); looked up through the game-manager singleton
//       g_gameReg (->+0x30 ->+0x10). Caches frames 10/11 and bails to
//       0 if any required frame is missing.
//   LoadLoadingBarSprite - the loading-screen
//       progress-bar sprite ("GAME_LOADINGBAR"); looked up through this->m_c
//       (->+0x10). Caches frames 1/2/3 and a loaded flag.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT bodies modulo one MSVC5 scheduling coin-flip: the target SINKS the
// lookup out-param's zero-init store (`mov [&spr],0`) to just after the two arg
// pushes; our cl HOISTS it before the lea - the same 2-3 instructions, permuted
// (byte-content identical). See config/units.toml. Kept wip, not strict-exact.

// ---------------------------------------------------------------------------
// The engine string-keyed sprite-set hash table. Lookup() hashes the class-name
// key, finds the entry, and writes the found sprite pointer through *ppOut
// (returning a found-flag). Modeled minimally so the `ecx=<map>; call <helper>`
// shape reloc-masks against the matched lookup helper.
// ---------------------------------------------------------------------------
struct CSprite;
class CSpriteHashTable {
public:
    int Lookup(char* szName, CSprite** ppOut);
};

// The engine sprite (animation) object. Only the load-bearing members are
// reconstructed: a frame-pointer table at +0x14 and the inclusive valid frame
// range [m_64 .. m_68].
struct CSprite {
    char m_pad00[0x14];
    int** m_14; // +0x14  frame-pointer table
    char m_pad18[0x64 - 0x18];
    int m_64; // +0x64  first valid frame number
    int m_68; // +0x68  last valid frame number
};

// The sprite manager: its name->sprite hash table is embedded at +0x10 (the
// `add ecx,0x10` before the lookup call addresses it).
struct CSpriteMgr {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10  the name->sprite hash table
};

// The world/key lookup table the Tick expiry path probes (FindByKey, RVA
// 0x1b8760 / "Lookup", external). Reached at resmgr->m_8 + 0x48.
struct CKeyTable {
    int FindByKey(int key, int* outFound);
};

// The intermediate resource object both loaders reach the sprite manager through:
// its +0x10 slot points at the CSpriteMgr; +0x8 holds the world key table.
struct CResMgr {
    char m_pad00[0x8];
    CKeyTable* m_8; // +0x8
    char m_padc[0x10 - 0xc];
    CSpriteMgr* m_10; // +0x10
};

// The per-level state object hung off g_gameReg->m_2c: the expiry path stamps a
// "time up" command (m_4f4/m_400/m_404) and the clock snapshot (m_3f8/m_3fc).
struct CLevelState {
    char m_pad00[0x3f8];
    int m_3f8; // +0x3f8
    int m_3fc; // +0x3fc
    int m_400; // +0x400
    int m_404; // +0x404
    char m_pad408[0x4f4 - 0x408];
    int m_4f4; // +0x4f4
};

// A notify target reached off g_gameReg->m_68 (Notify, RVA 0x7a510, external).
struct CLevelNotify {
    void Notify(int idx);
};

// One per-player timer slot in the g_gameReg->m_150 array (stride 0x238 bytes);
// the expiry path sets m_24 = 1.
struct CTimerSlot {
    char m_pad00[0x24];
    int m_24; // +0x24
};

// The loading bar reaches the resource object through this->m_c.
// The game-manager singleton (g_gameReg) reaches it through g_gameReg->m_30.
// The expiry path also touches m_2c (level state), m_68 (notify), a per-player
// timer-slot array at +0x150 (stride 0x238) and m_15c (a sub-object whose first
// slot overlaps the array, so it is read via the raw offset).
struct CGameReg {
    char m_pad00[0x2c];
    CLevelState* m_2c; // +0x2c
    CResMgr* m_30;     // +0x30
    char m_pad34[0x68 - 0x34];
    CLevelNotify* m_68; // +0x68
    char m_pad6c[0x150 - 0x6c];
    char m_150[0x238]; // +0x150  base of the per-player timer-slot array (stride 0x238)
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// CLoadingBar::LoadLoadingBarSprite
// ---------------------------------------------------------------------------
class CLoadingBar {
public:
    int LoadLoadingBarSprite();

    char m_pad00[0xc];
    CResMgr* m_c; // +0xc
    char m_pad10[0x4bc - 0x10];
    int m_4bc;  // +0x4bc loaded flag (set to 1)
    int* m_4c0; // +0x4c0 frame 2
    int* m_4c4; // +0x4c4 frame 3
    int* m_4c8; // +0x4c8 frame 1
};

RVA(0x000d7440, 0xad)
int CLoadingBar::LoadLoadingBarSprite() {
    CSprite* spr = 0;
    m_c->m_10->m_10map.Lookup("GAME_LOADINGBAR", &spr);
    if (!spr) {
        return 0;
    }

    m_4c8 = (spr->m_64 <= 1 && spr->m_68 >= 1) ? spr->m_14[1] : 0;
    m_4c0 = (spr->m_64 <= 2 && spr->m_68 >= 2) ? spr->m_14[2] : 0;
    m_4c4 = (spr->m_64 <= 3 && spr->m_68 >= 3) ? spr->m_14[3] : 0;
    m_4bc = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer - the on-screen game timer (C:\Proj\Gruntz). Holds the looked-up
// digit/colon sprite frames (m_10..m_20), the 64-bit base (m_28/m_2c) and
// accumulated (m_30/m_34) clock times, a running flag (m_48) and the decoded
// current value (m_4c). Methods below are in retail-RVA order.
// ---------------------------------------------------------------------------

// The drawable timer-frame object (one cached animation frame). Its draw entry
// (RenderFrame, RVA 0x153790, external/__thiscall) blits the frame at a screen
// position. Modeled with no body so its call reloc-masks.
struct CTimerFrame {
    void RenderFrame(int pSurf, int x, int y, int z);
};

// The archive/order object passed to HandleEvent + Serialize. The field-transfer
// entries are the virtuals at vtable byte-offsets 0x2c (Read) and 0x30 (Write),
// each taking a buffer ptr + a byte count. Modeled polymorphic (slot decls only,
// never defined -> no ??_7 here) so `ar->Write(buf,n)` lowers to the exact
// `mov eax,[ar]; push n; push buf; mov ecx,ar; call [eax+0x30]` __thiscall dispatch.
struct CTimerArchive {
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
    virtual void Read(void* buf, int n);  // +0x2c
    virtual void Write(void* buf, int n); // +0x30
};

class CTimer {
public:
    CTimer* Init();
    int LoadTimerSprite(int a, int b);
    void Reset();
    int Tick(int dt);
    int Draw(int x, int pSurf);
    void AddTime(int seconds, int minutes);
    int HandleEvent(CTimerArchive* ar, int kind, int a3, int a4);
    int Serialize(CTimerArchive* ar);   // 0x9c2e0 (this cluster)
    int Deserialize(CTimerArchive* ar); // 0x9c650 (external, declared-not-defined)

    int* m_0;  // +0x00 base x
    int* m_4;  // +0x04 base y
    int* m_8;  // +0x08 the looked-up sprite
    int m_c;   // +0x0c visible/active flag
    int* m_10; // +0x10 digit-frame 10s-min
    int* m_14; // +0x14 digit-frame 1s-min
    int* m_18; // +0x18 colon-frame
    int* m_1c; // +0x1c digit-frame 10s-sec
    int* m_20; // +0x20 digit-frame 1s-sec
    char m_pad24[0x28 - 0x24];
    int m_28; // +0x28 base time lo
    int m_2c; // +0x2c base time hi
    int m_30; // +0x30 accumulated lo
    int m_34; // +0x34 accumulated hi
    int m_38; // +0x38
    int m_3c; // +0x3c
    int m_40; // +0x40
    int m_44; // +0x44
    int m_48; // +0x48 running flag
    int m_4c; // +0x4c decoded current value (ms within hour)
};

// ---------------------------------------------------------------------------
// CTimer::Init (0x9bab0) - zero the live state. eax=this idiom from cl.
// ---------------------------------------------------------------------------
RVA(0x0009bab0, 0x35)
CTimer* CTimer::Init() {
    m_28 = 0;
    m_30 = 0;
    m_2c = 0;
    m_34 = 0;
    m_38 = 0;
    m_40 = 0;
    m_3c = 0;
    m_44 = 0;
    m_8 = 0;
    m_10 = 0;
    m_14 = 0;
    m_20 = 0;
    m_18 = 0;
    m_1c = 0;
    m_c = 0;
    m_48 = 0;
    return this;
}

// ---------------------------------------------------------------------------
// CTimer::LoadTimerSprite (0x9bb00)
// ---------------------------------------------------------------------------
RVA(0x0009bb00, 0x119)
int CTimer::LoadTimerSprite(int a, int b) {
    CSprite* spr = 0;
    g_gameReg->m_30->m_10->m_10map.Lookup("GAME_TIMER", &spr);
    m_8 = (int*)spr;
    if (!spr) {
        return 0;
    }

    m_10 = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_10) {
        return 0;
    }
    m_14 = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_14) {
        return 0;
    }
    m_20 = (spr->m_64 <= 11 && spr->m_68 >= 11) ? spr->m_14[11] : 0;
    if (!m_20) {
        return 0;
    }
    m_18 = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_18) {
        return 0;
    }
    m_1c = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_1c) {
        return 0;
    }

    m_0 = (int*)a; /* the two args captured at the tail */
    m_4 = (int*)b;
    m_c = 1;
    m_48 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::Reset (0x9bc70) - zero the sprite-frame cache + active flag.
// ---------------------------------------------------------------------------
RVA(0x0009bc70, 0x18)
void CTimer::Reset() {
    m_8 = 0;
    m_10 = 0;
    m_14 = 0;
    m_20 = 0;
    m_18 = 0;
    m_1c = 0;
    m_c = 0;
}

// The clock + frame-gate globals the Tick/Draw paths read (external delinked
// DATA, reloc-masked). g_645588 is the running game clock; g_644c54 a level
// base index; g_6455a0 a draw-throttle frame counter; g_645588 the start clock.
extern "C" {
    extern unsigned int g_645588;
}
extern int g_644c54;
DATA(0x006455a0)
extern unsigned int g_6455a0;

// The grunt the expiry / under-attack notify fires target (external, reloc-masked).
// ResolveDeathAnimation = "time up"; NotifyFortUnderAttack = "<60s remaining".
struct CGruntRef {
    void ResolveDeathAnimation(); // 0x455f0 (via thunk 0x3a1c)
    void NotifyFortUnderAttack(); // 0x45270 (via thunk 0x3201)
};

// The object FindByKey returns (or the raw m_15c when not found): its m_7c->m_18
// holds the grunt the notify fires on, gated non-null.
struct CTimerNotifyObj {
    char m_pad00[0x7c];
    struct Inner {
        char m_pad00[0x18];
        CGruntRef* m_18; // +0x18
    }* m_7c;             // +0x7c
};

// ---------------------------------------------------------------------------
// CTimer::Tick (0x9bca0) - recompute the remaining time from the running clock,
// clamp at 0, and on expiry stamp the level "time up" command + fire the death
// notify; under 60s fire the "fort under attack" notify. Then decode the clamped
// value into the four MM:SS digit frames (with leading-zero blanking -> frame 10)
// via the constant-divisor magic-division chain.
// ---------------------------------------------------------------------------
// @early-stop
// big function (605 B): the magic-division decode chain + the g_gameReg notify
// dispatch through the FindByKey helper; reconstruction is logically complete
// but the leading-zero-guard branch scheduling + 64-bit clamp regalloc plateau.
// Deferred to the final sweep / a leaf-first redo.
RVA(0x0009bca0, 0x25d)
int CTimer::Tick(int dt) {
    if (!m_48) {
        return 1;
    }
    // remaining = (m_30:m_34) - g_645588 + (m_28:m_2c), clamped at 0.
    __int64 rem = *(__int64*)&m_30 - (unsigned)g_645588 + *(__int64*)&m_28;
    int v = (rem > 0) ? (int)rem : 0;
    m_4c = v;

    if (v == 0) {
        // expired: clear, stamp the level "time up" command + clock snapshot.
        m_40 = 0;
        m_44 = 0;
        m_30 = 0;
        m_34 = 0;
        m_48 = 0;
        m_4c = 0;
        CLevelState* ls = g_gameReg->m_2c;
        ls->m_4f4 = 1;
        ls->m_400 = 0x1f4;
        ls->m_404 = 0;
        ls->m_3f8 = g_645588;
        ls->m_3fc = 0;
        g_gameReg->m_68->Notify(g_644c54);
        CTimerSlot* slot = (CTimerSlot*)((char*)&g_gameReg->m_150 + g_644c54 * 0x238);
        if (slot != 0) {
            slot->m_24 = 1;
        }
        int* key = *(int**)((char*)g_gameReg + 0x15c);
        if (key != 0) {
            int found = 0;
            CTimerNotifyObj* obj =
                (CTimerNotifyObj*)g_gameReg->m_30->m_8->FindByKey((int)key, &found);
            CTimerNotifyObj* hit = found ? obj : (CTimerNotifyObj*)key;
            if (hit != 0 && hit->m_7c->m_18 != 0) {
                hit->m_7c->m_18->ResolveDeathAnimation();
            }
        }
        return 1;
    }

    if ((unsigned)v < 0xea60) {
        int* key = *(int**)((char*)g_gameReg + 0x15c);
        if (key != 0) {
            int found = 0;
            CTimerNotifyObj* obj =
                (CTimerNotifyObj*)g_gameReg->m_30->m_8->FindByKey((int)key, &found);
            CTimerNotifyObj* hit = found ? obj : (CTimerNotifyObj*)key;
            if (hit != 0 && hit->m_7c->m_18 != 0) {
                hit->m_7c->m_18->NotifyFortUnderAttack();
            }
        }
    }

    // decode v (ms) into the MM:SS digits; a 0 digit with no significant digit to
    // its left becomes 10 (the blank frame).
    unsigned int t = (unsigned)v;
    int d10min = t / 600000;
    int d1min = t / 60000 % 10;
    if (d1min == 0 && d10min != 0) {
        d1min = 10;
    }
    unsigned int r = t % 60000;
    int d10sec = r / 10000;
    if (d10sec == 0 && (d10min != 0 || d1min != 0)) {
        d10sec = 10;
    }
    int d1sec = r / 1000 % 10;
    if (d1sec == 0 && d10min == 0 && d1min == 0 && d10sec == 0) {
        d1sec = 10;
    }

    CSprite* spr = (CSprite*)m_8;
    m_10 = (spr->m_64 <= d10min && d10min <= spr->m_68) ? spr->m_14[d10min] : 0;
    m_14 = (spr->m_64 <= d1min && d1min <= spr->m_68) ? spr->m_14[d1min] : 0;
    m_18 = (spr->m_64 <= d10sec && d10sec <= spr->m_68) ? spr->m_14[d10sec] : 0;
    m_1c = (spr->m_64 <= d1sec && d1sec <= spr->m_68) ? spr->m_14[d1sec] : 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::Draw (0x9bfa0) - blit the (up to) five cached digit/colon frames at
// the base position, fanned out around the centre by fixed pixel offsets.
// ---------------------------------------------------------------------------
RVA(0x0009bfa0, 0xb4)
int CTimer::Draw(int pSurf, int force) {
    if (!m_48) {
        return 1;
    }
    if (force == 0 && (unsigned)m_4c < 0x2710 && (unsigned)g_6455a0 >= 0xfa) {
        return 1;
    }
    if (m_10) {
        ((CTimerFrame*)m_10)->RenderFrame(pSurf, (int)m_0 - 0x22, (int)m_4, 0);
    }
    if (m_14) {
        ((CTimerFrame*)m_14)->RenderFrame(pSurf, (int)m_0 - 0x10, (int)m_4, 0);
    }
    if (m_20) {
        ((CTimerFrame*)m_20)->RenderFrame(pSurf, (int)m_0, (int)m_4, 0);
    }
    if (m_18) {
        ((CTimerFrame*)m_18)->RenderFrame(pSurf, (int)m_0 + 0x10, (int)m_4, 0);
    }
    if (m_1c) {
        ((CTimerFrame*)m_1c)->RenderFrame(pSurf, (int)m_0 + 0x22, (int)m_4, 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::AddTime (0x9c0e0) - add (seconds, minutes) to the accumulated 64-bit
// clock, each clamped (sec<=0x63, min<=0x3b) with a carry-adjust on overflow.
// ---------------------------------------------------------------------------
RVA(0x0009c0e0, 0xa3)
void CTimer::AddTime(int seconds, int minutes) {
    if (!m_48) {
        return;
    }
    unsigned int mins = (unsigned)minutes;
    if (mins > 0x3b) {
        mins = 0x3b;
    }
    unsigned int secs = (unsigned)seconds;
    if (secs > 0x63) {
        secs = 0x63;
    }
    unsigned int cur = (unsigned)m_4c;
    // carry = 1 unless (the minute already on the clock + new minutes) fits in 0x3b.
    unsigned int carry = 1;
    if (cur % 60000 / 1000 + mins <= 0x3b) {
        carry = 0;
    }
    // clamp seconds against the second already on the clock (cur / 60000).
    if (cur / 60000 + secs > 0x63) {
        secs = 0x63 - cur / 60000 - carry;
    }
    unsigned int total = (mins + secs * 60) * 1000;
    *(unsigned __int64*)&m_30 += total;
}

// ---------------------------------------------------------------------------
// CTimer::HandleEvent (0x9c1c0) - load (kind==4) / save (kind==7) the timer
// through the archive: dispatch the whole-object (de)serializer (kind 4 ->
// Deserialize 0x9c650, kind 7 -> Serialize 0x9c2e0) then stream the two 64-bit
// clock pairs (m_28/m_30 and m_38/m_40) field by field via the archive's
// Read(+0x2c)/Write(+0x30) virtuals.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~88%): retail pins `this`->ebp and `kind`->ebx; our cl swaps
// them (this->ebx, kind->ebp), a register rename that cascades through every
// per-block `cmp kind` + `lea edi,[this+N]`. Logic (call mapping, slot mapping,
// pointer-advance, ret 0x10) exact; see docs/patterns/zero-register-pinning.md.
RVA(0x0009c1c0, 0xdb)
int CTimer::HandleEvent(CTimerArchive* ar, int kind, int a3, int a4) {
    if (ar == 0) {
        return 0;
    }
    if (kind == 4) {
        int r = Deserialize(ar);
        if (!r) {
            return r;
        }
    } else if (kind == 7) {
        int r = Serialize(ar);
        if (!r) {
            return r;
        }
    }

    int* p = &m_28;
    if (kind == 4) {
        ar->Read(p, 8);
        p += 2;
        ar->Read(p, 8);
    } else if (kind == 7) {
        ar->Write(p, 8);
        p += 2;
        ar->Write(p, 8);
    }

    p = &m_38;
    if (kind == 4) {
        ar->Read(p, 8);
        p += 2;
        ar->Read(p, 8);
        return 1;
    } else if (kind == 7) {
        ar->Write(p, 8);
        p += 2;
        ar->Write(p, 8);
    }
    return 1;
}

// Per-serialize round counter the CString archive helpers bump (g_serialCount,
// = DAT_00629ad0). The frame-name reverse-lookup helper (0x155630) lives on the
// sprite registry (g_gameReg->m_30->m_10); modeled with NO body -> reloc-masks.
DATA(0x00629ad0)
extern int g_serialCount;
struct CStrReader {
    void ReadField(int dst, char* tmp, int* outZero);
};

// ---------------------------------------------------------------------------
// CTimer::Serialize (0x9c2e0) - write the timer through the save archive: the
// position/flags, the sprite-set name, then each digit/colon frame by its
// registry name + a found flag, then the two clock pairs. Mirrors the shared
// CActionOptionsMenuBar::Serialize idiom (vtable Transfer + inlined memset/strcpy
// + per-frame name reverse-lookup).
// ---------------------------------------------------------------------------
// @early-stop
// stack-packing wall (~96%, same family as CActionOptionsMenuBar::Serialize):
// retail reuses the dead mgr spill slot for the per-block `zero` int; our cl
// gives `zero` its own slot, shifting every frame-size immediate by 4. Body
// (vtable Transfer dispatch + inlined memset/strcpy + name lookup) exact.
RVA(0x0009c2e0, 0x2b6)
int CTimer::Serialize(CTimerArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CResMgr* mgr = g_gameReg->m_30;
    if (mgr == 0) {
        return 0;
    }

    ar->Write(&m_0, 4);
    ar->Write(&m_4, 4);

    char tmp[0x80];

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    if (m_8) {
        strcpy(tmp, (char*)m_8 + 0x24);
    }
    ar->Write(tmp, 0x80);

    ar->Write(&m_c, 4);

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_10) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_10, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_14) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_14, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_18) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_18, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_1c) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_1c, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_20) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_20, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    ar->Write(&m_48, 4);
    ar->Write(&m_4c, 4);
    return 1;
}
