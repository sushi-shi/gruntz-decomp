#include <StdAfx.h>

#include <Ints.h>

#include <Rez/DebugPrintf.h>

#include <Rez/DebugPrintfInternals.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u16* g_dprintfmonoscreen;

u32 g_dbprintfcurrentLine = 0;

u32 g_dprintfcurrentChar = 0;

dprintfOutputType g_dprintfOutType = DPRINTF_UNKNOWN;

FILE* g_dprintffile = NULL;

static dprintfinittype s_dprintfinit;

BOOLEAN dprintfExcludeRegions::In(u32 Num) {
    u32 Loop;
    for (Loop = 0; Loop < m_numRegions; Loop++) {
        if (Num >= m_ary[Loop].m_from && Num <= m_ary[Loop].m_to) {
            return TRUE;
        }
    }
    return FALSE;
}

void dprintfExcludeRegions::Add(u32 From, u32 To) {
    if (m_numRegions + 1 < MAX_EXCLUDE_REGIONS) {
        m_ary[m_numRegions].m_from = From;
        m_ary[m_numRegions].m_to = To;
        m_numRegions++;
    }
}

void dprintfExcludeRegions::Scan(char* Str) {
    char TmpStr[BUFSIZE];
    char* P;
    i32 From;
    i32 To;
    while (*Str != 0) {
        Str = strstr(Str, "X");
        if (Str == NULL) {
            return;
        }
        Str = strpbrk(Str, "0123456789");
        if (Str == NULL) {
            return;
        }
        strcpy(TmpStr, Str);
        P = TmpStr;
        while (*P != 0) {
            if (*P >= '0' && *P <= '9') {
                P++;
                Str++;
            } else {
                *P = 0;
            }
        }
        From = atol(TmpStr);
        if (*Str == '-') {
            Str = strpbrk(Str, "0123456789");
            if (Str == NULL) {
                return;
            }
            strcpy(TmpStr, Str);
            P = TmpStr;
            while (*P != 0) {
                if (*P >= '0' && *P <= '9') {
                    P++;
                    Str++;
                } else {
                    *P = 0;
                }
            }
            To = atol(TmpStr);
        } else {
            To = From;
        }
        Add(From, To);
    }
}

dprintfExcludeRegions g_dprintfExReg;

void dprintfmonoincline() {
    g_dprintfcurrentChar = 0;
    if (++g_dbprintfcurrentLine == LPP) {
        i32 i;
        for (i = CPL; i < CPL * LPP; i++) {
            g_dprintfmonoscreen[i - CPL] = g_dprintfmonoscreen[i];
        }
        for (i = CPL * (LPP - 1); i < CPL * LPP; i++) {
            g_dprintfmonoscreen[i] = ATTR + ' ';
        }
        g_dbprintfcurrentLine--;
    }
}

void dprintfmonoclrscr() {
    i32 i;
    for (i = 0; i < CPL * LPP; i++) {
        g_dprintfmonoscreen[i] = ATTR + ' ';
    }
    g_dbprintfcurrentLine = 0;
    g_dprintfcurrentChar = 0;
}

void dprintfmonoprint(char* message) {
    OutputDebugStringA(message);
}

void dprintfdoprint(char* Str) {}

void dprintf(char* fmt, ...) {
    if (g_dprintfOutType == DPRINTF_NOTHING || g_dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (g_dprintfExReg.In(0)) {
        return;
    }

    va_list ap;
    char buf[BUFSIZE];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    dprintfdoprint(buf);
}

void dprintf(i32 x, i32 y, char* fmt, ...) {
    if (g_dprintfOutType == DPRINTF_NOTHING || g_dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (g_dprintfExReg.In(0)) {
        return;
    }

    dgotoxy(x, y);
    va_list ap;
    char buf[BUFSIZE];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    dprintfdoprint(buf);
}

void dprintf(u32 Level, char* fmt, ...) {
    if (g_dprintfOutType == DPRINTF_NOTHING || g_dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (g_dprintfExReg.In(Level)) {
        return;
    }

    va_list ap;
    char buf[BUFSIZE];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    dprintfdoprint(buf);
}

void dprintf(u32 Level, i32 x, i32 y, char* fmt, ...) {
    if (g_dprintfOutType == DPRINTF_NOTHING || g_dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (g_dprintfExReg.In(Level)) {
        return;
    }

    dgotoxy(x, y);
    va_list ap;
    char buf[BUFSIZE];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    dprintfdoprint(buf);
}

void dgotoxy(i32 x, i32 y) {
    dgotoxy(0, x, y);
}

void dgotoxy(u32 Level, i32 x, i32 y) {}

void dclrscr() {
    dclrscr(0);
}

void dclrscr(u32 Level) {}

dprintfinittype::dprintfinittype() {
    char Buf[BUFSIZE];
    g_dprintfExReg.m_numRegions = 0;
    g_dprintfOutType = DPRINTF_NOTHING;
    g_dprintfOutType = DPRINTF_NOTHING;
    char* Str = getenv("DPRINTF");
    if (Str != NULL) {
        strcpy(Buf, Str);
        _strupr(Buf);
        if (strstr(Buf, "MONO")) {
            g_dprintfOutType = DPRINTF_MONOCHROME;
        }
        if (strstr(Buf, "FILE")) {
            g_dprintfOutType = DPRINTF_FILE;
        }
        if (strstr(Buf, "FILEAPPEND")) {
            g_dprintfOutType = DPRINTF_FILEAPPEND;
        }
        if (strstr(Buf, "COM1")) {
            g_dprintfOutType = DPRINTF_COM1;
        }
        if (strstr(Buf, "COM2")) {
            g_dprintfOutType = DPRINTF_COM2;
        }
        if (strstr(Buf, "STDOUT")) {
            g_dprintfOutType = DPRINTF_STDOUT;
        }
        if (strstr(Buf, "LPT1")) {
            g_dprintfOutType = DPRINTF_LPT1;
        }
        if (strstr(Buf, "LPT2")) {
            g_dprintfOutType = DPRINTF_LPT1;
        }
        if (strstr(Buf, "PRN")) {
            g_dprintfOutType = DPRINTF_PRN;
        }
        g_dprintfExReg.Scan(Buf);
    }
    g_dprintfOutType = DPRINTF_MONOCHROME;

    switch (g_dprintfOutType) {
        case DPRINTF_FILE:
            g_dprintffile = fopen("DPRINTF.OUT", "w");
            if (g_dprintffile == NULL) {
                g_dprintfOutType = DPRINTF_NOTHING;
            }
            break;
        case DPRINTF_FILEAPPEND:
            g_dprintffile = fopen("DPRINTF.OUT", "w");
            fclose(g_dprintffile);
            break;
        case DPRINTF_LPT1:
            g_dprintffile = fopen("LPT1", "w");
            if (g_dprintffile == NULL) {
                g_dprintfOutType = DPRINTF_NOTHING;
            }
            break;
        case DPRINTF_LPT2:
            g_dprintffile = fopen("LPT2", "w");
            if (g_dprintffile == NULL) {
                g_dprintfOutType = DPRINTF_NOTHING;
            }
            break;
        case DPRINTF_PRN:
            g_dprintffile = fopen("PRN", "w");
            if (g_dprintffile == NULL) {
                g_dprintfOutType = DPRINTF_NOTHING;
            }
            break;
    }
}

dprintfinittype::~dprintfinittype() {
    switch (g_dprintfOutType) {
        case DPRINTF_FILE:
        case DPRINTF_LPT1:
        case DPRINTF_LPT2:
        case DPRINTF_PRN:
            fclose(g_dprintffile);
            break;
    }
}
