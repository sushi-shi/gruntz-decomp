#ifndef DSNDMGR_WAVEFORMATPTR_H
#define DSNDMGR_WAVEFORMATPTR_H

// The PCM header at the DirectSound boundary.
//
// WaveFormatX models the record the way retail MOVES it - two of its u16 pairs go
// down as single dwords, which is why it is not simply the SDK struct - while
// DSBUFFERDESC::lpwfxFormat wants the SDK's type. Same 0x14 bytes, both readings
// real, so both are named instead of punned at the descriptor fill.
//
// Deliberately its own header: LPWAVEFORMATEX needs <mmsystem.h>, and pulling that
// into <Dsndmgr/SoundDevice.h> would drag it through every consumer of the sound
// device (SBI_RectOnly among them). Include this only where dsound.h already is.

#include <Dsndmgr/WaveFormatX.h> // the engine's record shape
#include <mmsystem.h>            // LPWAVEFORMATEX - the SDK's

union WaveFormatPtr {
    WaveFormatX* m_rec;   // the engine's dword-moved record
    LPWAVEFORMATEX m_sdk; // what DSBUFFERDESC::lpwfxFormat takes
};

#endif // DSNDMGR_WAVEFORMATPTR_H
