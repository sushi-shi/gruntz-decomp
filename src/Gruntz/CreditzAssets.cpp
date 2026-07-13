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

#include <Dsndmgr/GruntzSoundZ.h> // canonical CGruntzSoundZ / CGruntzSoundInnerZ
#include <Gruntz/GameMode.h>      // canonical CCreditsState : CState
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (m_4 / the singleton; m_sound @+0x48)

extern "C" CGruntzMgr* g_gameReg; // 0x64556c (the CGruntzMgr singleton)

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

// (0x039f20 is ??1?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@UAE@XZ - the movie/creditz
// playlist template dtor, RTTI-proven by the COL at its vtable 0x1e971c. The former
// placeholder `CWorker39f20` dtor here is dissolved; the real template COMDAT is
// instantiated + pinned in src/Gruntz/ArraySerialize.cpp.)
