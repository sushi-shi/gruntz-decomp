// RandomAmbientSound.cpp - the RTTI-named CRandomAmbientSound (vtable 0x5e713c,
// chain CRandomAmbientSound : CAmbientSound : CUserBase, sizeof 0x58). A
// random-interval ambient sound bound to one or two visibility boxes: Setup seeds
// the mgr handle / play params / boxes; Step (vtable slot 3) ticks the listener
// position against the boxes, drains the rolled countdown by the frame delta, and
// when it expires rerolls a fresh interval (global LCG rand) and (re)plays via
// Update; Update starts/stops the DirectSoundMgr voice.
//
// Real polymorphic: the ctor auto-stamps ??_7CRandomAmbientSound (this class's own
// vtable 0x5e713c); no manual vptr store (see the header note).
//
// Field names are placeholders; OFFSETS + emitted code bytes are load-bearing.
#include <Win32.h>                // RECT / CopyRect / SetRect (PosSoundObj rects + CommitSpriteAction)
#include <Gruntz/RandomAmbientSound.h>
#include <Gruntz/InputState.h> // CInput54 (g_gameReg->m_inputState @+0x54) + CObListSub (its +0x08 CObList)
#include <rva.h>
#include <Globals.h>

#include <math.h> // sqrt intrinsic (UpdateAt's positional falloff) - inline fsqrt

// ---------------------------------------------------------------------------
// The free `Spawn`/`Stop` ambient-sound pair (0x00c9d0 / 0x00ca00, __cdecl). They
// drive the ambient voice that hangs off a CGameObject's +0x7c aux: aux->m_requestState is
// the request state (0 = "spawn", 0x1e = "stop"), aux->m_voice the live voice.
// ---------------------------------------------------------------------------
// The sound voice object the aux points at (+0x168): its CObject vptr (slot 0 =
// scalar-deleting dtor), the DirectSoundMgr handle (+0x04), the playing flag
// (+0x14) and the spatial-mgr list node (+0x3c) RemoveAt unlinks.
class PosSoundVoice {
public:
    virtual void* ScalarDtor(i32 flag); // +0x00  slot 0 (scalar-deleting dtor)
    DirectSoundMgr* m_mgr;              // +0x04
    char m_pad8[0x14 - 0x8];
    i32 m_isPlaying; // +0x14  playing flag
    char m_pad18[0x3c - 0x18];
    void* m_spatialNode; // +0x3c  spatial-mgr list node
};
// The aux sub-object (CGameObject+0x7c): the init/action handler, the request state,
// the emit src-clip and the live voice slot.
struct PosSoundAux {
    char m_pad00[0x10];
    void* m_handler; // +0x10  the object's init/action handler (vs the default at 0x402d15)
    char m_pad14[0x1c - 0x14];
    i32 m_requestState; // +0x1c  request state (0 spawn / 0x1e stop / 5 spawned)
    char m_pad20[0x2c - 0x20];
    i32 m_srcL; // +0x2c  emit src clip: left
    i32 m_srcR; // +0x30                   right
    i32 m_srcT; // +0x34                   top
    i32 m_srcB; // +0x38                   bottom
    char m_pad3c[0x168 - 0x3c];
    PosSoundVoice* m_voice; // +0x168  the live voice
};
// The CGameObject the request rides on (only the touched offsets).
struct PosSoundObj {
    char m_pad00[0x08];
    i32 m_flags08; // +0x08  flags
    char m_pad0c[0x40 - 0xc];
    i32 m_flags40; // +0x40  flags
    char m_pad44[0x5c - 0x44];
    i32 m_x; // +0x5c  x
    i32 m_y; // +0x60  y
    char m_pad64[0x7c - 0x64];
    PosSoundAux* m_aux; // +0x7c  the aux
    char m_pad80[0x120 - 0x80];
    i32 m_120; // +0x120
    char m_pad124[0x134 - 0x124];
    i32 m_extentL; // +0x134  per-side emit extents (L/T/R/B)
    i32 m_extentT; // +0x138
    i32 m_extentR; // +0x13c
    i32 m_extentB; // +0x140
    RECT m_area;   // +0x144  emit source area (CopyRect base)
    RECT m_placed; // +0x154  placed rect written back on emit
    char m_pad164[0x19c - 0x164];
    void* m_layer; // +0x19c  layer/desc (its +0x10 feeds the factory)
};
// The spatial-sound voice CObList lives at g_gameReg->m_inputState + 0x08 (the same
// embedded CObList the manager ctors/tears down); RemoveAt unlinks the voice's node.
// Shared shape: CObListSub in <Gruntz/InputState.h>.

