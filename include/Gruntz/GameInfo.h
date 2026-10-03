#ifndef GRUNTZ_GAMEINFO_H
#define GRUNTZ_GAMEINFO_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

struct CGameInfoTime {
    // @identity-TODO: both reserved words participate in whole-record copies and
    // zeroing; score comparisons and date formatting never interpret either one.
    i32 m_reserved00;
    u32 m_score;
    u32 m_timeMs;

    i32 m_month;
    i32 m_day;
    i32 m_year;
    i32 m_reserved;
};

struct CGameInfoBody {
    i32 m_headerWord;
    u32 m_version;
    // @identity-TODO: copied and cleared with m_body; m_name's retail offset
    // requires this span, but no member access identifies its contents.
    char m_pad08[0x10 - 0x08];
    char m_name[0x32 - 0x10];
    char m_location[0xb4 - 0x32];
    CGameInfoTime m_time;
    u32 m_type;
};

class CGameInfo {
public:
    i32 SetNames(char* name, char* name2, i32 unused);
    i32 CopyBody(char* body);
    void ClearTime();
    i32 UpdateBestScore(i32 score, i32 timeMs, i32 gameType);
    i32 CopyIfLarger(CGameInfoTime* src, i32 type);
    i32 HasSupportedVersion();
    i32 FormatGameInfoString();

    // @identity-TODO: methods begin at m_body; no constructor, allocation, or
    // vptr store establishes the role of this preceding word.
    char m_reserved00[4];
    CGameInfoBody m_body;
};

i32 BuildGameDate(CGameInfoTime* out);

#endif // GRUNTZ_GAMEINFO_H
