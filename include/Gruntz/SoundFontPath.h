#ifndef GRUNTZ_GRUNTZ_SOUNDFONTPATH_H
#define GRUNTZ_GRUNTZ_SOUNDFONTPATH_H

#include <Dsndmgr/SfManager.h>
#include <Ints.h>

extern u16 g_sfDeviceId;

extern char g_sfMusic4[];
extern char g_sfLocal4[];
extern char g_sfMusic[];
extern char g_sfLocal[];
extern char g_sfDir[];

extern CSFMIDILocation g_sfMidiLocation;
extern CSFBufferObject g_sfBufferObject;

i32 SfDeviceInitKeys();
i32 SoundFontFileExists(char* path);
#endif // GRUNTZ_GRUNTZ_SOUNDFONTPATH_H
