// CheatConfig.h - Shared declarations for cheat TUs.
#ifndef SRC_GRUNTZ_CHEATCONFIG_H
#define SRC_GRUNTZ_CHEATCONFIG_H

struct ButeString { char *m_pchData; };

class CButeMgr {
public:
    int        GetIntDef(const char *tag, const char *key, int def);
    ButeString *GetStringDef(const char *tag, const char *key, ButeString *def);
    char       *GetString(const char *tag, const char *key);
};
extern CButeMgr g_buteMgr;  // @data: 0x2453d8
extern int *g_gameReg;      // @data: 0x24556c

class CString {
public:
    CString();
    CString(const char *);
    ~CString();
    char *m_pchData;
};

typedef struct {
    unsigned short wYear, wMonth, wDayOfWeek, wDay;
    unsigned short wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME;

extern "C" void __stdcall GetLocalTime(SYSTEMTIME *);
extern "C" int sprintf(char *, const char *, ...);

struct CheatRegCall {
    void Call(const char *name, int value, int isCheat);
};

class CCheatConfigMgr {
public:
    int LoadCheatConfigEx(int a1, int a2);
    int LoadCheatConfig(int a1);
};

class CCheatOwner {
public:
    int LoadBootyCheatState(int a1, int a2, int a3);
};

class CMonolithDancer {
public:
    int ShowMonolithDanceMessage();
};

class CCheatHandler {
public:
    int ProcessCheatCode(unsigned int wParam, unsigned int lParam);
};

#endif
