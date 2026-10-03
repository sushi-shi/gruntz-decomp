/* Build-time exports used only to generate mss32.lib. */
#define EXPORT __declspec(dllexport) void __stdcall
EXPORT AIL_release_sequence_handle(int handle) {}
EXPORT AIL_set_XMIDI_master_volume(int driver, int volume) {}
EXPORT AIL_XMIDI_master_volume(int driver) {}
EXPORT AIL_allocate_sequence_handle(int driver) {}
EXPORT AIL_set_sequence_loop_count(int sequence, int count) {}
EXPORT AIL_start_sequence(int sequence) {}
EXPORT AIL_end_sequence(int sequence) {}
EXPORT AIL_init_sequence(int sequence, int start, int number) {}
EXPORT AIL_resume_sequence(int sequence) {}
EXPORT AIL_sequence_status(int sequence) {}
EXPORT AIL_set_sequence_tempo(int sequence, int tempo, int milliseconds) {}
EXPORT AIL_set_sequence_volume(int sequence, int volume, int milliseconds) {}
EXPORT AIL_shutdown(void) {}
EXPORT AIL_startup(void) {}
EXPORT AIL_midiOutOpen(int driver, int midi, int device) {}
EXPORT AIL_stop_sequence(int sequence) {}
