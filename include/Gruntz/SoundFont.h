#ifndef GRUNTZ_GRUNTZ_SOUNDFONT_H
#define GRUNTZ_GRUNTZ_SOUNDFONT_H

#include <Ints.h>

i32 SFManager_SelectBestDevice();
void CloseSoundFontDevice();
i32 BuildSoundFontPath(char drive);

#endif // GRUNTZ_GRUNTZ_SOUNDFONT_H
