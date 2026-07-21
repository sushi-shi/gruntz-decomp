#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>

#include <Dsndmgr/GruntzSoundZ.h> // canonical CGruntzSoundZ / CGruntzSoundInnerZ
#include <Gruntz/GameMode.h>      // canonical CCreditsState : CState
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (m_4 / the singleton; m_sound @+0x48)

RVA(0x00039dc0, 0x10b)
void CCreditsState::LoadCreditzAssets() {
    i32 rising = (m_fxEnabled == 0);
    m_fxEnabled = rising;
    if (rising) {
        m_flashTimer = 0;
        m_fadeCountdown = 3000;
        CGruntzSoundInnerZ* cred = m_mgr->m_sound->FindBank("CREDITZ");
        if (cred != 0 && cred->IsBusy() != 0) {
            cred->StopAll();
        }
        CGruntzSoundInnerZ* mono = m_mgr->m_sound->FindBank("MONOLITH");
        if (mono != 0) {
            g_gameReg->m_sound->m_pCurrent = mono;
            g_gameReg->m_sound->Restart(0);
        }
    } else {
        m_fadeCountdown = 0;
        CGruntzSoundInnerZ* current = m_mgr->m_sound->m_pCurrent;
        CGruntzSoundInnerZ* mono = g_gameReg->m_sound->FindBank("MONOLITH");
        if (current == mono && mono != 0 && mono->IsBusy() != 0) {
            mono->Stop();
        }
        CGruntzSoundInnerZ* cred = m_mgr->m_sound->FindBank("CREDITZ");
        if (cred != 0 && current != cred) {
            m_mgr->m_sound->m_pCurrent = cred;
            if (cred->IsBusy() == 0) {
                cred->StopBank(0);
            }
        }
    }
}