// The factory the spawn path calls (Stub_00b960 via the 0x20e5 thunk). It news a
// 0x48-byte voice; modeled __stdcall (callee-cleaned, no `add esp`).
extern "C" void* __stdcall PosSoundSpawn(void* layer, i32 a2, void* outPt, i32 a4, i32 a5);

void SpawnPosSound(PosSoundObj* obj);

// ---------------------------------------------------------------------------
// StopPosSound (0x00c9d0): mark the request "stop" (state 2 in the global queue
// at 0x62990c) and run the spawn/stop driver.
// ---------------------------------------------------------------------------

RVA(0x0000c9d0, 0x18)
void StopPosSound(PosSoundObj* obj) {
    g_posSoundReq = 2;
    SpawnPosSound(obj);
}

// ---------------------------------------------------------------------------
// SpawnPosSound (0x00ca00): per-object placement tick. On a "spawn" request
// (aux->m_requestState == 0) stamp the object flags and, if its layer + the active world
// are live, new a voice through the factory; on a "stop" request (0x1e) tear the
// live voice down (StopAndRewind, unlink from the spatial mgr, scalar-dtor it).
// ---------------------------------------------------------------------------
// @early-stop
// out-param stack struct + virtual scalar-dtor dispatch + factory calling-conv:
// logic complete, but the spawn path passes obj->m_x/m_y through a 2-int stack
// out-param (the `sub esp,8` slots, address-escaped to the factory) whose exact
// [esp+N] schedule and the factory's callee-clean shape are not source-steerable.
// The teardown arm (StopAndRewind / RemoveAt / `call [vptr]`) is byte-exact.
RVA(0x0000ca00, 0xf0)
void SpawnPosSound(PosSoundObj* obj) {
    PosSoundAux* aux = obj->m_aux;
    i32 state = aux->m_requestState;
    if (state != 0) {
        if (state != 0x1e) {
            return;
        }
        PosSoundVoice* sound = aux->m_voice;
        if (sound == 0) {
            return;
        }
        CObListSub* arr = (CObListSub*)((char*)g_gameReg->m_inputState + 8);
        if (sound->m_mgr != 0) {
            sound->m_mgr->StopAndRewind();
            sound->m_isPlaying = 0;
        }
        if (sound->m_spatialNode != 0) {
            arr->RemoveAt(sound->m_spatialNode);
            sound->ScalarDtor(1);
        }
        aux->m_voice = 0;
        aux->m_requestState = 0;
        return;
    }

    obj->m_flags08 = (obj->m_flags08 & ~2) | 0x100001;
    obj->m_flags40 |= 1;
    aux->m_voice = 0;
    void* layer = obj->m_layer;
    if (layer != 0 && g_gameReg != 0 && g_gameReg->m_inputState != 0) {
        i32 pt[2];
        pt[0] = obj->m_x;
        pt[1] = obj->m_y;
        void* v = PosSoundSpawn(*(void**)((char*)layer + 0x10), 0x64, &pt, obj->m_120, 0);
        if (v != 0) {
            aux->m_voice = (PosSoundVoice*)v;
        }
    }
    aux->m_requestState = 5;
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Base ctor (0x00bb40): cl auto-stamps the vptr, then clear the mgr handle (+0x04)
// and +0x3c. The rest is set up by Setup.
// ---------------------------------------------------------------------------
RVA(0x0000bb40, 0xf)
CRandomAmbientSound::CRandomAmbientSound() {
    m_mgr = 0;
    m_3c = 0;
}

// ---------------------------------------------------------------------------
// Setup (0x00be50, __thiscall, 5 args): refuse a null mgr; otherwise stash the
// mgr + the three play params, copy the primary box (or stamp the no-box
// sentinel), and reset the secondary box to the sentinel. Returns 1, or 0 on the
// null guard.
// ---------------------------------------------------------------------------
RVA(0x0000be50, 0x8f)
i32 CRandomAmbientSound::Setup(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientBox* box, i32 a5) {
    if (mgr == 0) {
        return 0;
    }
    m_mgr = mgr;
    m_lastPosition = a2;
    m_scaleA = a3;
    m_scaleB = a5;
    m_panIndex = 0;
    m_isPlaying = 0;
    AmbientBox* p = &m_box1;
    if (box != 0) {
        *p = *box;
    } else {
        p->left = (i32)0x80000000;
    }
    if (p->left == 0 && m_box1.top == 0 && m_box1.right == 0 && m_box1.bottom == 0) {
        p->left = (i32)0x80000000;
    }
    m_box2.left = (i32)0x80000000;
    return 1;
}

// ---------------------------------------------------------------------------
// Update (0x00c2a0, __thiscall, 3 args playFlag/pos/kind): the play/stop driver.
// Gated on the mgr handle, the playing flag, and the active level
// (g_gameReg->m_soundEnabled and g_gameReg->m_inputState->m_armed). On play it reseeds
// the voice (mgr->ApplyAndPlay(1,m_panIndex,0,1)), scales pos by (m_scaleA clamped)/100 then
// m_scaleB/100 (both signed magic-/100), clamps the
// result to [0,100], and dispatches SetVolumeByIndex (kind==0) or CloneAndPlay
// (kind!=0); on stop it StopAndRewind's (kind==0) or CloneAndPlay-stops (kind!=0).
// ---------------------------------------------------------------------------
// @early-stop
// 3-push frame + twin signed-/100 magic-division scheduling wall (logic complete,
// all relocs paired). cl duplicates the scale-and-clamp block per kind branch (as
// retail does) but permutes the eax/ecx/edx use across the two 0x51eb851f reductions
// and the SetVolumeByIndex/CloneAndPlay tail; no source spelling pins that schedule.
// See zero-register-pinning.md and CGruntSpawnConfig::PickWeighted (the /100 family).
RVA(0x0000c2a0, 0x19e)
void CRandomAmbientSound::Update(i32 playFlag, i32 pos, i32 kind) {
    if (m_mgr == 0) {
        return;
    }
    if (playFlag == 0) {
        // Stop path.
        if (m_isPlaying == 0) {
            return;
        }
        if (kind != 0) {
            m_lastPosition = 0;
            m_mgr->CloneAndPlay(0, kind, 1);
            m_isPlaying = 0;
            return;
        }
        m_mgr->StopAndRewind();
        m_isPlaying = 0;
        return;
    }
    if (m_isPlaying != 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_armed == 0) {
        return;
    }

    if (kind != 0) {
        m_mgr->ApplyAndPlay(1, m_panIndex, 0, 1);
        i32 t = m_scaleA;
        m_lastPosition = pos;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * pos) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_mgr->CloneAndPlay(v, kind, 0);
        m_lastPosition = pos;
        m_isPlaying = 1;
        return;
    }

    m_mgr->ApplyAndPlay(1, m_panIndex, 0, 1);
    i32 t = m_scaleA;
    m_lastPosition = pos;
    if (t > 5) {
        t -= 0xf;
    }
    i32 v = (t * pos) / 100;
    if (m_scaleB > 0) {
        v = (v * m_scaleB) / 100;
    }
    if (v < 0) {
        m_mgr->SetVolumeByIndex(0);
        m_lastPosition = pos;
        m_isPlaying = 1;
        return;
    }
    if (v > 0x64) {
        v = 0x64;
    }
    m_mgr->SetVolumeByIndex(v);
    m_lastPosition = pos;
    m_isPlaying = 1;
}

