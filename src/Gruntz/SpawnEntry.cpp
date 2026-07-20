#include <rva.h>
#include <Gruntz/SpawnList.h> // canonical CSpawnEntry

RVA(0x0011c630, 0x6e)
CSpawnEntry::CSpawnEntry(CString name, i32 data) {
    m_name = name;
    m_flag = 0;
    m_data = data;
}
