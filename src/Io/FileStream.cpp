#include <StdAfx.h>
#include <Io/File.h>

#include <Ints.h>

#include <Io/FileStream.h>

#include <Gruntz/Multi.h>

io::File g_gruntzLogFile;

bool CFileLog::ReopenSharedFile(const std::string& path) {
    return g_gruntzLogFile.open(path, io::Replace);
}

void CloseFileIOGlobal() {
    g_gruntzLogFile.finish();
}

bool CFileLog::OpenGruntzLog() {
    CloseFileIOGlobal();
    return ReopenSharedFile("gruntz.log");
}

i32 CFileLog::IsLoggingEnabled() {
    return 0;
}

void CMulti::WriteTag(const char*) {}
