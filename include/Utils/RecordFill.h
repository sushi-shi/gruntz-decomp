#ifndef UTILS_RECORDFILL_H
#define UTILS_RECORDFILL_H

// ZeroRecords (0x17f500) - clear `count` 0x28-byte records. The out-of-line record
// eraser CRezBufferObject::Serialize's realloc arm calls (the other two arms zero
// their tail inline); defined in src/Utils/RecordFill.cpp.
void __stdcall ZeroRecords(void* dst, int count);

#endif // UTILS_RECORDFILL_H
