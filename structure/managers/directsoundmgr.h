#ifndef MANAGERS_DIRECTSOUNDMGR_H
#define MANAGERS_DIRECTSOUNDMGR_H

/*
 * DirectSoundMgr — DirectSound audio manager (+ SoundFont via SFMAN32).
 *
 * Leaked source TU:  C:\Proj\Dsndmgr\DSNDMGR.CPP
 *   companion:       DSndMgSR.cpp  (sound resource? "SR")
 *
 * Provenance: the names DirectSoundMgr / SFManager / DSndMgSR are mined from
 * strings (STRINGS_ANALYSIS.md §17 manager map; §9 SFMAN32/SFManager). They are
 * NOT in RTTI, so no @rtti tags. NO layout recovered — all @todo.
 *
 * Audio stack: DSOUND (DirectSound) + mss32 (Miles Sound System, XMI/MIDI digital
 * audio) + SFMAN32 (AIL SoundFont manager, Gruntz.SF2 / Gruntz4.SF2 banks).
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>
#include <dsound.h>

/*
 * PLAYLISTINFOSTRUCT — music playlist entry. The ONLY game-side template
 * instantiation besides zDArray is CArray<PLAYLISTINFOSTRUCT*>:
 *   .?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@
 * So PLAYLISTINFOSTRUCT is a real struct in the music subsystem. Layout @todo.
 */
struct PLAYLISTINFOSTRUCT
{
    //@size: unknown @todo
    //@todo: fields unknown (likely track id/name + flags). Held in a music
    //       playlist CArray.
};

/* SFManager — SoundFont manager (SFMAN32.DLL wrapper). Name from strings. */
class SFManager
{
public:
    //@size: unknown @todo
    //@todo: loads Gruntz.SF2 / Gruntz4.SF2 (4MB bank). Layout unknown.
};

/* DSndMgSR — DSndMgSR.cpp companion (sound resource handling?). Name from path. */
class DSndMgSR
{
public:
    //@size: unknown @todo
    //@todo: role/layout unknown.
};

/*
 * DirectSoundMgr — the DirectSound manager (primary class of DSNDMGR.CPP).
 * Name source: strings/manager-map only — no RTTI.
 */
class DirectSoundMgr
{
public:
    //@size: unknown @todo
    //@todo: init/buffers/3D-positional audio unknown. DSERR_* stringify table is
    //       embedded (ALLOCATED/BADFORMAT/NODRIVER/GENERIC + DS_OK).
};

#endif /* MANAGERS_DIRECTSOUNDMGR_H */
