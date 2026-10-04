#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/SavePaths.h>
#include <stdio.h>
#include <string.h>

namespace io {
std::string slotBaseName(unsigned int slot) {
    if (slot >= 10) return "";
    char name[24];
    sprintf(name, "Slot%u.sav", slot + 1);
    return name;
}
bool snapshotName(unsigned int slot, const char* field, size_t capacity, std::string& name) {
    if (!field || !capacity) return false;
    const char* end = static_cast<const char*>(memchr(field, 0, capacity));
    if (!end) return false;
    std::string stored(field, end - field);
    const size_t slash = stored.find_last_of("/\\");
    if (slash != std::string::npos) stored.erase(0, slash + 1);
    const std::string base = slotBaseName(slot);
    if (base.empty()) return false;
    if (stored != base) {
        const std::string prefix = base + ".stage-";
        if (stored.compare(0, prefix.size(), prefix) != 0) return false;
        const std::string digits = stored.substr(prefix.size());
        if (digits.empty() || digits.size() > 4) return false;
        unsigned int value = 0;
        for (size_t i = 0; i < digits.size(); ++i) {
            if (digits[i] < '0' || digits[i] > '9') return false;
            value = value * 10 + static_cast<unsigned int>(digits[i] - '0');
        }
        if (value >= 1024) return false;
    }
    name.swap(stored);
    return true;
}
void removePreviousSnapshot(unsigned int slot, const std::string& directory,
    const char* previous, size_t capacity, const std::string& currentName) {
    std::string oldName;
    std::string newName;
    if (!snapshotName(slot, previous, capacity, oldName)
        || !snapshotName(slot, currentName.c_str(), currentName.size() + 1, newName)
        || oldName == newName) return;
    const std::string path = directory + "/" + oldName;
    remove(path.c_str());
}

}
