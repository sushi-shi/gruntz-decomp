#ifndef GRUNTZ_SETTINGS_H
#define GRUNTZ_SETTINGS_H
#include <Io/Bytes.h>
#include <map>
#include <string>

class Settings {
public:
    Settings() : m_loaded(false) {}
    bool load(const std::string& path);
    bool save() const;
    bool decode(io::Input& source);
    bool encode(io::Output& target) const;
    int getInt(const std::string& key, int fallback = 0) const;
    std::string getString(const std::string& key, const std::string& fallback = "") const;
    void setInt(const std::string& key, int value);
    void setString(const std::string& key, const std::string& value);
    bool loaded() const { return m_loaded; }
private:
    struct Value {
        Value() : integer(false), number(0) {}
        bool integer;
        int number;
        std::string text;
    };
    std::map<std::string, Value> m_values;
    std::string m_path;
    bool m_loaded;
};
std::string settingsPath();
#endif
