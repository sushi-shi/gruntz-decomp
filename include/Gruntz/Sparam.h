#ifndef GRUNTZ_GRUNTZ_SPARAM_H
#define GRUNTZ_GRUNTZ_SPARAM_H

#include <Ints.h>

i32 Sparam_Get(char* sDest, const char* sSource, const char* sId);
i32 Sparam_Add(char* sSource, const char* sId, const char* sParam);
i32 Sparam_Add(char* sSource, const char* sId, i32 nParam);

#endif // GRUNTZ_GRUNTZ_SPARAM_H
