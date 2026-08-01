#ifndef GRUNTZ_SERIALARCHIVE_H
#define GRUNTZ_SERIALARCHIVE_H

class CFileMemBase;

typedef enum SerialMode {
    SERIAL_SAVE = 4,
    SERIAL_LOAD = 7,
} SerialMode;

extern "C" char g_syncErrMsgBuf[];

#endif // GRUNTZ_SERIALARCHIVE_H