// ---------------------------------------------------------------------------
// SetupFromMap (0x00c4b0, __thiscall, 5 args): resolve the mgr record for `key`
// out of holder->m_map (a CMapPtrToPtr); when found, seed this object via
// SetupPos(record->m_mgr, a3, a4, pos, a5). No-op when the key is absent.
// ---------------------------------------------------------------------------
// @early-stop
// CODE BYTE-EXACT - residual is the reloc-naming scoring artifact: retail's two
// calls go through ILT thunks (Lookup / thunk_FUN_0040c530) while our base names
// the callee directly, so objdiff scores the REL32 operands against
// differently-named symbols (~88.75%). Every instruction byte matches the
// delinked target (verified by base-vs-target objdump). Effectively matched.
RVA(0x0000c4b0, 0x53)
void CRandomAmbientSound::SetupFromMap(
    AmbSoundMapHolder* holder,
    void* key,
    i32 a3,
    i32 a4,
    AmbientPoint* pos,
    i32 a5
) {
    void* found = 0;
    holder->m_map.Lookup(key, &found);
    if (found != 0) {
        SetupPos(((AmbSoundRecord*)found)->m_mgr, a3, a4, pos, a5);
    }
}

// ---------------------------------------------------------------------------
// SetupPos (0x00c530, __thiscall, 5 args): the positional Setup. Refuse a null
// mgr or null position; otherwise stash the mgr + the two play params + a5,
// clear the playing flag (+0x14) and m_panIndex, and copy the (x,y) anchor into
// m_40/m_44. Returns 1, or 0 on either null guard.
// ---------------------------------------------------------------------------
RVA(0x0000c530, 0x51)
i32 CRandomAmbientSound::SetupPos(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5) {
    if (mgr == 0) {
        return 0;
    }
    if (pos == 0) {
        return 0;
    }
    m_mgr = mgr;
    m_lastPosition = a2;
    m_scaleA = a3;
    m_panIndex = 0;
    m_scaleB = a5;
    m_isPlaying = 0;
    m_40 = pos->x;
    m_44 = pos->y;
    return 1;
}

