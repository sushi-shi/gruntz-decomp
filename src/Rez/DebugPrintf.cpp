#include <rva.h>

#include <Rez/DebugPrintf.h>

#include <Win32.h>

#include <Rez/DebugPrintfInternals.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002bf84c)
u16* dprintfmonoscreen;
DATA(0x002bf8d4)
u32 dbprintfcurrentLine = 0;
DATA(0x002bf8d8)
u32 dprintfcurrentChar = 0;
DATA(0x002bf8dc)
dprintfOutputType dprintfOutType = DPRINTF_UNKNOWN;
DATA(0x002bf8e0)
FILE* dprintffile = NULL;

RVA_DYNINIT(0x00184b60, 0xa, dprintfinit)
RVA_DYNINIT(0x00184b70, 0xa, dprintfinit)
RVA_DYNINIT(0x00184b80, 0xe, dprintfinit)
RVA_DYNINIT(0x00184b90, 0xa, dprintfinit)
DATA(0x002bf848)
static dprintfinittype dprintfinit;

RVA(0x00184ba0, 0x33)
BOOLEAN dprintfExcludeRegions::In(u32 Num) {
    u32 Loop;
    for (Loop = 0; Loop < NumRegions; Loop++) {
        if (Num >= Ary[Loop].From && Num <= Ary[Loop].To) {
            return TRUE;
        }
    }
    return FALSE;
}

RVA(0x00184be0, 0x24)
void dprintfExcludeRegions::Add(u32 From, u32 To) {
    if (NumRegions + 1 < MAX_EXCLUDE_REGIONS) {
        Ary[NumRegions].From = From;
        Ary[NumRegions].To = To;
        NumRegions++;
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
dprintfExcludeRegions dprintfExReg;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184d50, 0x5f)
void dprintfmonoincline() {
    dprintfcurrentChar = 0;
    if (++dbprintfcurrentLine == LPP) {
        i32 i;
        for (i = CPL; i < CPL * LPP; i++) {
            dprintfmonoscreen[i - CPL] = dprintfmonoscreen[i];
        }
        for (i = CPL * (LPP - 1); i < CPL * LPP; i++) {
            dprintfmonoscreen[i] = ATTR + ' ';
        }
        dbprintfcurrentLine--;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184db0, 0x28)
void dprintfmonoclrscr() {
    i32 i;
    for (i = 0; i < CPL * LPP; i++) {
        dprintfmonoscreen[i] = ATTR + ' ';
    }
    dbprintfcurrentLine = 0;
    dprintfcurrentChar = 0;
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
    if (dprintfOutType == DPRINTF_NOTHING || dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (dprintfExReg.In(0)) {
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
    if (dprintfOutType == DPRINTF_NOTHING || dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (dprintfExReg.In(0)) {
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
    if (dprintfOutType == DPRINTF_NOTHING || dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (dprintfExReg.In(Level)) {
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
    if (dprintfOutType == DPRINTF_NOTHING || dprintfOutType == DPRINTF_UNKNOWN) {
        return;
    }
    if (dprintfExReg.In(Level)) {
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
    dprintfExReg.NumRegions = 0;
    dprintfOutType = DPRINTF_NOTHING;
    dprintfOutType = DPRINTF_NOTHING;
    char* Str = getenv("DPRINTF");
    if (Str != NULL) {
        strcpy(Buf, Str);
        _strupr(Buf);
        if (strstr(Buf, "MONO")) {
            dprintfOutType = DPRINTF_MONOCHROME;
        }
        if (strstr(Buf, "FILE")) {
            dprintfOutType = DPRINTF_FILE;
        }
        if (strstr(Buf, "FILEAPPEND")) {
            dprintfOutType = DPRINTF_FILEAPPEND;
        }
        if (strstr(Buf, "COM1")) {
            dprintfOutType = DPRINTF_COM1;
        }
        if (strstr(Buf, "COM2")) {
            dprintfOutType = DPRINTF_COM2;
        }
        if (strstr(Buf, "STDOUT")) {
            dprintfOutType = DPRINTF_STDOUT;
        }
        if (strstr(Buf, "LPT1")) {
            dprintfOutType = DPRINTF_LPT1;
        }
        if (strstr(Buf, "LPT2")) {
            dprintfOutType = DPRINTF_LPT1;
        }
        if (strstr(Buf, "PRN")) {
            dprintfOutType = DPRINTF_PRN;
        }
        dprintfExReg.Scan(Buf);
    }
    dprintfOutType = DPRINTF_MONOCHROME;

    switch (dprintfOutType) {
        case DPRINTF_FILE:
            dprintffile = fopen("DPRINTF.OUT", "w");
            if (dprintffile == NULL) {
                dprintfOutType = DPRINTF_NOTHING;
            }
            break;
        case DPRINTF_FILEAPPEND:
            dprintffile = fopen("DPRINTF.OUT", "w");
            fclose(dprintffile);
            break;
        case DPRINTF_LPT1:
            dprintffile = fopen("LPT1", "w");
            if (dprintffile == NULL) {
                dprintfOutType = DPRINTF_NOTHING;
            }
            break;
        case DPRINTF_LPT2:
            dprintffile = fopen("LPT2", "w");
            if (dprintffile == NULL) {
                dprintfOutType = DPRINTF_NOTHING;
            }
            break;
        case DPRINTF_PRN:
            dprintffile = fopen("PRN", "w");
            if (dprintffile == NULL) {
                dprintfOutType = DPRINTF_NOTHING;
            }
            break;
    }
}

RVA(0x001851b0, 0x23)
dprintfinittype::~dprintfinittype() {
    switch (dprintfOutType) {
        case DPRINTF_FILE:
        case DPRINTF_LPT1:
        case DPRINTF_LPT2:
        case DPRINTF_PRN:
            fclose(dprintffile);
            break;
    }
}
