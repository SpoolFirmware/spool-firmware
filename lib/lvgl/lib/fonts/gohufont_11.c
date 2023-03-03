/*******************************************************************************
 * Size: 11 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

/*
            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar
  14 rue de Plaisance, 75014 Paris, France
 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include "../lvgl.h"

#ifndef GOHUFONT_11
#define GOHUFONT_11 1
#endif

#if GOHUFONT_11

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,

    /* U+0021 "!" */
    0x0, 0x82, 0x8, 0x20, 0x82, 0x0, 0x20, 0x0,
    0x0,

    /* U+0022 "\"" */
    0x1, 0x45, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,

    /* U+0023 "#" */
    0x1, 0x45, 0x3e, 0x53, 0xe5, 0x14, 0x0, 0x0,
    0x0,

    /* U+0024 "$" */
    0x0, 0x87, 0x2a, 0xa1, 0xc2, 0xaa, 0x70, 0x80,
    0x0,

    /* U+0025 "%" */
    0x0, 0x4, 0xaa, 0x50, 0x85, 0x2a, 0x90, 0x0,
    0x0,

    /* U+0026 "&" */
    0x0, 0x6, 0x24, 0xa1, 0xa, 0xa4, 0x68, 0x0,
    0x0,

    /* U+0027 "'" */
    0x0, 0x82, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,

    /* U+0028 "(" */
    0x0, 0x42, 0x8, 0x41, 0x4, 0x8, 0x20, 0x40,
    0x0,

    /* U+0029 ")" */
    0x1, 0x2, 0x8, 0x10, 0x41, 0x8, 0x21, 0x0,
    0x0,

    /* U+002A "*" */
    0x0, 0x0, 0x8, 0xa9, 0xca, 0x88, 0x0, 0x0,
    0x0,

    /* U+002B "+" */
    0x0, 0x0, 0x8, 0x23, 0xe2, 0x8, 0x0, 0x0,
    0x0,

    /* U+002C "," */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x18, 0x60, 0x84,
    0x0,

    /* U+002D "-" */
    0x0, 0x0, 0x0, 0x3, 0xe0, 0x0, 0x0, 0x0,
    0x0,

    /* U+002E "." */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x18, 0x60, 0x0,
    0x0,

    /* U+002F "/" */
    0x0, 0x20, 0x84, 0x10, 0x82, 0x10, 0x42, 0x8,
    0x0,

    /* U+0030 "0" */
    0x0, 0x7, 0x22, 0x9a, 0xac, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0031 "1" */
    0x0, 0x2, 0x18, 0xa0, 0x82, 0x8, 0x20, 0x0,
    0x0,

    /* U+0032 "2" */
    0x0, 0x7, 0x22, 0x8, 0x42, 0x10, 0xf8, 0x0,
    0x0,

    /* U+0033 "3" */
    0x0, 0x7, 0x22, 0x8, 0xc0, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0034 "4" */
    0x0, 0x1, 0xc, 0x52, 0x4f, 0x84, 0x10, 0x0,
    0x0,

    /* U+0035 "5" */
    0x0, 0xf, 0xa0, 0xf0, 0x20, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0036 "6" */
    0x0, 0x7, 0x20, 0xf2, 0x28, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0037 "7" */
    0x0, 0xf, 0x82, 0x10, 0x42, 0x8, 0x20, 0x0,
    0x0,

    /* U+0038 "8" */
    0x0, 0x7, 0x22, 0x89, 0xc8, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0039 "9" */
    0x0, 0x7, 0x22, 0x89, 0xe0, 0x82, 0x70, 0x0,
    0x0,

    /* U+003A ":" */
    0x0, 0x0, 0x0, 0x30, 0xc0, 0xc, 0x30, 0x0,
    0x0,

    /* U+003B ";" */
    0x0, 0x0, 0x0, 0x61, 0x80, 0x18, 0x60, 0x84,
    0x0,

    /* U+003C "<" */
    0x0, 0x0, 0x84, 0x21, 0x2, 0x4, 0x8, 0x0,
    0x0,

    /* U+003D "=" */
    0x0, 0x0, 0x0, 0xf8, 0xf, 0x80, 0x0, 0x0,
    0x0,

    /* U+003E ">" */
    0x0, 0x4, 0x8, 0x10, 0x21, 0x8, 0x40, 0x0,
    0x0,

    /* U+003F "?" */
    0x0, 0x7, 0x22, 0x8, 0x42, 0x0, 0x20, 0x0,
    0x0,

    /* U+0040 "@" */
    0x0, 0x7, 0x22, 0xba, 0xab, 0xa0, 0x78, 0x0,
    0x0,

    /* U+0041 "A" */
    0x1, 0xc8, 0xa2, 0xfa, 0x28, 0xa2, 0x88, 0x0,
    0x0,

    /* U+0042 "B" */
    0x3, 0xc8, 0xa2, 0xf2, 0x28, 0xa2, 0xf0, 0x0,
    0x0,

    /* U+0043 "C" */
    0x1, 0xc8, 0xa0, 0x82, 0x8, 0x22, 0x70, 0x0,
    0x0,

    /* U+0044 "D" */
    0x3, 0xc8, 0xa2, 0x8a, 0x28, 0xa2, 0xf0, 0x0,
    0x0,

    /* U+0045 "E" */
    0x3, 0xe8, 0x20, 0xf2, 0x8, 0x20, 0xf8, 0x0,
    0x0,

    /* U+0046 "F" */
    0x3, 0xe8, 0x20, 0xf2, 0x8, 0x20, 0x80, 0x0,
    0x0,

    /* U+0047 "G" */
    0x1, 0xc8, 0xa0, 0xba, 0x28, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0048 "H" */
    0x2, 0x28, 0xa2, 0xfa, 0x28, 0xa2, 0x88, 0x0,
    0x0,

    /* U+0049 "I" */
    0x1, 0xc2, 0x8, 0x20, 0x82, 0x8, 0x70, 0x0,
    0x0,

    /* U+004A "J" */
    0x0, 0x20, 0x82, 0x8, 0x28, 0xa2, 0x70, 0x0,
    0x0,

    /* U+004B "K" */
    0x2, 0x29, 0x28, 0xc2, 0x89, 0x22, 0x88, 0x0,
    0x0,

    /* U+004C "L" */
    0x2, 0x8, 0x20, 0x82, 0x8, 0x20, 0xf8, 0x0,
    0x0,

    /* U+004D "M" */
    0x2, 0x2d, 0xaa, 0xaa, 0x28, 0xa2, 0x88, 0x0,
    0x0,

    /* U+004E "N" */
    0x2, 0x2c, 0xb2, 0xaa, 0xa9, 0xa6, 0x88, 0x0,
    0x0,

    /* U+004F "O" */
    0x1, 0xc8, 0xa2, 0x8a, 0x28, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0050 "P" */
    0x3, 0xc8, 0xa2, 0xf2, 0x8, 0x20, 0x80, 0x0,
    0x0,

    /* U+0051 "Q" */
    0x1, 0xc8, 0xa2, 0x8a, 0x2a, 0xa4, 0x68, 0x20,
    0x0,

    /* U+0052 "R" */
    0x3, 0xc8, 0xa2, 0xf2, 0x48, 0xa2, 0x88, 0x0,
    0x0,

    /* U+0053 "S" */
    0x1, 0xc8, 0xa0, 0x70, 0x20, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0054 "T" */
    0x3, 0xe2, 0x8, 0x20, 0x82, 0x8, 0x20, 0x0,
    0x0,

    /* U+0055 "U" */
    0x2, 0x28, 0xa2, 0x8a, 0x28, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0056 "V" */
    0x2, 0x28, 0xa2, 0x89, 0x45, 0x8, 0x20, 0x0,
    0x0,

    /* U+0057 "W" */
    0x2, 0x28, 0xa2, 0xaa, 0xaa, 0x94, 0x50, 0x0,
    0x0,

    /* U+0058 "X" */
    0x2, 0x28, 0x94, 0x21, 0x48, 0xa2, 0x88, 0x0,
    0x0,

    /* U+0059 "Y" */
    0x2, 0x28, 0x94, 0x20, 0x82, 0x8, 0x20, 0x0,
    0x0,

    /* U+005A "Z" */
    0x3, 0xe0, 0x84, 0x21, 0x8, 0x20, 0xf8, 0x0,
    0x0,

    /* U+005B "[" */
    0x0, 0xe2, 0x8, 0x20, 0x82, 0x8, 0x20, 0xe0,
    0x0,

    /* U+005C "\\" */
    0x1, 0x4, 0x8, 0x20, 0x41, 0x2, 0x8, 0x10,
    0x40,

    /* U+005D "]" */
    0x3, 0x82, 0x8, 0x20, 0x82, 0x8, 0x23, 0x80,
    0x0,

    /* U+005E "^" */
    0x0, 0x2, 0x14, 0x88, 0x0, 0x0, 0x0, 0x0,
    0x0,

    /* U+005F "_" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xf0,
    0x0,

    /* U+0060 "`" */
    0x1, 0x2, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,

    /* U+0061 "a" */
    0x0, 0x0, 0x0, 0x70, 0x27, 0xa2, 0x78, 0x0,
    0x0,

    /* U+0062 "b" */
    0x2, 0x8, 0x20, 0xb3, 0x28, 0xa2, 0xf0, 0x0,
    0x0,

    /* U+0063 "c" */
    0x0, 0x0, 0x0, 0x7a, 0x8, 0x20, 0x78, 0x0,
    0x0,

    /* U+0064 "d" */
    0x0, 0x20, 0x82, 0x7a, 0x28, 0xa6, 0x68, 0x0,
    0x0,

    /* U+0065 "e" */
    0x0, 0x0, 0x0, 0x72, 0x2f, 0xa0, 0x78, 0x0,
    0x0,

    /* U+0066 "f" */
    0x0, 0xc4, 0x10, 0x71, 0x4, 0x10, 0x40, 0x0,
    0x0,

    /* U+0067 "g" */
    0x0, 0x0, 0x0, 0x7a, 0x28, 0xa6, 0x68, 0x27,
    0x0,

    /* U+0068 "h" */
    0x2, 0x8, 0x20, 0xb3, 0x28, 0xa2, 0x88, 0x0,
    0x0,

    /* U+0069 "i" */
    0x0, 0x2, 0x0, 0x60, 0x82, 0x8, 0x30, 0x0,
    0x0,

    /* U+006A "j" */
    0x0, 0x2, 0x0, 0x60, 0x82, 0x8, 0x20, 0x8c,
    0x0,

    /* U+006B "k" */
    0x2, 0x8, 0x20, 0x92, 0x8e, 0x24, 0x88, 0x0,
    0x0,

    /* U+006C "l" */
    0x1, 0x82, 0x8, 0x20, 0x82, 0x8, 0x18, 0x0,
    0x0,

    /* U+006D "m" */
    0x0, 0x0, 0x0, 0xf2, 0xaa, 0xaa, 0xa8, 0x0,
    0x0,

    /* U+006E "n" */
    0x0, 0x0, 0x0, 0xf2, 0x28, 0xa2, 0x88, 0x0,
    0x0,

    /* U+006F "o" */
    0x0, 0x0, 0x0, 0x72, 0x28, 0xa2, 0x70, 0x0,
    0x0,

    /* U+0070 "p" */
    0x0, 0x0, 0x0, 0xb3, 0x28, 0xa2, 0xf2, 0x8,
    0x0,

    /* U+0071 "q" */
    0x0, 0x0, 0x0, 0x7a, 0x28, 0xa6, 0x68, 0x20,
    0x80,

    /* U+0072 "r" */
    0x0, 0x0, 0x0, 0xb3, 0x28, 0x20, 0x80, 0x0,
    0x0,

    /* U+0073 "s" */
    0x0, 0x0, 0x0, 0x72, 0x7, 0x2, 0xf0, 0x0,
    0x0,

    /* U+0074 "t" */
    0x1, 0x4, 0x10, 0xf1, 0x4, 0x10, 0x30, 0x0,
    0x0,

    /* U+0075 "u" */
    0x0, 0x0, 0x0, 0x8a, 0x28, 0xa6, 0x68, 0x0,
    0x0,

    /* U+0076 "v" */
    0x0, 0x0, 0x0, 0x8a, 0x25, 0x14, 0x20, 0x0,
    0x0,

    /* U+0077 "w" */
    0x0, 0x0, 0x0, 0x8a, 0xaa, 0xaa, 0x50, 0x0,
    0x0,

    /* U+0078 "x" */
    0x0, 0x0, 0x0, 0x89, 0x42, 0x14, 0x88, 0x0,
    0x0,

    /* U+0079 "y" */
    0x0, 0x0, 0x0, 0x8a, 0x28, 0xa6, 0x68, 0x27,
    0x0,

    /* U+007A "z" */
    0x0, 0x0, 0x0, 0xf8, 0x42, 0x10, 0xf8, 0x0,
    0x0,

    /* U+007B "{" */
    0x18, 0x82, 0x8, 0x23, 0x2, 0x8, 0x20, 0x81,
    0x80,

    /* U+007C "|" */
    0x0, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x80,
    0x0,

    /* U+007D "}" */
    0xc0, 0x82, 0x8, 0x20, 0x62, 0x8, 0x20, 0x8c,
    0x0,

    /* U+007E "~" */
    0x0, 0x0, 0x0, 0x42, 0xa1, 0x0, 0x0, 0x0,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 9, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 18, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 27, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 36, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 45, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 54, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 63, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 72, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 81, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 90, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 99, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 108, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 117, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 126, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 135, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 144, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 153, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 162, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 171, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 180, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 189, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 198, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 207, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 216, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 225, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 234, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 243, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 252, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 261, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 270, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 279, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 288, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 297, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 306, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 315, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 324, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 333, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 342, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 351, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 360, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 369, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 378, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 387, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 396, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 405, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 414, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 423, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 432, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 441, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 450, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 459, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 468, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 477, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 486, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 495, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 504, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 513, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 522, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 531, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 540, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 549, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 558, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 567, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 576, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 585, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 594, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 603, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 612, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 621, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 630, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 639, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 648, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 657, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 666, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 675, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 684, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 693, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 702, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 711, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 720, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 729, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 738, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 747, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 756, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 765, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 774, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 783, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 792, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 801, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 810, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 819, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 828, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 837, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 846, .adv_w = 96, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t gohufont_11 = {
#else
lv_font_t gohufont_11 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 11,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if GOHUFONT_11*/

