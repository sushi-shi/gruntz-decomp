// WwdWorker.h - the per-object worker the WWD factories allocate (0x17c bytes) and
// stamp into each object's +0x7c. Ctor (0x15b300) seeds it from the parent root +
// object handle + flags; Kick (vtbl+0x10) starts it when the object carries the
// 0x200000 flag. External/no-body so the calls reloc-mask. Placeholder name;
// offsets + code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CWWDWORKER_H
#define GRUNTZ_GRUNTZ_CWWDWORKER_H

#include <rva.h>

class CWwdWorker {
public:
    void Ctor(i32 root, i32 a, i32 flags); // 0x15b300
    virtual void V00();
    virtual void V04();
    virtual void V08();
    virtual void V0C();
    virtual i32 Kick(void* owner); // +0x10
};

#endif // GRUNTZ_GRUNTZ_CWWDWORKER_H
