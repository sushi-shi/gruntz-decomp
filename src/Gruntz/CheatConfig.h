// CheatConfig.h - Cheat configuration and booty-cheat state declarations.
#ifndef SRC_GRUNTZ_CHEATCONFIG_H
#define SRC_GRUNTZ_CHEATCONFIG_H

// Minimal CButeMgr model for the cheat config getters (butemgr unit, matched).
// GetIntDef @0x171aa0 (3-arg, ret int), GetString @0x171a60 (ret bool),
// GetStringDef @0x173180 (5-arg, ret int/bool).
class CButeMgr {
public:
    int  GetIntDef(const char *tag, const char *key, int def);
    bool GetString(const char *tag, const char *key, char *buf, int bufSize);
    int  GetStringDef(const char *tag, const char *key, const char *def,
                      char *buf, int bufSize);
};
// @data: 0x2453d8
extern CButeMgr g_buteMgr;

// The game registry pointer (g_gameReg @0x64556c).
// @data: 0x24556c
extern int *g_gameReg;

// CString - minimal MFC string for SEH-frame locals.
class CString {
public:
    CString();              // ??0CString@@QAE@XZ         @0x1b9b93
    CString(const char *);  // ??0CString@@QAE@PBD@Z      @0x1b9d4c
    ~CString();             // ??1CString@@QAE@XZ         @0x1b9cde
    char *m_pchData;
};

// SYSTEMTIME for the expiry date check (kernel32::GetLocalTime).
typedef struct {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} SYSTEMTIME;

extern "C" void __stdcall GetLocalTime(SYSTEMTIME *lpSystemTime);

// The sprintf helper the engine uses (inline NAFXCW CRT, used via sprintf call).
extern "C" int sprintf(char *buf, const char *fmt, ...);

// strlen / memcpy / strcmp helpers (engine CRT).
extern "C" int strlen(const char *s);
extern "C" void *memcpy(void *dst, const void *src, int n);
extern "C" int strcmp(const char *a, const char *b);

// The booty directory provider / rez manager (unmatched, external).
// Called as this->m_04->OpenDir(name).
extern "C" void *__cdecl GetRezDir(void *mgr, const char *name);

// The "LoadBootyCheatState" owning class (partial layout).
class CBootyCheatOwner {
public:
    int LoadBootyCheatState(int a1, int a2, int a3);
    char _pad00[0x04];
    void *m_04;
    void *m_08;
    void *m_0c;
    char _pad10[0x18];
    void *m_28;
    void *m_2c;
    void *m_30;
    void *m_34;
    char _pad38[0x180];
    int  m_1b8;
    int  m_1bc;
    int  m_1c0;
    int  m_1c4;
    int  m_1c8;
    int  m_1cc;
};

// The "LoadCheatConfigEx" owning class (partial layout).
class CCheatConfigMgr {
public:
    int LoadCheatConfigEx(int a1, int a2);  // @0x205c0
    int LoadCheatConfig(int a1);            // @0x22e60
    int RegisterCheat(const char *name, int value, int isCheat); // @0x4269
    char _pad00[0x10];
    int  m_10;
    int  m_14;
};

#endif // SRC_GRUNTZ_CHEATCONFIG_H
