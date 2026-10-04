#ifndef GRUNTZ_DDRAWMGR_FRAMEREFERENCE_H
#define GRUNTZ_DDRAWMGR_FRAMEREFERENCE_H

#include <string>
#include <Ints.h>

struct FrameReference {
    FrameReference() : frameIndex(0) {}
    FrameReference(const std::string& name, i32 index) : workerName(name), frameIndex(index) {}

    std::string workerName;
    i32 frameIndex;
};

#endif
