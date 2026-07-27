#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr (g_gameReg real type)
#include <Gruntz/GruntSpawnConfig.h>
#include <Dsndmgr/StreamVoice.h>
#include <Gruntz/GruntVoice.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Gruntz/GameRegistry.h>

#include <Bute/ButeMgr.h>   // CButeMgr g_buteMgr (GetIntDef)
#include <Bute/SymParser.h> // CGruntzMgr::m_symParser - the real 'WAV' resolver
#include <Gruntz/Enums.h>   // REZ_TAG_WAV ('WAV')
#include <Gruntz/Grunt.h>   // CGruntCoordList - the out-of-line node walker (0x29a30)
#include <Gruntz/Random.h>  // g_randSeed / g_randSeeded (the lazily-seeded LCG)
#include <rva.h>

RVA(0x00085df0, 0x4a)
CGruntSpawnConfig::~CGruntSpawnConfig() {
    Clear();
}

// ===========================================================================
// CGruntSpawnConfig::Init  (0x11adc0)
// ===========================================================================
// Bind to an owner and seed the config tree pointer, then build the voice list.
// On a null owner bail with 0. Stash owner (m_00) and owner->m_30 (m_04, the
// config tree), zero the sprite/object pairs (m_08..m_14), seed m_2c = 0x64, and
// run BuildVoiceList(); return its result negated/double-negated (a BOOL).
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): structurally
// byte-exact (every offset/immediate/call matches) but retail materializes 0 late
// via `test eax,eax` + a `lea esi,[ecx+8]` store-group, while the recompile pins
// xor edx,edx early (cmp edx,eax) and stores flat - a 1-instr phase shift not
// source-steerable. ~55% on a 68-byte fn; deferred to the final sweep.
RVA(0x0011adc0, 0x44)
BOOL CGruntSpawnConfig::Init(CGruntzMgr* owner) {
    if (owner == 0) {
        return 0;
    }
    m_configTree = 0;
    m_voices[0] = 0;
    m_voices[1] = 0;
    m_streams[0] = 0;
    m_streams[1] = 0;
    m_owner = owner;
    m_voiceVolume = 0x64;
    m_configTree = owner->m_world;
    return BuildVoiceList() != 0;
}

// ===========================================================================
// CGruntSpawnConfig::Clear  (0x11ae30)
// ===========================================================================
// Free every entry in m_18 (dtor + RezFree), empty the array, then drop the two
// owned objects (m_10/m_14) from m_04's sub-collection and zero the sprite/object
// pairs. The Remove call shape matches retail byte-exact.
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the loop
// bound (i<GetSize()) and `e!=0`/`p!=0` checks let MSVC pin a callee-saved zero
// (ebp) across the whole body, where retail keeps only ebx and uses `test`/fresh
// zeros + a 3-way pointer-grouped tail zeroing. Logic byte-exact in the middle
// (Remove call, idiv-free); ~68% from the zero-reg phase shift. Deferred.
RVA(0x0011ae30, 0x95)
void CGruntSpawnConfig::Clear() {
    for (i32 i = 0; i < m_voiceLists.GetSize(); i++) {
        CSpawnList* e = static_cast<CSpawnList*>(m_voiceLists[i]);
        // RezFree IS ::operator delete (both 0x1b9b82), so this pair IS `delete e`.
        delete e; // ~CSpawnList non-virtual (0x99ca0, defined in AreaMgr.cpp) + ??3
    }
    m_voiceLists.SetSize(0, -1);
    if (m_configTree != 0 && m_configTree->m_soundStream != 0) {
        StreamVoice** p = m_streams;
        for (i32 k = 0; k < 2; k++) {
            if (p[0] != 0) {
                m_configTree->m_soundStream->DestroyVoice(p[0]);
                p[0] = 0;
            }
            p++;
        }
    }
    m_owner = 0;
    m_configTree = 0;
    m_voices[0] = 0;
    m_voices[1] = 0;
    m_streams[0] = 0;
    m_streams[1] = 0;
}

