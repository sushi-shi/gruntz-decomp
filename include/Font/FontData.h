#ifndef GRUNTZ_FONT_DATA_H
#define GRUNTZ_FONT_DATA_H
#include <Io/Bytes.h>
namespace assets {
struct GlyphData {
    GlyphData() : width(0), height(0) {}
    unsigned int width;
    unsigned int height;
    std::vector<unsigned char> pixels;
};
struct FontData { std::vector<GlyphData> glyphs; };
bool readFont(io::Input& source, FontData& font);
bool writeFont(io::Output& target, const FontData& font);
}
#endif
