#include <StdAfx.h>

#include <Ints.h>

#include <Io/FileStream.h>

#include <Gruntz/Multi.h>

void CFileLog::ReopenSharedFile(char* path) {
    g_gruntzLogFile.Open(path, CFile::modeCreate, NULL);
    g_gruntzLogFile.Close();
    g_gruntzLogFile.Open(path, CFile::modeWrite, NULL);
}

void CloseFileIOGlobal() {
    g_gruntzLogFile.Close();
}

void CFileLog::OpenGruntzLog() {
    CloseFileIOGlobal();
    ReopenSharedFile("c:\\gruntz.log");
}

i32 CFileLog::IsLoggingEnabled() {
    return 0;
}

void CMulti::WriteTag(const char*) {}
