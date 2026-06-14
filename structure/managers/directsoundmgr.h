#ifndef MANAGERS_DIRECTSOUNDMGR_H
#define MANAGERS_DIRECTSOUNDMGR_H

/*
 * DirectSoundMgr — DirectSound audio manager (+ SoundFont via SFMAN32).
 *
 * Leaked source TU:  C:\Proj\Dsndmgr\DSNDMGR.CPP   companion: DSndMgSR.cpp
 *
 * Provenance: the names DirectSoundMgr / SFManager / DSndMgSR are mined from
 * strings (STRINGS_ANALYSIS.md §17 manager map; §9 SFMAN32/SFManager). They are
 * NOT in RTTI. NO layout recovered — name-only stubs.
 *
 * Audio stack: DSOUND (DirectSound) + mss32 (Miles Sound System, XMI/MIDI digital
 * audio) + SFMAN32 (AIL SoundFont manager, Gruntz.SF2 / Gruntz4.SF2 banks).
 * DSERR_* stringify table is embedded (ALLOCATED/BADFORMAT/NODRIVER/GENERIC + DS_OK).
 */

/*
 * PLAYLISTINFOSTRUCT — music playlist entry. The ONLY game-side template
 * instantiation besides zDArray is CArray<PLAYLISTINFOSTRUCT*>:
 *   .?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@
 * So PLAYLISTINFOSTRUCT is a real struct in the music subsystem. Layout unknown
 * (likely track id/name + flags; held in a music playlist CArray).
 */
struct PLAYLISTINFOSTRUCT { /* music playlist entry; fields unknown */ };

/* SFManager — SoundFont manager (SFMAN32.DLL wrapper; loads Gruntz.SF2/Gruntz4.SF2). */
class SFManager      { /* layout unknown */ };

/* DSndMgSR — DSndMgSR.cpp companion (sound-resource handling?). */
class DSndMgSR       { /* role/layout unknown */ };

/* DirectSoundMgr — the DirectSound manager (primary class of DSNDMGR.CPP). */
class DirectSoundMgr { /* init/buffers/3D-positional audio; layout unknown */ };

#endif /* MANAGERS_DIRECTSOUNDMGR_H */
