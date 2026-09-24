#ifndef GRUNTZ_GRUNTZ_STATUSBARSERIALMACROS_H
#define GRUNTZ_GRUNTZ_STATUSBARSERIALMACROS_H

#define SER(field)                                                                                 \
    if (field) {                                                                                   \
        if ((field)->SerializeFields(s, mode, typeId, payload) == 0)                               \
            return 0;                                                                              \
    }

#endif // GRUNTZ_GRUNTZ_STATUSBARSERIALMACROS_H
