// RecordFill.cpp - a leaf memory helper.
//   0x17f500  ZeroRecords  (__stdcall) - inline-memset `count` 0x28-byte records.
#include <rva.h>
#include <string.h>

// ---------------------------------------------------------------------------
// 0x17f500 - zero `count` consecutive 0x28-byte records at `dst`.
// ---------------------------------------------------------------------------
RVA(0x0017f500, 0x23)
void __stdcall ZeroRecords(void* dst, int count) {
    memset(dst, 0, count * 0x28);
}
