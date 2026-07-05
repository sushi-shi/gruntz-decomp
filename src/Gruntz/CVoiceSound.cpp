// CVoiceSound.cpp (renamed from Obj11c630.cpp) - CVoiceSound::CVoiceSound, the
// voice-sound record CSpawnEntry::AddVoiceSound (0x11c560, gruntspawnconfig) builds
// and appends to its CObList: {CString name, i32 = 0, i32 flag}. The former
// "Obj11c630" was a placeholder view of the canonical CVoiceSound (<Gruntz/
// GruntSpawnConfig.h>); folded onto it (ctor proven 2-arg: name-by-value + flag,
// ret 8 - the delinked disasm reads [esp+0x1c]=arg2 into m_08).
//
// The ctor takes the CString name by value (default-construct m_str 0x1b9b93,
// assign from the by-value arg 0x1b9e25), sets m_04/m_08, then the by-value CString
// param is destroyed by the callee at return (0x1b9cde) -> /GX EH frame. MFC
// CString ctor/operator=/dtor are external (reloc-masked); flags=eh.
#include <rva.h>
#include <Gruntz/GruntSpawnConfig.h> // canonical CVoiceSound

RVA(0x0011c630, 0x6e)
CVoiceSound::CVoiceSound(CString s, i32 flag) {
    m_str = s;
    m_04 = 0;
    m_08 = flag;
}
