// CreditzAssets.cpp - the credits music-bank swap tick (RVA 0x39dc0).
//
// Toggles m_1c4; on the rising edge it (re)starts the CREDITZ bank and hands the
// MONOLITH cue to the global mixer; on the falling edge it stops the MONOLITH cue
// and reseats the current bank to CREDITZ. Field names are placeholders; only
// offsets + code bytes are load-bearing.
#include <rva.h>

#include <Ints.h>

// The per-bank sound instance: a small vtable (transport controls at +0x28/+0x2c/
// +0x30) plus a non-virtual busy poll.
class CGruntzSoundInnerZ {
public:
    virtual void V00();
    virtual void V04();
    virtual void V08();
    virtual void V0C();
    virtual void V10();
    virtual void V14();
    virtual void V18();
    virtual void V1C();
    virtual void V20();
    virtual void V24();
    virtual void Stop();    // +0x28
    virtual void Play(i32); // +0x2c
    virtual void Fade();    // +0x30
    i32 IsBusy();           // 0x138f60
};

class CGruntzSoundZ {
public:
    CGruntzSoundInnerZ* Lookup(char* name); // 0x138730
    i32 Restart(i32 flag);                  // 0x1388c0
    char m_pad00[0x1c];
    CGruntzSoundInnerZ* m_1c; // +0x1c current bank
};

SIZE_UNKNOWN(CreditzMgr);
struct CreditzMgr {
    char m_pad00[0x48];
    CGruntzSoundZ* m_48; // +0x48 bank table
};
extern "C" CreditzMgr* g_mgrSettings; // 0x64556c

class CCreditsState {
public:
    void LoadCreditzAssets();
    char m_pad00[0x4];
    CreditzMgr* m_4; // +0x04
    char m_pad08[0x1bc - 0x8];
    i32 m_1bc; // +0x1bc
    i32 m_1c0; // +0x1c0
    i32 m_1c4; // +0x1c4 toggle
};

RVA(0x00039dc0, 0x10b)
void CCreditsState::LoadCreditzAssets() {
    i32 rising = (m_1c4 == 0);
    m_1c4 = rising;
    if (rising) {
        m_1bc = 0;
        m_1c0 = 3000;
        CGruntzSoundInnerZ* cred = m_4->m_48->Lookup("CREDITZ");
        if (cred != 0 && cred->IsBusy() != 0) {
            cred->Stop();
        }
        CGruntzSoundInnerZ* mono = m_4->m_48->Lookup("MONOLITH");
        if (mono != 0) {
            g_mgrSettings->m_48->m_1c = mono;
            g_mgrSettings->m_48->Restart(0);
        }
    } else {
        m_1c0 = 0;
        CGruntzSoundInnerZ* current = m_4->m_48->m_1c;
        CGruntzSoundInnerZ* mono = g_mgrSettings->m_48->Lookup("MONOLITH");
        if (current == mono && mono != 0 && mono->IsBusy() != 0) {
            mono->Fade();
        }
        CGruntzSoundInnerZ* cred = m_4->m_48->Lookup("CREDITZ");
        if (cred != 0 && current != cred) {
            m_4->m_48->m_1c = cred;
            if (cred->IsBusy() == 0) {
                cred->Play(0);
            }
        }
    }
}
