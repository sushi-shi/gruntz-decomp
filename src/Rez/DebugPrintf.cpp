#include <rva.h>

#include <Rez/DebugPrintf.h>

#include <Win32.h>

#include <Rez/DebugPrintfInternals.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002bf84c)
u16* g_dprintfmonoscreen;
DATA(0x002bf8d4)
u32 g_dbprintfcurrentLine = 0;
DATA(0x002bf8d8)
u32 g_dprintfcurrentChar = 0;
DATA(0x002bf8dc)
dprintfOutputType g_dprintfOutType = DPRINTF_UNKNOWN;
DATA(0x002bf8e0)
FILE* g_dprintffile = NULL;

RVA_DYNINIT(0x00184b60, 0xa, s_dprintfinit)
RVA_DYNINIT(0x00184b70, 0xa, s_dprintfinit)
RVA_DYNINIT(0x00184b80, 0xe, s_dprintfinit)
RVA_DYNINIT(0x00184b90, 0xa, s_dprintfinit)
DATA(0x002bf848)
static dprintfinittype s_dprintfinit;

RVA(0x00184ba0, 0x33)
BOOLEAN dprintfExcludeRegions::In(u32 Num) {
    u32 Loop;
    for (Loop = 0; Loop < m_NumRegions; Loop++) {
        if (Num >= m_Ary[Loop].m_From && Num <= m_Ary[Loop].m_To) {
            return TRUE;
        }
    }
    return FALSE;
}

RVA(0x00184be0, 0x24)
void dprintfExcludeRegions::Add(u32 From, u32 To) {
    if (m_NumRegions + 1 < MAX_EXCLUDE_REGIONS) {
        m_Ary[m_NumRegions].m_From = From;
        m_Ary[m_NumRegions].m_To = To;
        m_NumRegions++;
    }
}

RVA(0x00184c10, 0x136)
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

DATA(0x002bf850)
dprintfExcludeRegions g_dprintfExReg;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184d50, 0x5f)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184db0, 0x28)
void dprintfmonoclrscr() {
    i32 i;
    for (i = 0; i < CPL * LPP; i++) {
        g_dprintfmonoscreen[i] = ATTR + ' ';
    }
    g_dbprintfcurrentLine = 0;
    g_dprintfcurrentChar = 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184de0, 0xc)
void dprintfmonoprint(char* message) {
    OutputDebugStringA(message);
}

RVA(0x00184df0, 0x1)
void dprintfdoprint(char* Str) {}

RVA(0x00184e00, 0x55)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184e60, 0x6d)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184ed0, 0x5b)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184f30, 0x73)
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

RVA(0x00184fb0, 0x15)
void dgotoxy(i32 x, i32 y) {
    dgotoxy(0, x, y);
}

RVA(0x00184fd0, 0x1)
void dgotoxy(u32 Level, i32 x, i32 y) {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184fe0, 0xb)
void dclrscr() {
    dclrscr(0);
}

RVA(0x00184ff0, 0x1)
void dclrscr(u32 Level) {}

RVA(0x00185000, 0x1a6)
dprintfinittype::dprintfinittype() {
    char Buf[BUFSIZE];
    g_dprintfExReg.m_NumRegions = 0;
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

RVA(0x001851b0, 0x23)
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
