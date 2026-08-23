#include <rva.h>

#include <Io/FileStream.h>

#include <Gruntz/Multi.h>

RVA(0x000bd3e0, 0x34)
void CFileLog::ReopenSharedFile(char* path) {
    g_gruntzLogFile.Open(path, 0x1000, NULL);
    g_gruntzLogFile.Close();
    g_gruntzLogFile.Open(path, 1, NULL);
}

RVA(0x000bd430, 0xa)
void CloseFileIOGlobal() {
    g_gruntzLogFile.Close();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000bd450, 0x16)
void CFileLog::OpenGruntzLog() {
    CloseFileIOGlobal();
    ReopenSharedFile("c:\\gruntz.log");
}

// @identity-TODO: owner, ABI, and false result are proven; the query identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000bd480, 0x3)
i32 CFileLog::IsLoggingEnabled() {
    return 0;
}

RVA(0x000bd4a0, 0x3)
void CMulti::WriteTag(const char*) {}
