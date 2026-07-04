// vendor/miles/Mss32.h - the Miles Sound System (MSS32.DLL) API surface Gruntz imports.
//
// Third-party audio middleware ("AIL" = Audio Interchange Library, RAD Game Tools'
// Miles Sound System). GRUNTZ.EXE reaches these through the MSS32.DLL import table
// (the `call ds:[__imp__AIL_*@N]` IAT thunks); each is __stdcall with C linkage, so
// the decorated symbol is `_AIL_<name>@<argbytes>`. These 16 are the COMPLETE set of
// AIL entries in the retail IAT (0x6c46e8-0x6c4724) - Gruntz uses only the XMIDI/MIDI
// sequence side of Miles; digital audio goes through DirectSound instead.
//
// The handle/S32 typedefs alias i32 (all args are 4 bytes, so the @N decoration is
// unchanged) - matching-neutral, but they document the real Miles types.
#ifndef GRUNTZ_VENDOR_MSS32_H
#define GRUNTZ_VENDOR_MSS32_H

#include <Ints.h> // i32 (== Miles S32)

typedef i32 S32;        // Miles signed 32-bit scalar
typedef i32 HMDIDRIVER; // XMIDI/MIDI driver handle
typedef i32 HSEQUENCE;  // XMIDI sequence handle

extern "C" {
    // Library bring-up / teardown.
    __declspec(dllimport) void __stdcall AIL_startup();
    __declspec(dllimport) void __stdcall AIL_shutdown();
    __declspec(dllimport) HMDIDRIVER __stdcall
    AIL_midiOutOpen(HMDIDRIVER* mdi, void* fmt, S32 devid);

    // XMIDI master-volume control.
    __declspec(dllimport) S32 __stdcall AIL_set_XMIDI_master_volume(HMDIDRIVER mdi, S32 volume);
    __declspec(dllimport) S32 __stdcall AIL_XMIDI_master_volume(HMDIDRIVER mdi);

    // Sequence lifetime.
    __declspec(dllimport) HSEQUENCE __stdcall AIL_allocate_sequence_handle(HMDIDRIVER mdi);
    __declspec(dllimport) S32 __stdcall AIL_init_sequence(HSEQUENCE seq, void* xmidi, S32 seqNum);
    __declspec(dllimport) void __stdcall AIL_release_sequence_handle(HSEQUENCE seq);

    // Sequence transport.
    __declspec(dllimport) S32 __stdcall AIL_start_sequence(HSEQUENCE seq);
    __declspec(dllimport) S32 __stdcall AIL_set_sequence_loop_count(HSEQUENCE seq, S32 count);
    __declspec(dllimport) void __stdcall AIL_stop_sequence(HSEQUENCE seq);
    __declspec(dllimport) void __stdcall AIL_resume_sequence(HSEQUENCE seq);
    __declspec(dllimport) void __stdcall AIL_end_sequence(HSEQUENCE seq);

    // Per-sequence tuning + status.
    __declspec(dllimport) void __stdcall AIL_set_sequence_tempo(HSEQUENCE seq, S32 tempo, S32 ms);
    __declspec(dllimport) void __stdcall AIL_set_sequence_volume(HSEQUENCE seq, S32 volume, S32 ms);
    __declspec(dllimport) S32 __stdcall AIL_sequence_status(HSEQUENCE seq);
}

#endif // GRUNTZ_VENDOR_MSS32_H
