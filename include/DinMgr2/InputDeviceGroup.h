#ifndef DINMGR2_INPUTDEVICEGROUP_H
#define DINMGR2_INPUTDEVICEGROUP_H

#include <Ints.h>

class CInputDevBase;

class CInputDeviceGroup {
public:
    CInputDeviceGroup() : m_reserved00(0), m_count(0) {}

    void Clear();
    i32 FillFrom(CInputDevBase** src, i32 n, i32 unused);
    i32 Add(CInputDevBase* item);

    i32 m_reserved00;
    i32 m_count;
    CInputDevBase* m_items[32];
};

#endif // DINMGR2_INPUTDEVICEGROUP_H
