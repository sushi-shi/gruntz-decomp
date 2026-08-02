#ifndef NET_NETSESSION_H
#define NET_NETSESSION_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

i32 MakeButeSectionKey(char* dst, const char* section, const char* key);
void AppendInt(char* dst, const char* sep, i32 n);

#endif // NET_NETSESSION_H
