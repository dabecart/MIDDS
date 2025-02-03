#ifndef FONTS_h
#define FONTS_h

#include <stdint.h>

typedef struct FontDef {
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;

extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

#endif // FONTS_h