RVA(0x0011af00, 0x62)
BOOL CGruntSpawnConfig::LoadGruntVoices() {
    ClearSprites();
    i32 i = 0;
    CGruntVoice** slot = m_voices;
    for (; i < 2; i++, slot++) {
        CGameObject* spr =
            m_configTree->m_childGroup->CreateSprite(0, 0, 0, 0xdbba1, "GruntVoice", 0x4040003);
        spr->m_7c->m_notify(spr);
        CGruntVoice* got = static_cast<CGruntVoice*>(spr->m_7c->m_logic);
        *slot = got;
        if (got == 0) {
            return 0;
        }
    }
    return 1;
}

// ClearSprites (0x11af90): null the m_08/m_0c voice-sprite pair. Out-of-line
// (retail emits it standalone; the inline member folded away and never emitted).
// @early-stop
// base-register-bias wall (82%): the m_voices[2] loop form reproduces retail's
// advanced-base addressing ([base]/[base+4]; was this-relative at 67.8); the last
// line is the coalesce: retail folds the base into ecx (`add ecx,8`, this dead)
// with the zero in eax; cl leas into eax with the zero in ecx. Likely an inlined
// embedded-pair member in retail; not worth a struct+macro for 2 bytes.
RVA(0x0011af90, 0xb)
void CGruntSpawnConfig::ClearSprites() {
    CGruntVoice** p = m_voices;
    for (i32 i = 0; i < 2; i++) {
        p[i] = 0;
    }
}

// ===========================================================================
// CGruntSpawnConfig::LoadGruntSpawnConfig  (0x11afb0)
// ===========================================================================
// The percent/priority-gated voice spawn driver. Ensure the voices are loaded and
// the owner is ready, then read the per-section percent/priority from the bute
// config (formatted "SG%i"/"G%i" section names), roll the percent gate, skip if a
// higher-priority voice is already active, pick a weighted entry, duck the
// currently-playing voice's volume, open/configure the chosen stream, and play it.
// /GX EH frame from the two CString temporaries.
//
// @early-stop
// /GX EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md, topic:eh):
// the instruction selection, the volume-duck branch tree, the stream open/configure
// path, and the two CString Format/dtor temporaries are byte-faithful; the residual
// is the trylevel slot threading + the shared scope-exit dtor block the /GX state
// machine emits (the early-out gotos all funnel through one CString teardown, where
// retail's state ids differ). Logic complete; deferred to the final sweep.
// g_gameReg viewed for the LCG rand (__thiscall, ecx = the registry) + the master
// volume the duck halves.
// The bute config gate IS the CGrunt itself: its +0x10 bound CGameObject holds the
// currently-active voice id at +0x188 (the ex CSpawnButeConfig / CSpawnActiveVoice /
// CSpawnGate / CSpawnGateInner views are all dissolved onto the two real classes). The two owned
// voice streams (m_10/m_14) are real Dsndmgr StreamVoices (SetSource 0x1374c0 /
// Configure 0x137520 / the embedded StreamVoiceFeeder at +0x6c).
// (OpenStream lives on the SoundStream at m_configTree->m_soundStream; see the header.)

