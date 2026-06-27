// CustomWwdPath.cpp - the custom-level path resolver (0x03b940, __cdecl, returns a
// CString by value). Given a bare WWD name, it rewrites it to an absolute custom
// path "<cwd>\CUSTOM\<NAME>.WWD" (upper-cased, ".WWD" appended if absent). Names
// that are empty, already contain a backslash (already a path), or hit a getcwd
// failure are returned unchanged.
//
// Compiled /GX: the by-value CString param + the inner `orig` copy are destructible,
// so MSVC stamps the EH frame (push -1 / push handler / mov fs:0). The CString
// helpers are the real MFC symbols (operator=/+= /MakeUpper/GetLength), reloc-masked.
#include <rva.h>

#include <Mfc.h>    // CString + <windows.h>
#include <direct.h> // _getcwd
#include <string.h> // strstr

RVA(0x0003b940, 0x19d)
CString BuildCustomWwdPath(CString name) {
    if (name.GetLength() == 0) {
        return name;
    }
    if (strstr(name, "\\") != 0) {
        return name;
    }
    char cwd[254];
    if (_getcwd(cwd, 254) == 0) {
        return name;
    }
    CString orig = name;
    name = cwd;
    name += "\\CUSTOM\\";
    name += orig;
    name.MakeUpper();
    if (strstr(name, ".WWD") == 0) {
        name += ".WWD";
    }
    return name;
}
