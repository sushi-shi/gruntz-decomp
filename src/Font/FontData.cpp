#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Font/FontData.h>
namespace assets {
bool readFont(io::Input& source, FontData& font) {
    io::BinaryReader reader(source);
    unsigned int count;
    if (!reader.u32(count) || count == 0 || count > 256) return false;
    FontData result;
    result.glyphs.resize(count, GlyphData());
    for (unsigned int i = 0; i < count; ++i) {
        GlyphData& glyph = result.glyphs[i];
        if (!reader.u32(glyph.width) || !reader.u32(glyph.height)
            || glyph.width > 0x7fffffffU || glyph.height > 0x7fffffffU) return false;
        const size_t available = reader.remaining();
        if (!source.good() || (glyph.height && glyph.width > available / glyph.height)) return false;
        const size_t length = static_cast<size_t>(glyph.width) * glyph.height;
        glyph.pixels.resize(length);
        if (!reader.bytes(length ? &glyph.pixels[0] : NULL, length)) return false;
    }
    font.glyphs.swap(result.glyphs);
    return true;
}
bool writeFont(io::Output& target, const FontData& font) {
    if (font.glyphs.empty() || font.glyphs.size() > 256) return false;
    io::BinaryWriter writer(target);
    // Validate every glyph before emitting a partial record.
    for (size_t i = 0; i < font.glyphs.size(); ++i) {
        const GlyphData& glyph = font.glyphs[i];
        if (glyph.width > 0x7fffffffU || glyph.height > 0x7fffffffU
            || (glyph.height && glyph.width > 0x7fffffffU / glyph.height)
            || static_cast<size_t>(glyph.width) * glyph.height != glyph.pixels.size()) return false;
    }
    if (!writer.u32(static_cast<unsigned int>(font.glyphs.size()))) return false;
    for (size_t j = 0; j < font.glyphs.size(); ++j) {
        const GlyphData& glyph = font.glyphs[j];
        if (!writer.u32(glyph.width) || !writer.u32(glyph.height)
            || !writer.bytes(glyph.pixels.empty() ? NULL : &glyph.pixels[0], glyph.pixels.size())) return false;
    }
    return true;
}
}
