#ifndef GRUNTZ_SERIALCLOCKMACROS_H
#define GRUNTZ_SERIALCLOCKMACROS_H

#include <Gruntz/SerialArchive.h>

#define SERIALIZE_CLOCK_PAIR(ar, mode, first, second)                                              \
    do {                                                                                           \
        if ((mode) != SERIAL_SAVE) {                                                               \
            if ((mode) == SERIAL_LOAD) {                                                           \
                (ar)->Read(&(first), sizeof(first));                                               \
                (ar)->Read(&(second), sizeof(second));                                             \
            }                                                                                      \
        } else {                                                                                   \
            (ar)->Write(&(first), sizeof(first));                                                  \
            (ar)->Write(&(second), sizeof(second));                                                \
        }                                                                                          \
    } while (0)

#endif // GRUNTZ_SERIALCLOCKMACROS_H
