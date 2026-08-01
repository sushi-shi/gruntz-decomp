#ifndef GRUNTZ_COMOUTREF_H
#define GRUNTZ_COMOUTREF_H

template<class T> union ComOutRef {
    void** m_asVoid;
    T** m_asTyped;
};

#endif // GRUNTZ_COMOUTREF_H
