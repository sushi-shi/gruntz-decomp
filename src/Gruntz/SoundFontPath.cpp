#include <Gruntz/SoundFontPath.h>
#include <rva.h>
#include <Gruntz/SoundFont.h>

#include <Win32.h>

#include <Dsndmgr/SfManager.h>
#include <stdio.h>
#include <string.h>
#include <Gruntz/SFSelectDevice.h>

RVA(0x000f8e20, 0x56)
void CloseSoundFontDevice() {
    if (g_sfReady != 0 && g_sfDevice != 0 && g_sfDeviceCount != 0) {
        SfDeviceInitKeys();
        g_sfDevice->SF_Close(g_sfDeviceId);
        FreeLibrary(g_sfDll);
        g_sfDll = 0;
        g_sfReady = 0;
    }
}

RVA(0x000f8ec0, 0x50)
i32 SfDeviceInitKeys() {
    if (g_sfReady == 0) {
        return 0;
    }
    g_sfMidiLocation.m_PresetIndex = 0;
    for (i32 i = 1; i <= 0x7f; i++) {
        g_sfMidiLocation.m_BankIndex = static_cast<WORD>(i);

        // Byte-evidenced two-argument vendor ABI.
        (reinterpret_cast<SfGetLoadedBankPathname2>(g_sfDevice->SF_GetLoadedBankPathname))(
            g_sfDeviceId,
            &g_sfMidiLocation
        );
    }
    g_sfMidiLocation.m_BankIndex = 1;
    return 1;
}

RVA(0x000f8f30, 0x160)
i32 BuildSoundFontPath(char drive) {
    if (g_sfReady == 0) {
        return 0;
    }

    GetCurrentDirectoryA(0xff, g_sfDir);
    int len = static_cast<int>(strlen(g_sfDir));
    if (len > 0 && g_sfDir[len - 1] == '\\') {
        g_sfDir[len - 1] = '\0';
    }
    sprintf(g_sfLocal4, "%s\\Gruntz4.SF2", g_sfDir);
    sprintf(g_sfLocal, "%s\\Gruntz.SF2", g_sfDir);
    sprintf(g_sfMusic4, "%c:\\MUSIC\\Gruntz4.SF2", drive);
    sprintf(g_sfMusic, "%c:\\MUSIC\\Gruntz.SF2", drive);
    SfDeviceInitKeys();

    g_sfMidiLocation.m_BankIndex = 1;
    g_sfMidiLocation.m_PresetIndex = 0;
    g_sfBufferObject.m_Size = 0x80;
    g_sfBufferObject.m_Flag = 0;
    int hiVer = 0;
    i32 res = 0x6b;
    if (g_sfVer >= 0x37115c) {
        hiVer = 1;
    }
    g_sfBufferObject.m_Buffer = hiVer ? g_sfLocal4 : g_sfLocal;
    if (SoundFontFileExists(g_sfBufferObject.m_Buffer)) {
        res = g_sfDevice->SF_LoadBank(g_sfDeviceId, &g_sfMidiLocation, &g_sfBufferObject);
    }
    if (res != 0) {
        g_sfBufferObject.m_Buffer = hiVer ? g_sfMusic4 : g_sfMusic;
        res = g_sfDevice->SF_LoadBank(g_sfDeviceId, &g_sfMidiLocation, &g_sfBufferObject);
    }
    return res == 0;
}

RVA(0x000f90f0, 0x45)
i32 SoundFontFileExists(char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000) != -1;
}