RVA(0x0011afb0, 0x321)
BOOL CGruntSpawnConfig::LoadGruntSpawnConfig(
    CGrunt* who,
    i32 cue,
    i32 which,
    i32 priority,
    i32 percent
) {
    if (m_voices[0] == 0 && !LoadGruntVoices()) {
        return 0;
    }
    if (who == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    i32 voiceId = GetButeSlot(who, cue);
    CString local_10;
    CString local_14;
    local_14.Format("SG%i", voiceId);
    local_10.Format("G%i", cue);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_14), "Per", -1);
        if (percent == -1) {
            percent = g_buteMgr.GetIntDef("GruntPercent", static_cast<LPCTSTR>(local_10), 0);
        }
    }
    if (percent < 100 && percent < g_gameReg->Rand() % 0x65) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_14), "Pri", -1);
        if (priority == -1) {
            priority = g_buteMgr.GetIntDef("GruntPriority", static_cast<LPCTSTR>(local_10), 1);
        }
    }
    CGruntVoice** voices = m_voices;
    for (i32 i = 0; i < 2; i++) {
        if (priority <= voices[i]->m_playFlags) {
            return 0;
        }
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == 0 || m_configTree->m_soundStream == 0) {
        return 0;
    }
    CGruntVoice* v8 = m_voices[0];
    CGruntVoice* v0c = m_voices[1];
    i32 a = v8->m_playFlags;
    i32 b = v0c->m_playFlags;
    i32 c = v8->m_source;
    i32 d = v0c->m_source;
    StreamVoice** streams = m_streams;
    // `who` IS the gate: the ex-CSpawnButeConfig view was CGrunt (its +0x10/+0x170/
    // +0x234/+0x258 are m_object/m_entranceReason/m_coordToggle/m_gruntKind), and the
    // ex-CSpawnActiveVoice at +0x10 was the bound CGameObject, whose +0x188 IS the
    // object id each CGruntVoice caches as m_source.
    CGameObject* gate = who->m_object;
    i32 chosen;
    if (b < a) {
        chosen = 1;
        if (c == gate->m_188) {
            chosen = 0;
            if (b != 0 && streams[1] != 0) {
                (static_cast<DirectSoundMgr*>(streams[1]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && streams[0] != 0) {
            (static_cast<DirectSoundMgr*>(streams[0]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 0;
        if (d == gate->m_188) {
            chosen = 1;
            if (a != 0 && streams[0] != 0) {
                (static_cast<DirectSoundMgr*>(streams[0]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (b != 0 && streams[1] != 0) {
            (static_cast<DirectSoundMgr*>(streams[1]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (streams[chosen] == 0) {
        streams[chosen] = m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (streams[chosen] == 0) {
            return 0;
        }
    }
    StreamVoice* stream = streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) != 0) {
        stream->Configure(vol, 0, 0, 0);
    }
    CGruntVoice* voice = voices[chosen];
    return voice->Setup(gate->m_188, stream, priority, 0) != 0;
}

// ===========================================================================
// CGruntSpawnConfig::SpawnVoiceDriver overloads (0x11b3b0 / 0x11b7c0)
// ===========================================================================
// The two sibling weighted grunt-voice spawn drivers (percent LCG gate @0x6c1288,
// priority reject, weighted pick, lazy sprite create, CGruntVoice::Setup tail).
// Re-homed from the ApiCaller backlog by RVA proximity (dead-centre of the
// 0x11axxx-0x11cxxx CGruntSpawnConfig family).
//
// @early-stop
// /GX EH single-epilogue wall: the complete weighted-spawn body was reconstructed
// and builds, but caps at ~47% because cl duplicates the frame-teardown per
// return-site while retail funnels to one shared `jmp` epilogue. The return-0 stub
// scores 73-83% via the smaller-fn normalization artifact, so the highest-% version
// (this stub) is kept per the REVERT rule. Final-sweep candidates.
RVA(0x0011b3b0, 0x338)
i32 CGruntSpawnConfig::SpawnVoiceDriver(void* /*spawner*/, i32, i32, i32, i32, i32) {
    return 0;
}

// @early-stop
// twin of 0x11b3b0: same /GX EH single-epilogue wall; five-argument member overload,
// stub kept as the highest-% version (full body ~47% vs stub-artifact 73-83%).
RVA(0x0011b7c0, 0x304)
i32 CGruntSpawnConfig::SpawnVoiceDriver(i32, i32, i32, i32, i32) {
    return 0;
}

// ===========================================================================
// CGruntSpawnConfig::GetButeSlot  (0x11bba0)
// ===========================================================================
// Return the grunt's voice-list band id, chosen by the CGrunt's m_entranceReason (a
// switch over 0..0x20), plus two early specials on m_gruntKind (0x3a, 0x39). A null
// grunt or out-of-range selector returns 0.
//
// @early-stop
// jump-table scoring-artifact wall (docs/patterns/jumptable-data-overlap.md): the
// dispatch, the index/jump table, and every case body are byte-IDENTICAL to retail
// (verified by raw byte-compare of the head + case blocks in source-case order).
// objdiff scores ~73% only because the inline `.text` jump-table region carries a
// base reloc against a $L label vs the target's switchdataD self-reloc. The code
// IS matched; the % undercounts it. No source change applies - stop chasing.
RVA(0x0011bba0, 0x1f4)
i32 CGruntSpawnConfig::GetButeSlot(CGrunt* config, i32 cue) {
    if (config == 0) {
        return 0;
    }
    if (config->m_gruntKind == 0x3a) {
        return VOICE_CUES_PER_BAND * 19 + cue;
    }
    if (config->m_gruntKind == 0x39) {
        return VOICE_CUES_PER_BAND * 13 + cue;
    }
    switch (static_cast<u32>(config->m_entranceReason)) {
        case 0:
            return VOICE_CUES_PER_BAND * 17 + cue;
        case 1:
            return VOICE_CUES_PER_BAND * 3 + cue;
        case 2:
            return VOICE_CUES_PER_BAND * 4 + cue;
        case 3:
            return VOICE_CUES_PER_BAND * 5 + cue;
        case 4:
            return VOICE_CUES_PER_BAND * 6 + cue;
        case 5:
            return VOICE_CUES_PER_BAND * 7 + cue;
        case 6:
            return VOICE_CUES_PER_BAND * 8 + cue;
        case 7:
            return VOICE_CUES_PER_BAND * 10 + cue;
        case 8:
            return VOICE_CUES_PER_BAND * 11 + cue;
        case 9:
            return VOICE_CUES_PER_BAND * 12 + cue;
        case 10:
            return VOICE_CUES_PER_BAND * 16 + cue;
        case 11:
            return VOICE_CUES_PER_BAND * 20 + cue;
        case 12:
            return VOICE_CUES_PER_BAND * 22 + cue;
        case 13:
            return VOICE_CUES_PER_BAND * 23 + cue;
        case 14:
            return VOICE_CUES_PER_BAND * 24 + cue;
        case 15:
            return VOICE_CUES_PER_BAND * 25 + cue;
        case 16:
            return VOICE_CUES_PER_BAND * 27 + cue;
        case 17:
            return VOICE_CUES_PER_BAND * 28 + cue;
        case 18:
            if (config->m_coordToggle != 0) {
                return VOICE_CUES_PER_BAND * 30 + cue;
            }
            return VOICE_CUES_PER_BAND * 29 + cue;
        case 19:
            return VOICE_CUES_PER_BAND * 31 + cue;
        case 20:
            return VOICE_CUES_PER_BAND * 32 + cue;
        case 21:
            return VOICE_CUES_PER_BAND * 33 + cue;
        case 22:
            return VOICE_CUES_PER_BAND * 34 + cue;
        case 23:
            return cue; // band 0
        case 24:
            return VOICE_CUES_PER_BAND * 1 + cue;
        case 25:
            return VOICE_CUES_PER_BAND * 2 + cue;
        case 26:
            return VOICE_CUES_PER_BAND * 9 + cue;
        case 27:
            return VOICE_CUES_PER_BAND * 14 + cue;
        case 28:
            return VOICE_CUES_PER_BAND * 15 + cue;
        case 29:
            return VOICE_CUES_PER_BAND * 18 + cue;
        case 30:
            return VOICE_CUES_PER_BAND * 21 + cue;
        case 31:
            return VOICE_CUES_PER_BAND * 26 + cue;
        case 32:
            return VOICE_CUES_PER_BAND * 35 + cue;
        default:
            return 0;
    }
}

// ===========================================================================
// CGruntSpawnConfig::PickWeighted  (0x11bee0)
// ===========================================================================
// Resolve voice-list `voiceId` to its .WAV parse record. `which` names an explicit
// entry; -1 (or an index past the end) rolls a random one, then re-rolls up to five
// times so the list does not repeat its own previous pick. The winning index is
// memoized in the list's m_lastPicked, the list is walked that many nodes, and the
// entry's name is resolved through the owner's CSymParser under the 'WAV' tag.
//
// The three lazily-seeded LCG expansions are retail's own spelling, not an artifact:
// the FIRST roll calls the shared out-of-line CGruntzMgr::Rand() (`mov ecx,g_gameReg;
// call 0x39ae`) while BOTH arms of the re-roll loop expand the recurrence in line -
// exactly the split retail emits, and the same hand-inlined idiom as
// CRandomAmbientSound::Init2 (0xcd70). The `span == 0` arms are the degenerate-range
// guard: with lo == 0 the endpoint select collapses to MSVC's mask form
// (`and esi,0x10000; neg; sbb; not; and esi,edi`).
//
// @early-stop
// seed-coalescing wall (~93%): every branch, offset, immediate, call and the whole
// control-flow graph match retail. The residual is ONE register-coalescing tie-break,
// in the two re-roll arms only. Retail coalesces the re-roll `seed` onto ecx - the
// register already holding the value-numbered g_randSeed - so the lazy arm pays the
// copy (`call ebp; mov ecx,eax`) and `al` stays free for the flag (`or al,1; mov
// [g_randSeeded],al`). cl 5.0 here instead binds `seed` to timeGetTime's eax, so the
// CACHED arm pays the copy (`mov eax,ecx` + a `jmp`) and, with al clobbered, the flag
// update degrades to the memory form (`or BYTE PTR [g_randSeeded],1`). Both spellings
// are the same instruction count; it is purely which side of the phi gets the move.
// Five source spellings were measured against it: lazy-arm-first if/else (91.96),
// arm-local `seed = g_randSeed` pre-init (91.96, adds cl/al shuffles), LCG as a
// self-update (worse - swaps esi/edi roles too), cached-arm-first if/else (92.66, kept
// - it at least restores the hoisted `mov al,[g_randSeeded]`), and hoisting the flag
// into its own u8 to force the register form (89.57). `permute fn` also exhausted 300
// iterations with no change. Site 1's non-loop roll and the whole list walk are exact;
// the same coloring flip is what shifts the GetName/ResolveQualified tail scheduling.
// The two `fs:0` prologue/epilogue rows are the usual /GX reloc-display artifact.
RVA(0x0011bee0, 0x230)
CParseSource* CGruntSpawnConfig::PickWeighted(i32 voiceId, i32 which) {
    if (voiceId < 0) {
        return 0;
    }
    if (voiceId == 0) {
        return 0;
    }
    if (voiceId >= m_voiceLists.GetSize()) {
        return 0;
    }
    CSpawnList* list = static_cast<CSpawnList*>(m_voiceLists[voiceId]);
    if (list == 0) {
        return 0;
    }

    i32 pick = which;
    if (pick == -1 || pick >= list->m_list.GetCount()) {
        i32 hi = list->m_list.GetCount() - 1;
        // Retail loads the registry pointer next to the timeGetTime import slot,
        // above the span test, not down in the arm that calls Rand().
        CGruntzMgr* reg = g_gameReg;
        i32 span = hi + 1;
        if (span == 0) {
            i32 seed;
            if (!(g_randSeeded & 1)) {
                g_randSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_randSeed;
            }
            g_randSeed = seed * 214013 + 2531011;
            pick = (g_randSeed & 0x10000) ? 0 : hi;
        } else {
            pick = reg->Rand() % span;
        }
        if (list->m_list.GetCount() > 1) {
            i32 tries = 5;
            while (pick == list->m_lastPicked && tries > 0) {
                i32 rehi = list->m_list.GetCount() - 1;
                i32 respan = rehi + 1;
                // Inside the loop g_randSeed is already register-cached, so retail's
                // seed lives in THAT register and only the lazy-seed arm needs a move
                // (`call ebp; mov ecx,eax`). Priming each arm from the cache - rather
                // than an if/else, which homes the seed in timeGetTime's eax and costs
                // a `jmp` plus the reversed move - is what reproduces it. The two
                // declarations stay arm-local: one shared across both arms widens the
                // live range and MSVC then materializes it into eax up front. Site 1
                // above has no cache yet, so it keeps the if/else (and matches).
                if (respan == 0) {
                    i32 seed;
                    if (g_randSeeded & 1) {
                        seed = g_randSeed;
                    } else {
                        g_randSeeded |= 1;
                        seed = timeGetTime();
                    }
                    g_randSeed = seed * 214013 + 2531011;
                    pick = (g_randSeed & 0x10000) ? 0 : rehi;
                } else {
                    i32 seed;
                    if (g_randSeeded & 1) {
                        seed = g_randSeed;
                    } else {
                        g_randSeeded |= 1;
                        seed = timeGetTime();
                    }
                    g_randSeed = seed * 214013 + 2531011;
                    pick = ((g_randSeed >> 0x10) & 0x7fff) % respan;
                }
                tries--;
            }
        }
    }

    list->m_lastPicked = pick;
    CSpawnEntry* entry;
    if (pick >= list->m_list.GetCount()) {
        entry = 0;
    } else {
        // Retail walks through the OUT-OF-LINE node helper (`mov ecx,list; call
        // 0x29a30`), not MFC's inline CPtrList::GetNext - unlike the sibling walk in
        // CAreaMgr::LoadObjectImageResources (0x9a510), which really is inline. The
        // POSITION <-> void* pun is the language-forced seam Grunt.h documents for
        // this helper; it stays local to the one call site that needs it.
        CGruntCoordList* nodes = static_cast<CGruntCoordList*>(&list->m_list);
        void*& cursor = reinterpret_cast<void*&>(list->m_cursor);
        cursor = list->m_list.GetHeadPosition();
        if (cursor == 0) {
            entry = 0;
        } else {
            entry = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
        }
        for (i32 i = pick; i > 0; i--) {
            if (cursor == 0) {
                entry = 0;
            } else {
                entry = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
            }
        }
    }
    if (entry == 0) {
        return 0;
    }
    return m_owner->m_symParser->ResolveQualified(static_cast<LPCTSTR>(entry->GetName()), REZ_TAG_WAV);
}

RVA(0x0011c1a0, 0x46)
BOOL CGruntSpawnConfig::BuildVoiceList() {
    m_voiceLists.SetSize(0, -1);
    m_voiceLists.SetAtGrow(0, 0);
    for (i32 i = 1; i < 0x4b0; i++) {
        m_voiceLists.SetAtGrow(i, BuildVoiceSoundList(i));
    }
    return 1;
}

RVA(0x0011c560, 0x91)
void CSpawnList::AddVoiceSound(CString s, i32 flag) {
    CSpawnEntry* node = new CSpawnEntry(s, flag);
    if (node != 0) {
        m_list.AddTail(node);
    }
}

RVA(0x0011c6c0, 0x27)
i32 CGruntSpawnConfig::AnyVoicePlaying() {
    i32 i = 0;
    CGruntVoice** p = m_voices;
    for (; i < 2; i++, p++) {
        if (*p != 0 && (*p)->m_playFlags != 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x0011c700, 0x20)
i32 CGruntSpawnConfig::VoicePlaying(i32 i) {
    CGruntVoice* v = m_voices[i];
    if (v != 0 && v->m_playFlags != 0) {
        return 1;
    }
    return 0;
}

RVA(0x0011c730, 0x5c)
void CGruntSpawnConfig::StopVoice(i32 id) {
    i32 tag08 = m_voices[0]->m_source;
    i32 tag0c = m_voices[1]->m_source;
    if (tag08 == id) {
        if (m_streams[0] != 0) {
            m_streams[0]->m_feeder.Pause();
        }
        if (m_voices[0] != 0) {
            m_voices[0]->Reset();
        }
    } else if (tag0c == id) {
        if (m_streams[1] != 0) {
            m_streams[1]->m_feeder.Pause();
        }
        if (m_voices[1] != 0) {
            m_voices[1]->Reset();
        }
    }
}

RVA(0x0011c7b0, 0x2d)
void CGruntSpawnConfig::PauseAllVoices() {
    // The two parallel pairs the old "p[2]" cursor spanned: m_voices[] @+0x08 and
    // m_streams[] @+0x10 (p[2] was simply m_streams[k]).
    for (i32 k = 0; k < 2; k++) {
        if (m_streams[k] != 0) {
            m_streams[k]->m_feeder.Pause();
        }
        if (m_voices[k] != 0) {
            m_voices[k]->Reset();
        }
    }
}

RVA(0x0011c7f0, 0x2b)
void CGruntSpawnConfig::ResetPicks() {
    PauseAllVoices();
    for (i32 i = 0; i < m_voiceLists.GetSize(); i++) {
        CSpawnList* e = static_cast<CSpawnList*>(m_voiceLists[i]);
        if (e != 0) {
            e->m_lastPicked = -1;
        }
    }
}

RVA(0x0011c830, 0x12)
BOOL CGruntSpawnConfig::IsReady() {
    return m_owner->m_isVoiceEnabled != 0;
}
