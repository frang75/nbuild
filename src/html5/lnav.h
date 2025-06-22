/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: lnav.h
 *
 */

/* Html5 Laterial Nav */

#include "html5.hxx"

__EXTERN_C

LNav *lnav_create(
    const real32_t width,
    const color_t bkcolor,
    const color_t hbkcolor,
    const color_t sbkcolor,
    const color_t txcolor,
    const color_t hvcolor,
    const color_t secolor,
    const real32_t fsize1,
    const real32_t fsize2);

void lnav_write_l1(LNav *lnav, const char_t *text, const char_t *url, const bool_t selected, Stream *html);

void lnav_write_l2(LNav *lnav, const char_t *text, const char_t *url, const bool_t selected, Stream *html);

__END_C