// ---------------------------------------------------------------------------
// UpdateAt (0x00c5b0, __thiscall, 3 args x/y/force): the positional play driver.
// Compute the listener->anchor distance (|m_40-x|, |m_44-y|); if either axis is
// past 0x280 stop the voice. Otherwise derive a falloff volume (100 - dist/3,
// clamped) and a pan (dx/4, clamped, signed by which side of m_40 the listener
// is), scale the volume by m_scaleA/100 then m_scaleB/100, set volume + pan; and when not
// already playing (and the active level is live) reseed and re-set the volume,
// marking the voice playing.
// ---------------------------------------------------------------------------
// @early-stop
// twin signed-/100 magic-division scheduling wall (logic complete, all relocs
// paired) - the SAME family as the sibling Update (0x00c2a0): cl duplicates the
// scale-and-clamp block (as retail does) but permutes eax/ecx/edx across the two
// 0x51eb851f reductions and the SetVolByIdx/SetPanByIdx tail; no source spelling
// pins that schedule. See zero-register-pinning.md and the /100 family.
RVA(0x0000c5b0, 0x1df)
void CRandomAmbientSound::UpdateAt(i32 x, i32 y, i32 force) {
    i32 ax = m_40 - x;
    i32 dx = ax < 0 ? -ax : ax;
    i32 ay = m_44 - y;
    i32 dy = ay < 0 ? -ay : ay;
    i32 dist2 = dx * dx + dy * dy;
    if (dx > 0x280 || dy > 0x280) {
        if (m_mgr != 0 && m_isPlaying != 0) {
            m_mgr->StopAndRewind();
            m_isPlaying = 0;
        }
        return;
    }

    i32 dist = __ftol(sqrt((double)dist2));
    i32 vol = 0x64 - dist / 3;
    if (vol > 0x64) {
        vol = 0x64;
    } else if (vol < 0) {
        vol = 0;
    }
    i32 pan = dx / 4;
    if (pan > 0x64) {
        pan = 0x64;
    } else if (pan < 0) {
        pan = 0;
    }
    if (m_40 < x) {
        pan = -pan;
    }

    {
        i32 t = m_scaleA;
        m_lastPosition = vol;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * vol) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_mgr->SetVolumeByIndex(v);
    }
    m_panIndex = pan;
    m_mgr->SetPanByIndex(pan);

    if (m_isPlaying != 0) {
        return;
    }
    if (m_mgr == 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_armed == 0) {
        return;
    }
    m_mgr->ApplyAndPlay(1, m_panIndex, 0, 1);
    {
        i32 t = m_scaleA;
        m_lastPosition = vol;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * vol) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_mgr->SetVolumeByIndex(v);
    }
    m_lastPosition = vol;
    m_isPlaying = 1;
}

