#ifndef GRUNTZ_GRUNTZ_HEAPDIAG_H
#define GRUNTZ_GRUNTZ_HEAPDIAG_H

#include <Ints.h>

i32 FileExists(const char* path);

struct tagMODULEENTRY32;
namespace Utils {
    namespace WinAPI {
        i32 LegacyFindModule(
            unsigned long th32ProcessID,
            unsigned long moduleID,
            void* outBuf,
            unsigned long bufSize
        );
    } // namespace WinAPI
} // namespace Utils

namespace ApiCallerStubs {
    void ReportHeapStatus(i32 status);
}

#endif // GRUNTZ_GRUNTZ_HEAPDIAG_H
