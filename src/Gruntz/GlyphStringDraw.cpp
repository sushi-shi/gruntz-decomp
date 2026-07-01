// GlyphStringDraw.cpp - DrawGlyphString (0x115220), a __cdecl text helper: walks
// a string, maps each char through a glyph font's [lo..hi] range table, and
// dispatches a draw call per printable glyph onto the context's drawable.
#include <Ints.h>
#include <rva.h>

extern "C" unsigned int strlen(const char* s);

struct Drawable {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void Draw(i32 x, i32 y, i32 glyph, i32 z); // slot +0x28
};

struct DrawCtx {
    char pad00[0xc];
    Drawable* m_0c; // +0x0c
};

struct GlyphFont {
    char pad00[0x14];
    i32* m_14; // +0x14 glyph table
    char pad18[0x64 - 0x18];
    i32 m_64; // +0x64 lo char
    i32 m_68; // +0x68 hi char
};

RVA(0x00115220, 0xa4)
i32 DrawGlyphString(DrawCtx* ctx, i32 x, i32 y, const char* str, GlyphFont* font, i32 advance) {
    if (!ctx) {
        return 0;
    }
    if (!str) {
        return 0;
    }
    if (!font) {
        return 0;
    }
    i32 len = (i32)strlen(str);
    if (len <= 0) {
        return 0;
    }
    for (i32 i = 0; i < len; i++) {
        i32 c = (signed char)str[i];
        i32 glyph;
        if (c >= font->m_64 && c <= font->m_68) {
            glyph = font->m_14[c];
        } else {
            glyph = 0;
        }
        if (glyph) {
            ctx->m_0c->Draw(x, y, glyph, 0);
        }
        x += advance;
    }
    return 1;
}

SIZE_UNKNOWN(Drawable);
SIZE_UNKNOWN(DrawCtx);
SIZE_UNKNOWN(GlyphFont);
