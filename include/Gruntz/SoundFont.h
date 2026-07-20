#ifndef GRUNTZ_GRUNTZ_SOUNDFONT_H
#define GRUNTZ_GRUNTZ_SOUNDFONT_H

#include <Ints.h>

i32 SFManager_SelectBestDevice();   // 0x0f8970  pick + init the best SFMAN32 device
void CloseSoundFontDevice();        // 0x0f8e20  tear the selected device down
i32 BuildSoundFontPath(char drive); // 0x0f8f30  probe the 4 candidate SF2 paths

#endif // GRUNTZ_GRUNTZ_SOUNDFONT_H
