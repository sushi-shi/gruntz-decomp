#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/Settings.h>
#include <Io/File.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

namespace {
const size_t maxConfigBytes = 1024 * 1024;
const char signature[] = "GRUNTZ CONFIG 1\n";
std::string normalize(const std::string& name) {
    std::string result(name);
    for (size_t i = 0; i < result.size(); ++i)
        if (result[i] >= 'A' && result[i] <= 'Z') result[i] += 'a' - 'A';
    return result;
}
std::string escape(const std::string& value) {
    const char hex[] = "0123456789ABCDEF";
    std::string result;
    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (c < 32 || c == '%' || c == '=' || c >= 127) {
            result += '%'; result += hex[c >> 4]; result += hex[c & 15];
        } else result += static_cast<char>(c);
    }
    return result;
}
int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
bool unescape(const std::string& value, std::string& result) {
    result.erase();
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '%') {
            if (i + 2 >= value.size()) return false;
            const int high = hexDigit(value[i + 1]), low = hexDigit(value[i + 2]);
            if (high < 0 || low < 0) return false;
            c = static_cast<char>((high << 4) | low); i += 2;
        }
        if (!c) return false;
        result += c;
    }
    return true;
}
bool parseInteger(const std::string& text, int& value) {
    if (text.empty()) return false;
    size_t digit = text[0] == '-' ? 1 : 0;
    if (digit == text.size()) return false;
    for (size_t i = digit; i < text.size(); ++i)
        if (text[i] < '0' || text[i] > '9') return false;
    errno = 0;
    char* end;
    const long number = strtol(text.c_str(), &end, 10);
    if (errno == ERANGE || *end || number < (-2147483647L - 1) || number > 2147483647L) return false;
    value = static_cast<int>(number);
    return true;
}
}
std::string settingsPath() {
    const char* path = getenv("GRUNTZ_CONFIG");
    return path && *path ? std::string(path) : std::string("gruntz.cfg");
}
int Settings::getInt(const std::string& key, int fallback) const {
    std::map<std::string, Value>::const_iterator found = m_values.find(normalize(key));
    return found != m_values.end() && found->second.integer ? found->second.number : fallback;
}
std::string Settings::getString(const std::string& key, const std::string& fallback) const {
    std::map<std::string, Value>::const_iterator found = m_values.find(normalize(key));
    return found != m_values.end() && !found->second.integer ? found->second.text : fallback;
}
void Settings::setInt(const std::string& key, int value) {
    Value entry; entry.integer = true; entry.number = value;
    m_values[normalize(key)] = entry;
}
void Settings::setString(const std::string& key, const std::string& value) {
    Value entry; entry.text = value;
    m_values[normalize(key)] = entry;
}
bool Settings::decode(io::Input& source) {
    io::BinaryReader reader(source);
    const size_t length = reader.remaining();
    if (!source.good() || length < sizeof(signature) - 1 || length > maxConfigBytes) return false;
    std::vector<char> raw(length);
    if (!reader.bytes(length ? &raw[0] : NULL, length)) return false;
    std::string text;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n') continue;
        text += raw[i];
    }
    if (text.compare(0, sizeof(signature) - 1, signature) != 0) return false;
    std::map<std::string, Value> values;
    size_t start = sizeof(signature) - 1;
    while (start < text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(start, end - start);
        start = end + 1;
        if (line.size() < 4 || line[1] != ' ' || (line[0] != 's' && line[0] != 'i')) return false;
        const size_t split = line.find('=', 2);
        if (split == std::string::npos) return false;
        std::string key, value;
        if (!unescape(line.substr(2, split - 2), key) || key.empty()
            || !unescape(line.substr(split + 1), value)) return false;
        key = normalize(key);
        if (values.find(key) != values.end()) return false;
        Value entry;
        entry.integer = line[0] == 'i';
        if (entry.integer) { if (!parseInteger(value, entry.number)) return false; }
        else entry.text = value;
        values[key] = entry;
    }
    m_values.swap(values);
    return true;
}
bool Settings::encode(io::Output& target) const {
    std::string text(signature);
    for (std::map<std::string, Value>::const_iterator i = m_values.begin(); i != m_values.end(); ++i) {
        if (i->first.empty() || i->first.find('\0') != std::string::npos
            || i->second.text.find('\0') != std::string::npos) return false;
        text += i->second.integer ? "i " : "s ";
        text += escape(i->first); text += '=';
        if (i->second.integer) { char number[16]; sprintf(number, "%d", i->second.number); text += number; }
        else text += escape(i->second.text);
        text += '\n';
        if (text.size() > maxConfigBytes) return false;
    }
    return target.write(text.data(), text.size());
}
bool Settings::load(const std::string& path) {
    std::string resolved;
    if (!io::absolutePath(path, resolved)) return false;
    io::File file;
    if (!file.open(resolved, io::ReadOnly)) {
        if (file.error() != io::NotFound) return false;
        m_values.clear(); m_path = resolved; m_loaded = true; return true;
    }
    Settings parsed;
    if (!parsed.decode(file) || !file.finish()) return false;
    m_values.swap(parsed.m_values); m_path = resolved; m_loaded = true;
    return true;
}
bool Settings::save() const {
    if (!m_loaded) return false;
    // Validate and encode before touching the previous configuration on disk.
    io::MemoryOutput bytes;
    if (!encode(bytes)) return false;
    const std::string temporary = m_path + ".tmp";
    io::File file;
    if (!file.open(temporary, io::Replace)) return false;
    const std::vector<unsigned char>& data = bytes.bytes();
    const bool written = file.write(data.empty() ? NULL : &data[0], data.size());
    const bool closed = file.finish();
    if (!written || !closed || !io::replaceFile(temporary, m_path)) {
        remove(temporary.c_str()); return false;
    }
    return true;
}
