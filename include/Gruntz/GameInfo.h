#ifndef GRUNTZ_GAMEINFO_H
#define GRUNTZ_GAMEINFO_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

struct CGameInfoTime {
    i32 m_0;
    u32 m_score;
    u32 m_timeMs;

    i32 m_month;
    i32 m_day;
    i32 m_year;
    i32 m_reserved;
};
SIZE_UNKNOWN();

struct CGameInfoBody {
    i32 m_headerWord;
    u32 m_version;
    char m_pad08[0x10 - 0x08];
    char m_name[0x32 - 0x10];
    char m_location[0xb4 - 0x32];
    CGameInfoTime m_time;
    u32 m_type;
};
SIZE(0xd4);

class CGameInfo {
public:
    i32 SetNames(char* name, char* name2, i32 unused);
    i32 CopyBody(char* body);
    i32 Update(i32 s, i32 timestamp, i32 type);
    i32 CopyIfLarger(CGameInfoTime* src, i32 type);
    i32 HasSupportedVersion();
    i32 FormatGameInfoString();

    char m_reserved00[4];
    CGameInfoBody m_body;
};
SIZE(0xd8);

i32 BuildGameDate(CGameInfoTime* out);

#endif // GRUNTZ_GAMEINFO_H
