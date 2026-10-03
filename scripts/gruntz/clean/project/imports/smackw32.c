/* Build-time exports used only to generate smackw32.lib. */
#define EXPORT __declspec(dllexport) void __stdcall
EXPORT SmackSoundUseDirectSound(int directSound) {}
EXPORT SmackToBuffer(int smack, int left, int top, int pitch, int height, int buffer, int flags) {}
EXPORT SmackNextFrame(int smack) {}
EXPORT SmackOpen(int name, int flags, int extra) {}
EXPORT SmackWait(int smack) {}
EXPORT SmackGoto(int smack, int frame) {}
EXPORT SmackSoundOnOff(int smack, int on) {}
EXPORT SmackClose(int smack) {}
EXPORT SmackToBufferRect(int smack, int flags) {}
EXPORT SmackDoFrame(int smack) {}
