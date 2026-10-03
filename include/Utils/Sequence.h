#ifndef GRUNTZ_UTILS_SEQUENCE_H
#define GRUNTZ_UTILS_SEQUENCE_H

#include <vector>
#include <list>
#include <stdexcept>

template<class T, class U>
void growAndAssign(std::vector<T>& values, int index, const U& value) {
    if (index < 0) throw std::out_of_range("Negative sequence index");
    T stored = value;
    if (static_cast<typename std::vector<T>::size_type>(index) >= values.size()) {
        values.resize(static_cast<typename std::vector<T>::size_type>(index) + 1);
    }
    values[index] = stored;
}

template<class T>
T takeFront(std::list<T>& values) {
    T value = values.front();
    values.pop_front();
    return value;
}

template<class T>
T takeBack(std::list<T>& values) {
    T value = values.back();
    values.pop_back();
    return value;
}

template<class Iterator>
Iterator iteratorAt(Iterator first, Iterator last, int index) {
    if (index < 0) return last;
    while (index-- > 0 && first != last) ++first;
    return first;
}

#endif