// ---------------------------------------------------------------------------
// CommitSpriteAction (0x0000c840, __cdecl) - a sibling of SpawnPosSound in the
// positional-sound spawn path (re-homed from src/Stub/ApiCallers.cpp). On a fresh
// spawn request (aux->m_requestState == 0) it stamps the object's placed/spawn flag
// bits, resolves the handler-vs-default flag, and - when the layer and the input mgr
// are live - emits the sound-sprite into the active layer through the world sound-set
// factory (full vs simple by the +0x138 extent), copies the placed rect back, then
// latches the request "spawned" (5). Returns 1.
// @early-stop
// arg-load scheduling wall (~94%): body byte-exact through the flag math and both
// exits; the residual is MSVC's just-in-time vs pre-load interleaving of the factory
// member-arg loads (same push order, same args) + the g_gameReg->m_inputState test
// landing in eax vs retail's ecx. Same instructions, different temp-register rotation.
struct PosSoundPlaced { // the create-helper return record; its placed RECT is at +0x28
    char m_pad0[0x28];
    RECT m_28; // +0x28
};
SIZE_UNKNOWN(PosSoundPlaced);
extern "C" void DefaultActionHandler_2d15(); // LAB_00402d15 (address only)
// The world sound-set factory create calls (CWorldSoundSet::CreateRandom @0xbb60 via
// thunk 0x3c97 / CreateAmbient5 @0xb7b0 via thunk 0x2ad6); modeled __stdcall exactly
// like the sibling PosSoundSpawn - the factory ptr (layer +0x10) leads the arg list.
PosSoundPlaced* __stdcall WorldSoundCreateFull(
    void* factory,
    i32 z,
    RECT* rc,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0xbb60
PosSoundPlaced* __stdcall WorldSoundCreateSimple(void* factory, i32 z, RECT* rc, i32 a, i32 b); // 0xb7b0
RVA(0x0000c840, 0x13d)
i32 CommitSpriteAction(PosSoundObj* obj) {
    PosSoundAux* aux = obj->m_aux;
    if (aux->m_requestState == 0) {
        obj->m_flags08 |= 1;
        obj->m_flags40 |= 1;
        if (aux->m_handler == (void*)DefaultActionHandler_2d15) {
            obj->m_flags08 |= 2;
        } else {
            obj->m_flags08 &= ~2;
        }
        void* layer = obj->m_layer;
        if (layer && g_gameReg) {
            RECT rc;
            CopyRect(&rc, &obj->m_area);
            if (aux->m_srcL > 0 || aux->m_srcR > 0) {
                SetRect(&rc, aux->m_srcL, aux->m_srcT, aux->m_srcR, aux->m_srcB);
            }
            if (g_gameReg->m_inputState) {
                PosSoundPlaced* placed;
                if (obj->m_extentT > 0) {
                    placed = WorldSoundCreateFull(
                        *(void**)((char*)layer + 0x10),
                        0x64,
                        &rc,
                        obj->m_120,
                        obj->m_extentL,
                        obj->m_extentT,
                        obj->m_extentR,
                        obj->m_extentB,
                        0
                    );
                } else {
                    placed =
                        WorldSoundCreateSimple(*(void**)((char*)layer + 0x10), 0x64, &rc, obj->m_120, 0);
                }
                if (placed && obj->m_placed.top > 0) {
                    placed->m_28 = obj->m_placed;
                }
            }
        }
        obj->m_flags08 |= 0x10000;
        aux->m_requestState = 5;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Step (0x00cb30, vtable slot 3, __thiscall, 3 args x/y/force): test the listener
// position against the two visibility boxes; if it left both (and we are playing)
// stop the voice. Otherwise drain the rolled countdown by the frame delta, and on
// expiry flip the roller phase, roll a fresh interval over the active phase's
// [lo,hi], halve+clamp it to <=1000, and (re)play via Update.
// ---------------------------------------------------------------------------
// @early-stop
// rand()-call + idiv-scheduling wall (~70%, logic complete, all relocs paired).
// The two structurally-identical reroll arms (phase A over m_40..m_44, phase B
// over m_48..m_4c) each emit the global rand call twice (span==0 coin-flip vs
// idiv-by-span) and cl interleaves the span test / idiv / the +0x50 store with the
// loop-head box compares; no single source spelling pins that interleave. The box
// in/out test + the countdown drain are byte-exact. See zero-register-pinning.md
// and CGruntSpawnConfig::PickWeighted (the same rand-inline/idiv family).
RVA(0x0000cb30, 0x168)
void CRandomAmbientSound::Step(i32 x, i32 y, i32 force) {
    i32 inBox = 0;
    i32 b1 = m_box1.left;
    if (b1 == (i32)0x80000000) {
        inBox = 1;
    } else if (x <= b1 || x >= m_box1.right || y <= m_box1.top || y >= m_box1.bottom) {
        i32 b2 = m_box2.left;
        if (b2 == (i32)0x80000000 || x <= b2 || x >= m_box2.right || y <= m_box2.top
            || y >= m_box2.bottom) {
            inBox = 1;
        }
    }

    if (inBox == 0) {
        if (m_isPlaying != 0 && m_mgr != 0) {
            Update(0, 0x3e8, 1);
            m_isPlaying = 0;
        }
        m_phase = 0;
        return;
    }

    if (force != 0 && m_phase != 0 && m_isPlaying != 0) {
        return;
    }

    if ((u32)m_countdownMs <= (u32)g_tickDelta) {
        m_countdownMs = 0;
    } else {
        m_countdownMs = m_countdownMs - g_tickDelta;
    }
    if (m_countdownMs != 0) {
        return;
    }

    m_phase ^= 1;
    if (m_phase != 0) {
        i32 lo = m_40;
        i32 hi = m_44;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (winapi_00cd00_timeGetTime() & 1) ? lo : hi;
        } else {
            r = winapi_00cd00_timeGetTime() % span + lo;
        }
        m_countdownMs = r;
        i32 half = r >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Update(1, 0x64, half);
    } else {
        i32 lo = m_intervalLoB;
        i32 hi = m_intervalHiB;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (winapi_00cd00_timeGetTime() & 1) ? lo : hi;
        } else {
            r = winapi_00cd00_timeGetTime() % span + lo;
        }
        m_countdownMs = r;
        i32 half = r >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Update(0, 0x64, half);
    }
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(AmbSoundMap);
SIZE_UNKNOWN(AmbSoundMapHolder);
SIZE_UNKNOWN(AmbSoundRecord);
SIZE_UNKNOWN(AmbientPoint);
SIZE_UNKNOWN(PosSoundAux);
SIZE_UNKNOWN(PosSoundObj);
SIZE_UNKNOWN(PosSoundVoice);
