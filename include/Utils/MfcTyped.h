#ifndef GRUNTZ_UTILS_MFCTYPED_H
#define GRUNTZ_UTILS_MFCTYPED_H

#include <Mfc.h>

template<class NodeT> union MfcListPosition {
    POSITION m_position;
    NodeT* m_node;
};

template<class NodeT> inline NodeT* MfcNodeFromPosition(POSITION position) {
    MfcListPosition<NodeT> value;
    value.m_position = position;
    return value.m_node;
}

template<class NodeT> inline POSITION MfcPositionFromNode(NodeT* node) {
    MfcListPosition<NodeT> value;
    value.m_node = node;
    return value.m_position;
}

template<class T> union MfcPtrArrayStorage {
    void** m_untyped;
    T** m_typed;
};

template<class T> inline T** MfcPtrArrayData(CPtrArray& array) {
    MfcPtrArrayStorage<T> data;
    data.m_untyped = array.GetData();
    return data.m_typed;
}

#endif // GRUNTZ_UTILS_MFCTYPED_H
