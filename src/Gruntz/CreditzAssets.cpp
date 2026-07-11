// CreditzAssets.cpp - CCreditsState::LoadCreditzAssets (RVA 0x39dc0), the credits
// music-bank swap tick on the canonical CCreditsState (<Gruntz/GameMode.h>).
//
// Toggles m_1c4; on the rising edge it (re)starts the CREDITZ bank and hands the
// MONOLITH cue to the global mixer; on the falling edge it stops the MONOLITH cue
// and reseats the current bank to CREDITZ. The banks are the canonical
// CGruntzSoundZ/CGruntzSoundInnerZ (<Dsndmgr/GruntzSoundZ.h>): the old local
// views' Lookup == FindBank @0x138730, m_1c == m_pCurrent, and the +0x28/+0x2c/
// +0x30 transport slots are StopAll/StopBank/Stop. The bank table is reached both
// through the state's owner back-ptr (m_4 == CState::m_4, the CGruntzMgr) and the
// 0x64556c singleton global - the same object, both reads authentic retail.
#include <rva.h>

#include <Dsndmgr/GruntzSoundZ.h>           // canonical CGruntzSoundZ / CGruntzSoundInnerZ
#include <Gruntz/GameMode.h>                // canonical CCreditsState : CState
#include <Gruntz/GruntzMgr.h>               // CGruntzMgr (m_4 / the singleton; m_sound @+0x48)
#include <Gruntz/BoundaryLowerDtorsViews.h> // CWorker39f20 (the 0x39f20 /GX leaf dtor)

extern "C" CGruntzMgr* g_gameReg; // 0x64556c (the CGruntzMgr singleton)

// The Rez heap free (0x1b9b82, __cdecl); canonical extern "C" decl (creditzassets is
// base-profile, so the /GX throwing-ness the dtor TU relied on is moot here).
extern "C" void RezFree(void* p); // 0x1b9b82

RVA(0x00039dc0, 0x10b)
void CCreditsState::LoadCreditzAssets() {
    i32 rising = (m_1c4 == 0);
    m_1c4 = rising;
    if (rising) {
        m_1bc = 0;
        m_1c0 = 3000;
        CGruntzSoundInnerZ* cred = m_4->m_sound->FindBank("CREDITZ");
        if (cred != 0 && cred->IsBusy() != 0) {
            cred->StopAll();
        }
        CGruntzSoundInnerZ* mono = m_4->m_sound->FindBank("MONOLITH");
        if (mono != 0) {
            g_gameReg->m_sound->m_pCurrent = mono;
            g_gameReg->m_sound->Restart(0);
        }
    } else {
        m_1c0 = 0;
        CGruntzSoundInnerZ* current = m_4->m_sound->m_pCurrent;
        CGruntzSoundInnerZ* mono = g_gameReg->m_sound->FindBank("MONOLITH");
        if (current == mono && mono != 0 && mono->IsBusy() != 0) {
            mono->Stop();
        }
        CGruntzSoundInnerZ* cred = m_4->m_sound->FindBank("CREDITZ");
        if (cred != 0 && current != cred) {
            m_4->m_sound->m_pCurrent = cred;
            if (cred->IsBusy() == 0) {
                cred->StopBank(0);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 0x039f20 - ~CWorker39f20 (/GX): stamp the derived vtable (0x5e971c), RezFree the
// +0x04 heap buffer, then fold the CObject base subobject (restamp the base dtor
// vtable 0x5e8cb4). Byte-shape identical to ~CRezBufferObject. __thiscall. RVA-homed
// here (RVA-contiguous with CCreditsState::LoadCreditzAssets @0x39dc0).
// @identity-TODO: the derived vtable 0x5e971c is ??_7?$CArray@PAUPLAYLISTINFOSTRUCT@@..
// (config/vtable_names.csv) - a standalone out-of-line dtor of the CArray<
// PLAYLISTINFOSTRUCT*> template; the owning STATE object that holds this CArray member
// is unrecovered, so the placeholder CWorker39f20 stays.
RVA(0x00039f20, 0x51)
CWorker39f20::~CWorker39f20() {
    if (m_4) {
        RezFree(m_4);
    }
}
