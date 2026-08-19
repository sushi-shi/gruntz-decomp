#ifndef GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
#define GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Wap32/Object.h>

typedef struct _SEQUENCE* HSEQUENCE;

typedef struct _MDI_DRIVER* HMDIDRIVER_;

extern HMDIDRIVER_ g_ailMidiDriver;

void __stdcall EmptyMsgHook(WPARAM wParam, LPARAM lParam);

class CGruntzSoundInnerZ : public CObject {
public:
    virtual ~CGruntzSoundInnerZ() OVERRIDE;

    virtual i32 DecodeBuf(const void* buf, u32 len, const char* name);

    virtual i32 Load(const char* path, const char* name);
    virtual void ReleaseHandle();
    virtual i32 IsStarted();
    virtual i32 Play(HWND hOwner, i32 mode);
    virtual i32 StopAll();
    virtual i32 StopBank(i32 bank);
    virtual i32 Stop();
    virtual i32 Retrigger();

    virtual i32 IsMidi();
    virtual i32 LoadSpecial(const char* resName, const char* name);

    CGruntzSoundInnerZ() {
        m_name[0] = 0;
        m_pauseDepth = 0;
        m_playMode = 0;
        m_playOwner = NULL;
        m_tempoPct = 0x64;
        m_volumePct = 0x64;
        m_seqHandle = NULL;
        m_loadBuffer = NULL;
    }

    i32 IsBusy();

    i32 SetTempo(i32 tempo, i32 ms);
    i32 SetVolume(i32 volume, i32 ms);
    i32 SetLoop(i32 loop);

    char m_name[0x40];
    i32 m_pauseDepth;
    i32 m_playMode;
    HWND m_playOwner;
    i32 m_volumePct;
    i32 m_tempoPct;
    HSEQUENCE m_seqHandle;
    char* m_loadBuffer;
};

class CGruntzSoundZ {
public:
    ~CGruntzSoundZ() {
        Shutdown();
    }

    i32 Init(HINSTANCE hInst, HWND hwnd, i32 noMidi);
    void Shutdown();
    void StopAndFlush();

    i32 SetXMidiVolume(i32 volume);
    i32 GetXMidiVolume();
    CGruntzSoundInnerZ* CreateBank2(const char* path, const char* name);
    CGruntzSoundInnerZ* CreateBank(const void* buf, u32 len, const char* name);
    void Insert(CGruntzSoundInnerZ* inner);
    CGruntzSoundInnerZ* FindBank(const char* key);
    i32 PlayCreate2(const char* path, i32 playMode, const char* name);
    i32 PlayCreate3(const void* buf, u32 len, i32 playMode, const char* name);
    i32 PlayByName(const char* name, i32 playMode);
    void StopCurrent();
    i32 Restart(i32 playMode);
    i32 StopAll();
    i32 StopBank(i32 bank);
    i32 IsPlaying();
    i32 Retrigger();

    CMapStringToOb m_map;
    CGruntzSoundInnerZ* m_pCurrent;
    HWND m_ownerWnd;
    HINSTANCE m_hInstance;
    i32 m_enabled;
};

#endif // GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
