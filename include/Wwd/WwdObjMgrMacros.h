#ifndef GRUNTZ_WWD_WWDOBJMGRMACROS_H
#define GRUNTZ_WWD_WWDOBJMGRMACROS_H

#define REGISTER_CHILD_OBJECT_ID(obj) m_registeredGameObjectsById[WwdKey(obj)] = obj

#define PLACE_OBJECT_RECT(dst, object, rect)                                                       \
    (dst).left = (object)->rect.left + (object)->m_screenPosition.m_x;                             \
    (dst).top = (object)->rect.top + (object)->m_screenPosition.m_y;                               \
    (dst).right = (object)->rect.right + (object)->m_screenPosition.m_x;                           \
    (dst).bottom = (object)->rect.bottom + (object)->m_screenPosition.m_y

#define REMOVE_ACTIVE_OBJECT_AT(pos, obj)                                                          \
    m_list.RemoveAt(pos);                                                                          \
    m_activeGameObjectsById.RemoveKey(WwdKey(obj))

#endif // GRUNTZ_WWD_WWDOBJMGRMACROS_H
