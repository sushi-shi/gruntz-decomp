#ifndef GRUNTZ_IO_SAVEPATHS_H
#define GRUNTZ_IO_SAVEPATHS_H
#include <string>
#include <stddef.h>
namespace io {
std::string slotBaseName(unsigned int slot);
bool snapshotName(unsigned int slot, const char* field, size_t capacity, std::string& name);
// Best-effort cleanup after publication; compare validated basenames so a reused
// filename is retained regardless of the path separators in the previous record.
void removePreviousSnapshot(unsigned int slot, const std::string& directory,
    const char* previous, size_t capacity, const std::string& currentName);
}
#endif
