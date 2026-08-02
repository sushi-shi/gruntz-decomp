#ifndef GRUNTZ_GAMEINFO_H
#define GRUNTZ_GAMEINFO_H

#include <Ints.h>
#include <rva.h>

struct CGameInfoTime {
    i32 m_0;
    u32 m_4;
    u32 m_timeMs;

    i32 m_month;
    i32 m_day;
    i32 m_year;
    i32 m_18;
};
SIZE_UNKNOWN();

class CGameInfo {
public:
    i32 SetNames(char* name, char* name2, i32 unused);
    i32 CopyBody(char* body);
    i32 Update(i32 s, i32 timestamp, i32 type);
    i32 CopyIfLarger(CGameInfoTime* src, i32 type);
    i32 HasSupportedVersion();
    i32 FormatGameInfoString();

    char m_00[4];
    i32 m_04;
    u32 m_version;
    char m_pad0c[0x14 - 0xc];
    char m_name[0x36 - 0x14];
    char m_location[0xb8 - 0x36];
    CGameInfoTime m_time;
    u32 m_type;
};
SIZE_UNKNOWN();

i32 BuildGameDate(CGameInfoTime* out);

#endif // GRUNTZ_GAMEINFO_H
