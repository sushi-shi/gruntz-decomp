// SpawnEntry.cpp (was CVoiceSound.cpp / Obj11c630.cpp) - CSpawnEntry::CSpawnEntry,
// the 12-byte named record's constructor. CSpawnList::AddVoiceSound (0x11c560,
// gruntspawnconfig) builds one per voice-sound name and appends it to the list:
// {CString m_name, i32 m_flag = 0, i32 m_data = the flag arg}. The canonical
// CSpawnEntry (<Gruntz/SpawnList.h>) unifies the former CVoiceSound / CSpawnEntryN
// / CObjResNode / Obj09a260 views (see that header's proof); its GetName (0x9a260)
// / GetTail (0x9a830) live in AreaMgr.cpp, the retail TU band.
//
// The ctor takes the CString name by value (default-construct m_name 0x1b9b93,
// assign from the by-value arg 0x1b9e25), zeroes m_flag, stores m_data, then the
// by-value CString param is destroyed by the callee at return (0x1b9cde) -> /GX
// EH frame. MFC CString ctor/operator=/dtor are external (reloc-masked); flags=eh.
#include <rva.h>
#include <Gruntz/SpawnList.h> // canonical CSpawnEntry

RVA(0x0011c630, 0x6e)
CSpawnEntry::CSpawnEntry(CString name, i32 data) {
    m_name = name;
    m_flag = 0;
    m_data = data;
}
