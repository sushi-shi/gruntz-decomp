#include <Io/FileStream.h>
#include <rva.h>

// CFileLog::ReopenSharedFile - reopen the shared file object around a close. Ignores
// `this`; the single stack arg is the path.
// @early-stop
// regalloc-tiebreak wall: both `path` (3x push arg) and `&g_obj646778` (3x ecx) are
// used 3 times; retail pins `path` in esi + re-materializes the global address as an
// immediate each call, cl pins the global address in esi + re-pushes path from the
// stack. Same code shape, opposite callee-saved pick; not source-steerable (~79%).
RVA(0x000bd3e0, 0x34)
void CFileLog::ReopenSharedFile(char* path) {
    g_obj646778.Open(path, 0x1000, 0);
    g_obj646778.Close();
    g_obj646778.Open(path, 1, 0);
}

RVA(0x000bd430, 0xa)
void CloseFileIOGlobal() {
    g_obj646778.Close();
}

RVA(0x000bd450, 0x16)
void CFileLog::OpenGruntzLog() {
    CloseFileIOGlobal();
    ReopenSharedFile("c:\\gruntz.log");
}
