#include <StdAfx.h>
#include <Wap32/PlatformText.h>
#include <vector>

bool loadResourceText(unsigned int id, std::string& text) {
    std::vector<char> buffer(256);
    for (;;) {
        const int length = LoadStringA(AfxGetResourceHandle(), id, &buffer[0], buffer.size());
        if (length < static_cast<int>(buffer.size()) - 1 || buffer.size() >= 65536) {
            text.assign(&buffer[0], length);
            return length != 0;
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::string readWindowText(HWND window) {
    std::vector<char> buffer(GetWindowTextLengthA(window) + 1);
    const int length = GetWindowTextA(window, &buffer[0], buffer.size());
    return std::string(&buffer[0], length);
}

std::string readListBoxText(HWND window, int index, bool combo) {
    const LRESULT length = SendMessageA(window, combo ? CB_GETLBTEXTLEN : LB_GETTEXTLEN, index, 0);
    if (length < 0) return std::string();
    std::vector<char> buffer(length + 1);
    const LRESULT copied = SendMessageA(window, combo ? CB_GETLBTEXT : LB_GETTEXT,
        index, reinterpret_cast<LPARAM>(&buffer[0]));
    return copied < 0 ? std::string() : std::string(&buffer[0], copied);
}
